/*
 *
 * zibaldone - a C++/Java library for Thread, Timers and other Stuff
 *
 * http://sourceforge.net/projects/zibaldone/
 *
 * version 3.1.2, August 29th, 2015
 *
 * Copyright (C) 2012  ilant (ilant@users.sourceforge.net)
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 * ilant ilant@users.sourceforge.net
 *
 */

#include "Thread.h"
#include "Events.h"
#include "Log.h"
#include <algorithm>

namespace Z
{
//-------------------------------------------------------------------------------------------
class EvMng{
    EventManager EvMng;
} _evMng;
//-------------------------------------------------------------------------------------------
//Event
std::string Event::label()const{return _label;}

Event::Event(const std::string& label):_label(label)
{
    if(label.empty()) {
        ziblog(LOG::ERR, "ERROR: an instance of Event cannot have empty label");
        _label = "sob";
    }
}

Event::~Event(){_label.clear();}

void Event::emitEvent()
{
    if(_label==StopThreadLabel) {
        ziblog(LOG::ERR, "Invalid emitEvent with label=StopThreadLabel (it's reserved). Call Stop() instead");
        return;
    }
    std::deque < Thread * > listeners;
    pthread_mutex_lock(&EventManager::_rwReq);
    if(EventManager::_pendingWriters) pthread_cond_wait(&EventManager::_noPendingWriters, &EventManager::_rwReq);
    EventManager::_pendingReaders++;
    pthread_mutex_unlock(&EventManager::_rwReq);
    if(EventManager::_evtReceivers.find(_label) != EventManager::_evtReceivers.end()) listeners = EventManager::_evtReceivers[_label];
    for(size_t i = 0; i < listeners.size(); ++i) listeners[i]->pushIn(clone());
    pthread_mutex_lock(&EventManager::_rwReq);
    if(!--EventManager::_pendingReaders) pthread_cond_signal(&EventManager::_noPendingReaders);
    pthread_mutex_unlock(&EventManager::_rwReq);
}

//-------------------------------------------------------------------------------------------
//Thread
Thread::Thread()
{
    _th = 0;
    _set=false;
    pthread_mutex_init(&_lockQueue, NULL);
    pthread_mutex_init(&_lockSet, NULL);
    pthread_cond_init(&_thereIsAnEvent, NULL);
}

//VERY IMPORTANT! We couldn't embed Stop() into the Thread base class destructor because this way the thread loop
//function would continue running untill the base class destructor had not been called. As well known the base class
//destructor is called after the inherited class destructor, and so the thread loop would continue his life for a little
//time between the end of the inherited class destructor and the base class destructor call. This means that the thread
//loop function could use again some member variabiles (defined in the subclass) yet destroyed by subclass destructor.
Thread::~Thread()
{
    if(alive()) {
        ziblog(LOG::ERR, "Thread loop (run method) MUST BE END before destroying a Thread object");
        Stop();
    }
    StopReceivingAndDiscardReceived();
}

void * Thread::ThreadLoop(void * arg)
{
    ((Thread *)arg)->run();
    return NULL;
}

void Thread::Start()
{
    pthread_mutex_lock(&_lockSet);
    if(!_set) {
        pthread_create (&_th, NULL, ThreadLoop, this);
        _set=true;
    }
    pthread_mutex_unlock(&_lockSet);
}

void Thread::Stop()
{
    pthread_mutex_lock(&_lockSet);
    if(_set) {
        Event* stopThread = new StopThreadEvent();
        pushIn(stopThread);
        Join();
        _set=false;
        _th=0;
    }
    pthread_mutex_unlock(&_lockSet);
}

void Thread::Join()
{
    int ret = pthread_join(_th, NULL);
    if(ret == EDEADLK) ziblog(LOG::ERR, "thread cannot join himself");
}

void Thread::StopReceivingAndDiscardReceived()
{
    EventManager::unmapReceiver(this);
    pthread_mutex_lock(&_lockQueue);
    std::map < std::string, std::deque < Event* > >::iterator it;
    std::deque < Event* > * evList = NULL;
    for(it = _eventQueue.begin(); it != _eventQueue.end(); it++){
        evList = &(it->second);
        for(size_t i=0; i<evList->size(); i++) {
            delete (*evList)[i];
            (*evList)[i]=NULL;
        }
        evList->clear();
    }
    _eventQueue.clear();
    pthread_mutex_unlock(&_lockQueue);
}

void Thread::register2Label(const std::string& label){EventManager::mapLabel2Receiver(label, this);}

void Thread::pushIn(Event* ev)
{
    pthread_mutex_lock(&_lockQueue);
    std::map < std::string, std::deque < Event* > >::iterator it = _eventQueue.find(ev->label());
    if(it == _eventQueue.end()) {
        std::deque < Event* > evts;
        it = _eventQueue.insert(it, std::pair < std::string, std::deque < Event* > > (ev->label(),  evts));
    }
    it->second.push_front(ev);
    _chronologicalLabelSequence.push_front(ev->label());
    pthread_cond_signal(&_thereIsAnEvent);
    pthread_mutex_unlock(&_lockQueue);
}

Event* Thread::pullOut(int maxWaitMsec)
{
    pthread_mutex_lock(&_lockQueue);
    if (_eventQueue.empty()) {
        if(maxWaitMsec == NOWAIT) {
            pthread_mutex_unlock(&_lockQueue);
            return NULL;
        } else if(maxWaitMsec == WAIT4EVER) pthread_cond_wait(&_thereIsAnEvent, &_lockQueue);
        else {
            time_t s = maxWaitMsec/1000;
            long ns = (maxWaitMsec%1000)*1000000;
            struct timespec tp;
            clock_gettime(CLOCK_REALTIME,&tp);
            s += tp.tv_sec;
            ns += tp.tv_nsec;
            s += (ns/1000000000);
            ns %= 1000000000;
            struct timespec ts;
            ts.tv_nsec = ns;
            ts.tv_sec  = s;
            if(pthread_cond_timedwait(&_thereIsAnEvent, &_lockQueue, &ts)) {
                pthread_mutex_unlock(&_lockQueue);
                return NULL;
            }
        }
    }
    std::deque < Event* > *evQ = &(_eventQueue.find(_chronologicalLabelSequence.back())->second);
    _chronologicalLabelSequence.pop_back();
    Event* ret =  evQ->back();
    evQ->pop_back();
    if(evQ->empty()) _eventQueue.erase(ret->label());
    pthread_mutex_unlock(&_lockQueue);
    return ret;
}

Event* Thread::pullOut(const std::string& label, int maxWaitMsec)
{
    std::deque <std::string> labels;
    labels.push_front(label);
    return pullOut(labels, maxWaitMsec);
}

Event* Thread::pullOut(std::deque <std::string> labels, int maxWaitMsec)
{
    labels.push_front(StopThreadLabel);//StopThreadEvent MUST be always handled
    if(maxWaitMsec == WAIT4EVER) {
        ziblog(LOG::WRN, "invalid wait forever for a specific bundle of events... set wait to default value of 1 sec to prevent starvation...");
        maxWaitMsec = 1000;
    }
    std::map < std::string, std::deque < Event* > >::iterator it;
    bool timeout = false;
    time_t s = maxWaitMsec/1000;
    long ns = (maxWaitMsec%1000)*1000000;
    struct timespec tp;
    clock_gettime(CLOCK_REALTIME,&tp);
    s += tp.tv_sec;
    ns += tp.tv_nsec;
    s += (ns/1000000000);
    ns %= 1000000000;
    struct timespec ts;
    ts.tv_nsec = ns;
    ts.tv_sec  = s;
    pthread_mutex_lock(&_lockQueue);
    while (!timeout){
        for(size_t i=0; i<labels.size(); i++) {
            if((it = _eventQueue.find(labels[i])) != _eventQueue.end()) {
                Event* ret = (it->second).back();
                (it->second).pop_back();
                if((it->second).empty()) _eventQueue.erase(ret->label());
                for(size_t j = 0; j<_chronologicalLabelSequence.size(); j++) {
                    if(_chronologicalLabelSequence[j]==labels[i]) {
                        _chronologicalLabelSequence.erase(_chronologicalLabelSequence.begin()+j);
                        break;
                    }
                }
                pthread_mutex_unlock(&_lockQueue);
                return ret;
            }
        }
        if(pthread_cond_timedwait(&_thereIsAnEvent, &_lockQueue, &ts)) {
            pthread_mutex_unlock(&_lockQueue);
            return NULL;
        }
        clock_gettime(CLOCK_REALTIME,&tp);
        timeout = (tp.tv_sec > ts.tv_sec) || ((tp.tv_sec == ts.tv_sec) && (tp.tv_nsec >= ts.tv_nsec));
    }
    pthread_mutex_unlock(&_lockQueue);
    return NULL;
}

bool Thread::alive(){return (_th && !pthread_kill(_th, 0));}

//-------------------------------------------------------------------------------------------
//AutonomousThread
void AutonomousThread::run(){while(!exit) this->singleLoopCycle();}

AutonomousThread::AutonomousThread()
{
    pthread_mutex_init(&_lock, NULL);
    exit=true;
}

void AutonomousThread::Start()
{
    pthread_mutex_lock(&_lock);
    exit=false;
    Thread::Start();
    pthread_mutex_unlock(&_lock);
}

void AutonomousThread::Stop()
{
    pthread_mutex_lock(&_lock);
    exit=true;
    Join();
    pthread_mutex_unlock(&_lock);
}

//-------------------------------------------------------------------------------------------
//SameThreadAddressSpaceEventReceiver
Event* SameThreadAddressSpaceEventReceiver::pullOut(int maxWaitMsec)
{
    return Thread::pullOut(maxWaitMsec);
}

Event* SameThreadAddressSpaceEventReceiver::pullOut(const std::string& label, int maxWaitMsec)
{
    return Thread::pullOut(label, maxWaitMsec);
}

Event* SameThreadAddressSpaceEventReceiver::pullOut(std::deque <std::string> labels, int maxWaitMsec)
{
    return Thread::pullOut(labels, maxWaitMsec);
}

void SameThreadAddressSpaceEventReceiver::register2Label(const std::string& label)
{
    Thread::register2Label(label);
}

//-------------------------------------------------------------------------------------------
//EVENT MANAGER
pthread_mutex_t EventManager::_rwReq;
pthread_cond_t EventManager::_noPendingWriters;
pthread_cond_t EventManager::_noPendingReaders;
int EventManager::_pendingWriters;
int EventManager::_pendingReaders;

std::map < std::string, std::deque < Thread *> > EventManager::_evtReceivers;
pthread_mutex_t EventManager::_lockEvtReceivers;

EventManager::EventManager()
{
    pthread_mutex_init(&_rwReq, NULL);
    pthread_cond_init(&_noPendingReaders, NULL);
    pthread_cond_init(&_noPendingWriters, NULL);
    _pendingWriters = _pendingReaders = 0;
    pthread_mutex_init(&_lockEvtReceivers, NULL);
}

void EventManager::mapLabel2Receiver(const std::string& label, Thread *T)
{
    if(!label.length()) {
        ziblog(LOG::WRN, "INVALID EVENT NAME!");
        return;
    }
    pthread_mutex_lock(&_rwReq);
    if(_pendingReaders) pthread_cond_wait(&_noPendingReaders, &_rwReq);
    _pendingWriters++;
    pthread_mutex_unlock(&_rwReq);
    pthread_mutex_lock(&_lockEvtReceivers);
    if(_evtReceivers.find(label) == _evtReceivers.end()){
        std::deque < Thread *> listeners;
        _evtReceivers.insert(std::pair <std::string, std::deque < Thread *> >(label,listeners));
    }
    std::deque < Thread *>* evtReceiverList = &_evtReceivers[label];
    std::deque < Thread *>::iterator itEvtReceiverList;
    for(itEvtReceiverList = evtReceiverList->begin(); itEvtReceiverList != evtReceiverList->end(); itEvtReceiverList++) {
        if(*itEvtReceiverList == T) {//someone is trying to register twice to the same event...
            pthread_mutex_lock(&_rwReq);
            if(!--_pendingWriters) pthread_cond_signal(&_noPendingWriters);
            pthread_mutex_unlock(&_rwReq);
            pthread_mutex_unlock(&_lockEvtReceivers);
            return;
        }
    }//if we're here then the receiver is not registered.
    _evtReceivers[label].push_front(T);
    pthread_mutex_lock(&_rwReq);
    if(!--_pendingWriters) pthread_cond_signal(&_noPendingWriters);
    pthread_mutex_unlock(&_rwReq);
    pthread_mutex_unlock(&_lockEvtReceivers);
}

void EventManager::unmapReceiver(Thread *T)
{
    pthread_mutex_lock(&_rwReq);
    if(_pendingReaders) pthread_cond_wait(&_noPendingReaders, &_rwReq);
    _pendingWriters++;
    pthread_mutex_unlock(&_rwReq);
    pthread_mutex_lock(&_lockEvtReceivers);
    std::deque < Thread *>::iterator itThreadList;
    std::map < std::string, std::deque < Thread *> >::iterator itEvtRecvs = _evtReceivers.begin();
    while(itEvtRecvs != _evtReceivers.end()){
        itThreadList = std::remove(itEvtRecvs->second.begin(), itEvtRecvs->second.end(), T);
        itEvtRecvs->second.erase(itThreadList, itEvtRecvs->second.end());
        if(itEvtRecvs->second.empty()) _evtReceivers.erase(itEvtRecvs++);
        else itEvtRecvs++;
    }
    pthread_mutex_lock(&_rwReq);
    if(!--_pendingWriters) pthread_cond_signal(&_noPendingWriters);
    pthread_mutex_unlock(&_rwReq);
    pthread_mutex_unlock(&_lockEvtReceivers);
}
//-------------------------------------------------------------------------------------------
}//namespace Z
