/*
 *
 * zibaldone - a C++ library for Thread, Timers and other Stuff
 *
 * Copyright (C) 2012  Antonio Buccino
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
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
#include "Log.h"
#include <algorithm>

const std::string DataEvent::DataEventName[EndOfList] = {"TxDataEvent", "RxDataEvent"};
const std::string ErrorEvent::ErrorEventName = "ErrorEvent";
const std::string StopThreadEvent::StopThreadEventName = "StopThreadEvent";
static EventManager EvMng;//serve per chiamare il costruttore di eventManger che inizializza le variabili (semafori, variabili condition, ...) per la corretta gestione degli eventi.

//-------------------------------------------------------------------------------------------
//Event
Event::Event(std::string eventName):_eventName(eventName){}

std::string Event::eventName(){return _eventName;}

void Event::emitEvent()
{
    std::deque < Thread * > listeners;
    pthread_mutex_lock(&EventManager::_rwReq);
    if(EventManager::_pendingWriters) pthread_cond_wait(&EventManager::_noPendingWriters, &EventManager::_rwReq);
    EventManager::_pendingReaders++;
    pthread_mutex_unlock(&EventManager::_rwReq);
    if(EventManager::_evtReceivers.find(_eventName) != EventManager::_evtReceivers.end()) listeners = EventManager::_evtReceivers[_eventName];
    for(size_t i = 0; i < listeners.size(); ++i) listeners[i]->pushIn(clone());
    pthread_mutex_lock(&EventManager::_rwReq);
    if(!--EventManager::_pendingReaders) pthread_cond_signal(&EventManager::_noPendingReaders);
    pthread_mutex_unlock(&EventManager::_rwReq);
}

void Event::sendEventTo(Thread* target){target->pushIn(clone());}
//-------------------------------------------------------------------------------------------
//THREAD
Thread::Thread()
{
    _th = 0;
    pthread_mutex_init(&_lockQueue, NULL);
    pthread_cond_init(&_thereIsAnEvent, NULL);
}

Thread::~Thread(){StopReceivingAndDiscardReceived();}

void * Thread::ThreadLoop(void * arg)
{
    ((Thread *)arg)->run();
    return NULL;
}

void Thread::Start(){pthread_create (&_th, NULL, ThreadLoop, this);}

//NOTA IMPORTANTE: Join() non deve mai essere chiamata dentro run(). Infatti Join() chiama pthread_join che si blocca in attesa che il thread termini. Tuttavia questo
//non potrà mai terminare se è bloccato in attesa! Analogo discorso vale per Stop() dato che chiama al suo interno Join(). Effettivamente, è un'evidente contraddizione
//chiamare Join() di se` stesso dentro run(), tuttavia potrebbe invece venire la tentazione di chiamare Stop(). Il modo giusto per terminare un thread dentro la run()
//stessa di questo è uscire dal loop della run() dopo aver eventualmente liberato l'eventuale memoria allocata in heap.

void Thread::Stop()
{
    StopThreadEvent stopThread;
    stopThread.sendEventTo(this);
    Join();//aspetto che sia effettivamente terminato prima di uscire (si tratta cmq di pochi msec, ma fondamentali perche' il chiamante abbia la certezza del risultato!)
}

void Thread::Join()
{
    if(_th) pthread_join(_th, NULL);
    else ziblog("MUST START THREAD BEFORE JOIN!");
}

void Thread::StopReceivingAndDiscardReceived()
{
    EventManager::unmapReceiver(this);//rimuovo la registrazione del thread da EventManager
    pthread_mutex_lock(&_lockQueue);
    std::map < std::string, std::deque < Event* > >::iterator it;
    std::deque < Event* > * evList = NULL;
    for(it = _eventQueue.begin(); it != _eventQueue.end(); it++){
        evList = &(it->second);
        for(size_t i=0; i<evList->size(); i++) delete (*evList)[i];//chiamo il distruttore per ogni puntatore di ogni evento memorizzato in coda
        evList->clear();//ripulisco la coda degli eventi del thread
    }
    _eventQueue.clear();//ho ripulito gli eventi per ogni tipo, ora ripulisco i tipi (nomi degli eventi)
    pthread_mutex_unlock(&_lockQueue);
}

void Thread::register2Event(std::string eventName){EventManager::mapEvent2Receiver(eventName, this);}

void Thread::pushIn(Event* ev)
{
    pthread_mutex_lock(&_lockQueue);
    std::map < std::string, std::deque < Event* > >::iterator it = _eventQueue.find(ev->eventName());
    if(it == _eventQueue.end()) {
        std::deque < Event* > evts;
        it = _eventQueue.insert(it, std::pair < std::string, std::deque < Event* > > (ev->eventName(),  evts));
    }
    it->second.push_front(ev);
    _chronologicalEventNameSequence.push_front(ev->eventName());
    pthread_cond_signal(&_thereIsAnEvent);
    pthread_mutex_unlock(&_lockQueue);
}

Event* Thread::pullOut(int maxWaitMsec)
{
    pthread_mutex_lock(&_lockQueue);//accesso esclusivo alla coda...
    if (_eventQueue.empty()) {//se la coda e' vuota...
        if(maxWaitMsec == NOWAIT) {
            pthread_mutex_unlock(&_lockQueue);//rilascio l'esclusivita' di accesso sulla coda
            return NULL;//la coda e` vuota e il chiamante non vuole che si aspetti sulla coda. Restituisco un puntatore a NULL.
        } else if(maxWaitMsec == WAIT4EVER) pthread_cond_wait(&_thereIsAnEvent, &_lockQueue);//...resto in attesa sulla variabile condition indefinitamente
        else {//attesa sulla coda per un tempo finito = maxWaitMsec > 0
            struct timespec ts;//compilo ts
            int s = maxWaitMsec/1000;
            int ms = maxWaitMsec%1000;
            struct timeval tp;
            gettimeofday(&tp, NULL);
            ts.tv_sec  = tp.tv_sec + s;
            ts.tv_nsec = tp.tv_usec * 1000 + ms * 1000000;
            if(pthread_cond_timedwait(&_thereIsAnEvent, &_lockQueue, &ts)) {/* ...resto in attesa sulla variabile condition per l'intervallo
                                                                             * massimo prestabilito oppure...
                                                                             */
                pthread_mutex_unlock(&_lockQueue);//rilascio l'esclusivita' di accesso sulla coda
                return NULL;//timeout di attesa senza che siano stati ricevuti eventi sulla coda. Restituisco un puntatore a NULL.
            }
        }
    }
    std::deque < Event* > *evQ = &(_eventQueue.find(_chronologicalEventNameSequence.back())->second);/* non serve verificare che it !=
                                                                                                      * _eventQueue.end() perche' c'e'
                                                                                                      * sicuramente almeno una entry
                                                                                                      * corrispondente all'evento in coda.
                                                                                                      * Infatti se sono qui, vuol dire che
                                                                                                      * e` arrivato un evento (altrimenti
                                                                                                      * sarei uscito ritornando NULL al
                                                                                                      * termine dell'attesa sulla condition
                                                                                                      * variabile "_thereIsAnEvent"
                                                                                                      */
    _chronologicalEventNameSequence.pop_back();//aggiorno _chronologicalEventNameSequence
    Event* ret =  evQ->back();/* nota: non serve controllare la condizione evQ->empty(), infatti un evento c'e` sicuramente, se no sarei
                               * uscito per timeout oppure sarei ancoda sulla variabile condizione "_thereIsAnEvent"
                               */
    evQ->pop_back();//rimuovo dalla coda l'evento
    if(evQ->empty()) _eventQueue.erase(ret->eventName());/* se non ci sono piu' eventi di questo tipo, rimuovo la relativa lista dalla
                                                          * mappa della coda
                                                          */
    pthread_mutex_unlock(&_lockQueue);//rilascio l'esclusivita' di accesso sulla coda
    return ret;
}

