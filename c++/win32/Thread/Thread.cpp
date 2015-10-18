/*
 *
 * zibaldone - a C++ library for Thread, Timers and other Stuff
 * http://sourceforge.net/projects/zibaldone/
 *
 * Copyright (C) 2012  ilant (ilant@users.sourceforge.net)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 2.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "Thread.h"
#include "Events.h"
#include "Log.h"
#include <algorithm>

namespace Z
{
//-------------------------------------------------------------------------------------------
class EvMng {
#if defined(_MSC_VER)
    EventManager _EvMng;
#else
    EventManager EvMng;
#endif
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

Event::Event(const Event &ev) {_label=ev._label;}

Event::~Event(){_label.clear();}

Event & Event::operator = (const Event &ev)
{
    _label = ev._label;
    return *this;
}

void Event::emitEvent()
{
    if(_label==StopThreadLabel) {
        ziblog(LOG::ERR, "Invalid emitEvent with label=StopThreadLabel (it's reserved). Call Stop() instead");
        return;
    }
    std::deque < Thread * > listeners;
//TODO - for now we use WaitForSingleObject that works both with win xp and win 7,
//but it's better (and we will do so as soon as possible) use the condition variables
//that is more efficient but it's supported only for win version newer than win vista.
//#if defined(_WIN32) && (_WIN32_WINNT < 0x0600)
    WaitForSingleObject(EventManager::_rwReq, INFINITE);
    while(EventManager::_pendingWriters) {
		ReleaseMutex(EventManager::_rwReq);
		WaitForSingleObject(EventManager::_noPendingWriters, INFINITE);
		WaitForSingleObject(EventManager::_rwReq, INFINITE);
	}
//#else
//    //TODO!!! use condition variables, supported from win vista onwards
//#endif
    EventManager::_pendingReaders++;
    ReleaseMutex(EventManager::_rwReq);
    if(EventManager::_evtReceivers.find(_label) != EventManager::_evtReceivers.end()) listeners = EventManager::_evtReceivers[_label];
    for(size_t i = 0; i < listeners.size(); ++i) listeners[i]->pushIn(clone());
    WaitForSingleObject(EventManager::_rwReq, INFINITE);
    if(!--EventManager::_pendingReaders) SetEvent(EventManager::_noPendingReaders);
    ReleaseMutex(EventManager::_rwReq);
}
//-------------------------------------------------------------------------------------------
//Thread
Thread::Thread()
{
    _th = 0;
    _set=false;
    _lockQueue = CreateMutex(NULL, FALSE, NULL);
    _lockSet = CreateMutex(NULL, FALSE, NULL);
    _thereIsAnEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
}

//VERY IMPORTANT! We couldn't embed the Stop() into the Thread base class destructor because this way the thread loop
//function would continue running until the base class destructor had not been called. As well known the base class
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

DWORD WINAPI Thread::ThreadLoop(LPVOID arg)
{
    ((Thread *)arg)->run();
    return 0;
}

void Thread::Start()
{
    WaitForSingleObject(_lockSet, INFINITE);
    if(!_set) {
        _th = CreateThread(NULL, 0, ThreadLoop, this, 0, NULL);
        _set=true;
    }
    ReleaseMutex(_lockSet);
}

void Thread::Stop()
{
    WaitForSingleObject(_lockSet, INFINITE);
    if(_set) {
        Event* stopThread = new StopThreadEvent();
        pushIn(stopThread);
        Join();
        _set=false;
        _th=0;
    }
    ReleaseMutex(_lockSet);
}

void Thread::Join()
{
    if(_th && _th != GetCurrentThread()) {
        DWORD ret = WaitForSingleObject(_th, INFINITE);
        if(ret == WAIT_FAILED) ziblog(LOG::WRN, "JOIN FAILED (error code = %ld)!", GetLastError());
    }
    else ziblog(LOG::WRN, "MUST START THREAD BEFORE JOIN!");
}

void Thread::StopReceivingAndDiscardReceived()
{
    EventManager::unmapReceiver(this);
    WaitForSingleObject(_lockQueue, INFINITE);
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
    ReleaseMutex(_lockQueue);
}

void Thread::register2Label(const std::string& label){EventManager::mapLabel2Receiver(label, this);}

void Thread::pushIn(Event* ev)
{
    WaitForSingleObject(_lockQueue, INFINITE);
    std::map < std::string, std::deque < Event* > >::iterator it = _eventQueue.find(ev->label());
    if(it == _eventQueue.end()) {
        std::deque < Event* > evts;
        it = _eventQueue.insert(it, std::pair < std::string, std::deque < Event* > > (ev->label(),  evts));
    }
    it->second.push_front(ev);
    _chronologicalLabelSequence.push_front(ev->label());
    SetEvent(_thereIsAnEvent);
    ReleaseMutex(_lockQueue);
}

Event* Thread::pullOut(int maxWaitMsec)
{
    WaitForSingleObject(_lockQueue, INFINITE);
    while(_eventQueue.empty()) {
        if(maxWaitMsec == NOWAIT) {
            ReleaseMutex(_lockQueue);
            return NULL;
        } else if(maxWaitMsec == WAIT4EVER) {
			ReleaseMutex(_lockQueue);
			WaitForSingleObject(_thereIsAnEvent, INFINITE);
			WaitForSingleObject(_lockQueue, INFINITE);
		}
        else {
			DWORD start, end;
			ReleaseMutex(_lockQueue);
			start = GetTickCount();
			if(WaitForSingleObject(_thereIsAnEvent, maxWaitMsec) == WAIT_TIMEOUT) return NULL;
			WaitForSingleObject(_lockQueue, INFINITE);
			end = GetTickCount();
			maxWaitMsec -= (end-start);
		}
    }
    std::deque < Event* > *evQ = &(_eventQueue.find(_chronologicalLabelSequence.back())->second);
    _chronologicalLabelSequence.pop_back();
    Event* ret =  evQ->back();
    evQ->pop_back();
    if(evQ->empty()) _eventQueue.erase(ret->label());
    ReleaseMutex(_lockQueue);
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
    labels.push_front(StopThreadLabel);
    if(maxWaitMsec == WAIT4EVER) {
        ziblog(LOG::WRN, "invalid wait forever for a specific bundle of events... set wait to default value of 1 sec to prevent starvation...");
        maxWaitMsec = 1000;
    }
    std::map < std::string, std::deque < Event* > >::iterator it;
	int timeout = maxWaitMsec;
	int end, start;
    WaitForSingleObject(_lockQueue, INFINITE);
    while(timeout>0){
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
                ReleaseMutex(_lockQueue);
                return ret;
            }
        }
		ReleaseMutex(_lockQueue);
		start = GetTickCount();
		if(WaitForSingleObject(_thereIsAnEvent, timeout) == WAIT_TIMEOUT) return NULL;
		end = GetTickCount();
		WaitForSingleObject(_lockQueue, INFINITE);
		timeout -= (end-start);
    }
    ReleaseMutex(_lockQueue);
    return NULL;
}

bool Thread::alive()
{
    if(!_th) return false;
    DWORD isAlive = 0;
    GetExitCodeThread(_th, &isAlive);
    return isAlive==STILL_ACTIVE;
}

//-------------------------------------------------------------------------------------------
//AutonomousThread
void AutonomousThread::run(){while(!exit) this->singleLoopCycle();}

AutonomousThread::AutonomousThread()
{
    _lock = CreateMutex(NULL, FALSE, NULL);
    exit=true;
}

void AutonomousThread::Start()
{
    WaitForSingleObject(_lock, INFINITE);
    exit=false;
    Thread::Start();
    ReleaseMutex(_lock);
}

void AutonomousThread::Stop()
{
    WaitForSingleObject(_lock, INFINITE);
    exit=true;
    Join();
    ReleaseMutex(_lock);
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
HANDLE EventManager::_rwReq;
HANDLE EventManager::_noPendingWriters;
HANDLE EventManager::_noPendingReaders;
int EventManager::_pendingWriters;
int EventManager::_pendingReaders;

std::map < std::string, std::deque < Thread *> > EventManager::_evtReceivers;
HANDLE EventManager::_lockEvtReceivers;

EventManager::EventManager()
{
    _rwReq = CreateMutex(NULL, FALSE, NULL);
    _noPendingReaders = CreateEvent(NULL, FALSE, FALSE, NULL);
    _noPendingWriters = CreateEvent(NULL, FALSE, FALSE, NULL);
    _pendingWriters = _pendingReaders = 0;
    _lockEvtReceivers = CreateMutex(NULL, FALSE, NULL);
}

void EventManager::mapLabel2Receiver(const std::string& label, Thread *T)
{
    if(!label.length()) {
        ziblog(LOG::WRN, "INVALID EVENT NAME!");
        return;
    }
    WaitForSingleObject(_rwReq, INFINITE);
    while(_pendingReaders) {
		ReleaseMutex(_rwReq);
		WaitForSingleObject(_noPendingReaders, INFINITE);
		WaitForSingleObject(_rwReq, INFINITE);
	}
    _pendingWriters++;
    ReleaseMutex(_rwReq);
    WaitForSingleObject(_lockEvtReceivers, INFINITE);
    if(_evtReceivers.find(label) == _evtReceivers.end()){
        std::deque < Thread *> listeners;
        _evtReceivers.insert(std::pair <std::string, std::deque < Thread *> >(label,listeners));
    }
    std::deque < Thread *>* evtReceiverList = &_evtReceivers[label];

    std::deque < Thread *>::iterator itEvtReceiverList;
    for(itEvtReceiverList = evtReceiverList->begin(); itEvtReceiverList != evtReceiverList->end(); itEvtReceiverList++) {
        if(*itEvtReceiverList == T) {
            WaitForSingleObject(_rwReq, INFINITE);
            if(!--_pendingWriters) SetEvent(_noPendingWriters);
            ReleaseMutex(_rwReq);
            ReleaseMutex(_lockEvtReceivers);
            return;
        }
    }
    _evtReceivers[label].push_front(T);
	WaitForSingleObject(_rwReq, INFINITE);
    if(!--_pendingWriters) SetEvent(_noPendingWriters);
    ReleaseMutex(_rwReq);
    ReleaseMutex(_lockEvtReceivers);
}

void EventManager::unmapReceiver(Thread *T)
{
    WaitForSingleObject(_rwReq, INFINITE);
    while(_pendingReaders) {
		ReleaseMutex(_rwReq);
		WaitForSingleObject(_noPendingReaders, INFINITE);
		WaitForSingleObject(_rwReq, INFINITE);
	}
    _pendingWriters++;
    ReleaseMutex(_rwReq);
    WaitForSingleObject(_lockEvtReceivers, INFINITE);
    std::deque < Thread *>::iterator itThreadList;
    std::map < std::string, std::deque < Thread *> >::iterator itEvtRecvs = _evtReceivers.begin();
    while(itEvtRecvs != _evtReceivers.end()){
        itThreadList = std::remove(itEvtRecvs->second.begin(), itEvtRecvs->second.end(), T);
        itEvtRecvs->second.erase(itThreadList, itEvtRecvs->second.end());
        if(itEvtRecvs->second.empty()) _evtReceivers.erase(itEvtRecvs++);
        else itEvtRecvs++;
    }
    WaitForSingleObject(_rwReq, INFINITE);
    if(!--_pendingWriters) SetEvent(_noPendingWriters);
    ReleaseMutex(_rwReq);
    ReleaseMutex(_lockEvtReceivers);
}
//-------------------------------------------------------------------------------------------
}//namespace Z