Event* Thread::pullOut(std::string eventName, int maxWaitMsec)
{
    std::deque <std::string> eventNames;
    eventNames.push_front(eventName);
    return pullOut(eventNames, maxWaitMsec);
}

Event* Thread::pullOut(std::deque <std::string> eventNames, int maxWaitMsec)/* se decido di estrarre un evento di un certo tipo, devo anche
                                                                             * fornire un timeout, per evitare situazioni ambigue (se l'evento
                                                                             * non c'e', resto in attesa indefinita sinche' non arriva quello
                                                                             * specifico evento? Di fatto bloccando tutti? E se vieme richiesto
                                                                             * per errore un evento che non puo' arrivare resto li per sempre?
                                                                             */
{
    eventNames.push_front(StopThreadEvent::StopThreadEventName);//come spiegato sopra, DEVO sempre e comunque gestire StopThreadEventName
    if(maxWaitMsec == WAIT4EVER) {
        ziblog("invalid wait forever for a specific bundle of events... set wait to default value of 1 sec to prevent starvation...");
        maxWaitMsec = 1000;
    }
    std::map < std::string, std::deque < Event* > >::iterator it;
    bool timeout = false;
    struct timespec ts;
    int s = maxWaitMsec/1000;
    int ms = maxWaitMsec%1000;
    struct timeval tp;
    gettimeofday(&tp, NULL);
    ts.tv_sec  = tp.tv_sec + s;
    ts.tv_nsec = tp.tv_usec*1000 + ms*1000000;
    pthread_mutex_lock(&_lockQueue);
    while (!timeout){
        for(size_t i=0; i<eventNames.size(); i++) {
            if((it = _eventQueue.find(eventNames[i])) != _eventQueue.end()) {/* ho trovato in coda un evento del tipo richiesto... non serve
                                                                              * nemmeno far partire il timeout!
                                                                              */
                Event* ret = (it->second).back();
                (it->second).pop_back();
                if((it->second).empty()) _eventQueue.erase(ret->eventName());
                for(size_t j = 0; j<_chronologicalEventNameSequence.size(); j++) {
                    if(_chronologicalEventNameSequence[j]==eventNames[i]) {
                        _chronologicalEventNameSequence.erase(_chronologicalEventNameSequence.begin()+j);
                        break;
                    }
                }
                pthread_mutex_unlock(&_lockQueue);
                return ret;
            }
        }
        if(pthread_cond_timedwait(&_thereIsAnEvent, &_lockQueue, &ts)) {
            pthread_mutex_unlock(&_lockQueue);
            return NULL;//timeout scaduto senza aver ricevuto l'evento voluto. Ritorno NULL.
        }
        gettimeofday(&tp, NULL);/* se sono qui, e' arrivato un evento sulla coda. Resta da controllare se e' quello voluto,
                                 * e in caso contrario riarmare l'attesa per il tempo restante
                                 */
        timeout = (tp.tv_sec > ts.tv_sec) || ((tp.tv_sec == ts.tv_sec) && (tp.tv_usec*1000 >= ts.tv_nsec));
    }
    pthread_mutex_unlock(&_lockQueue);
    return NULL;/* ho ricevuto qualcosa ma tale ricezione e' avvenuta proprio allo scadere di maxWaitTimeout. In tal caso, potrebbe succedere
                 * che il tempo residuo ts della pthread_cond_timedwait sia talmente esiguo da scadere quando l'esecuzione arriva alla chiamata
                 * di gettimeofday per cui timeout = true. In tal caso tutto va esattamente come se fosse scaduto il timeout prima della ricezione
                 * dell'evento atteso, solo che mi trovo fuori da "while (!timeout) anziche', come normalmente accadrebbe in questi casi, dentro
                 * l'if(pthread_cond_timedwait(&_thereIsAnEvent, &_lockQueue, &ts)). Occorre allora rilasciare il semaforo e ritornare NULL 
                 * esattamente come se l'evento atteso fosse arrivato troppo in ritardo (non e' cosi' in realta', ma e' esattamente come se fosse cosi',
                 * e questo e' l'unico modo di trattare l'evento mantenendo la coerenza rispetto al chiamante. D'altronde la colpa e' dovuta al tempo 
                 * materiale per eseguire le istruzioni tra la pthread_cond_timedwait e la gettimeofday: e' il "bello" della programmazione real-time!)
                 */
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

void EventManager::mapEvent2Receiver(std::string eventName, Thread *T)
{
    if(!eventName.length()) {
        ziblog("INVALID EVENT NAME!");
        return;
    }
    pthread_mutex_lock(&_rwReq);
    if(_pendingReaders) pthread_cond_wait(&_noPendingReaders, &_rwReq);
    _pendingWriters++;
    pthread_mutex_unlock(&_rwReq);
    pthread_mutex_lock(&_lockEvtReceivers);
    if(_evtReceivers.find(eventName) == _evtReceivers.end()){//se il tipo di evento non era ancora stato registrato
        std::deque < Thread *> listeners;
        _evtReceivers.insert(std::pair <std::string, std::deque < Thread *> >(eventName,listeners));
    }
    std::deque < Thread *>* evtReceiverList = &_evtReceivers[eventName];/* lista dei tread registrati per ricevere l'evento "eventName"
                                                                         * (nel caso non ci fosse stato nessun thread registrato, ora
                                                                         * c'e una lista vuota)
                                                                         */
    
    std::deque < Thread *>::iterator itEvtReceiverList;
    for(itEvtReceiverList = evtReceiverList->begin(); itEvtReceiverList != evtReceiverList->end(); itEvtReceiverList++) {
        if(*itEvtReceiverList == T) {//qualcuno sta facendo per errore una doppia registrazione su uno stesso evento
            pthread_mutex_lock(&_rwReq);
            if(!--_pendingWriters) pthread_cond_signal(&_noPendingWriters);
            pthread_mutex_unlock(&_rwReq);
            pthread_mutex_unlock(&_lockEvtReceivers);
            return;
        }
    }//se sono qui, il ricevitore non e' ancora registrato. Lo registro e rilascio il semaforo.
    _evtReceivers[eventName].push_front(T);//registro un nuovo gestore per questo tipo di evento
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

EventManager::~EventManager(){_evtReceivers.clear();}
