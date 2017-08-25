/*
 *
 * zibaldone - a C++/Java library for Thread, Timers and other Stuff
 *
 * http://sourceforge.net/projects/zibaldone/
 *
 * version 3.2.0, February 14th, 2016
 *
 * Copyright (C) 2012  bucc4neer (bucc4neer@users.sourceforge.net)
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
 * bucc4neer bucc4neer@users.sourceforge.net
 *
 */

#include "Thread.h"
#include "Events.h"
#include "Log.h"
#include <algorithm>

namespace Z
{
//-------------------------------------------------------------------------------------------
class EvMng {//ha il solo scopo di chiamare il costruttore di EventManager che inizializza i mutex, condition
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
#ifdef _ZDEBUG
        ziblog(LOG::ERR, "ERROR: an instance of Event cannot have empty label");
#endif
        _label = "sob";//assegno un nome per evitare crash.
    }
}

Event::~Event(){_label.clear();}

void Event::emitEvent()
{
    if(_label==StopThreadLabel) {
#ifdef _ZDEBUG
        ziblog(LOG::ERR, "Invalid emitEvent with label=StopThreadLabel (it's reserved). Call Stop() instead");
#endif
        return;
    }
    std::deque < Thread * > listeners;
//TODO - per ora uso WaitForSingleObject che funziona sia con win xp che con win 7,
//in futuro farò modifica che usa le variabili condition, che rende il codice più
//efficiente, ma sono utilizzabili solo da win vista in poi....
//#if defined(_WIN32) && (_WIN32_WINNT < 0x0600)
    WaitForSingleObject(EventManager::_rwReq, INFINITE);
    while(EventManager::_pendingWriters) {//devo usare il while perchè putroppo win non è standard posix e non ho
										  //una primitiva come la pthread_cond_wait che atomicamente faccia lock
										  //del mutex sulla notifica di _noPendingWriters...
		ReleaseMutex(EventManager::_rwReq);
		WaitForSingleObject(EventManager::_noPendingWriters, INFINITE);
		WaitForSingleObject(EventManager::_rwReq, INFINITE);
	}
//#else
//    //TODO!!! usare condition variable, supportate da win vista in poi
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
//Anyway even if we call Stop() and immediately after we destroy the Thread object (explicitly with a delete or by exiting
//the scope) there is no issue in that since Stop() method does not exit until the thread has ended thanks to the Join()
//called inside Stop() after pushIn of StopThread!
Thread::~Thread()
{
#ifdef _ZDEBUG
	WaitForSingleObject(_lockSet, INFINITE);
if(_set) {
        std::cerr<<"Thread loop (run method) MUST BE END before destroying a Thread object\n";
        std::cerr<<"Remember that a Thread loop should terminate only processing a StopThreadEvent\n";
    }
    //ATTENZIONE! Non posso usare ziblog perche` se stessi facendo delete della _instance di log, la emitEvent chiamata 
    //dalla enqueueMessage dentro ziblog fa pushIn che fa lock di _lockSet già lockata causando un deadlock
    ReleaseMutex(_lockSet);
#endif
    Stop();//Non sto embeddando Stop nel distruttore (vedi commento sopra!). La chiamata a Stop fatta qui
           //serve solo come come "paracadute". Se tutto va come deve, entro in Stop, faccio lock di _lockSet,
           //controllo _set che e` false e esco. Se invece il thread sta erroneamente girando, lo stoppo.
           //compilando con il flag _ZDEBUG quest'ultimo caso e`sottolineato come errore!
    EventManager::unmapReceiver(this);//rimuovo la registrazione del thread da EventManager
    CloseHandle(_lockQueue);
    CloseHandle(_lockSet);
}

//NOTA su ThreadLoop: un metodo statico significa che ce n'e` una sola copia! Non che puo' essere chiamato solo in un thread!
//Quindi ognuno chiama la funzione e all'uscita dalla run() chiamata da ThreadLoop ritrova il SUO arg, ovvero il puntatore al
//allo stesso oggetto che conteneva quella particolare run())
DWORD WINAPI Thread::ThreadLoop(LPVOID arg)
{
    ((Thread *)arg)->run();
    return 0;
}

//NOTA IMPORTANTE: occorre garantire che il thread sia creato UNA VOLTA SOLA per ogni oggetto di tipo thread istanziato.
//in caso contrario infatti avrei più thread indipendenti che però condividerebbero gli stessi dati incapsulati nella
//classe associata al thread. Occorre quindi prevenire che una doppia chiamata a Start sullo stesso oggetto comporti
//la creazione di due thread (il thread va creato solo LA PRIMA VOLTA che viene chiamato il metodo Start().
void Thread::Start()
{
    WaitForSingleObject(_lockSet, INFINITE);
    if(!_set) {
        _th = CreateThread(NULL, 0, ThreadLoop, this, 0, NULL);
        _set=true;
    }
    ReleaseMutex(_lockSet);
}

#ifdef _ZDEBUG
void Thread::Start(const std::string& thread_Name)
{
    WaitForSingleObject(_lockSet, INFINITE);
    if(!_set) {
        _th = CreateThread(NULL, 0, ThreadLoop, this, 0, NULL);
        threadName=thread_Name;
        _set=true;
    }
    ReleaseMutex(_lockSet);
}
#endif

// OSSERVAZIONI:
// => Mi è successo di chiamare erroneamente Stop() dentro run(). Il risultato e` stato un deadlock! Peraltro difficile da individuare! 
//    Infatti Join esce subito, mentre la purgeRxQueue completa il danno rimuovendo tutti gli eventi, e quindi anche lo StopThreadEvent
//    dalla coda prima che la pullOut possa estrarlo dalla coda e quindi terminare il thread! (pullOut non può correttamente fare nulla 
//    perchè c'è il lock su _lockSet). Avevo anche pensato di far ritornare un bool a Join, in modo da rilevare questa situazione e 
//    magari non fare la purgeRxQueue in un caso come questo, ma alla fine ho deciso di non farlo perchè la cura sarebbe stata peggiore 
//    del male! Infatti il paradigma è che uscito dalla Stop il thread DEVE essere terminato! Se lo lascio ancora li c'è già un errore, 
//    perchè chi chiama Stop() confida sul fatto che può dover aspettare ma una volta che Stop ritorna, il thread è terminato. In 
//    definitiva la regola è quella di non chiamare Stop MAI dentro run, e se lo si fa l'errore è li e non si può recuperare commettendo 
//    un altro errore! E` meglio loggare in tutti i modi per rendere più facile rilevare l'errore anzichè tentare di correggerlo rendendo 
//    l'errore semplicemente più subdolo perchè più difficile da individuare!!!!
//
// => Normalmente nessuno dovrebbe chiamare Stop dopo Join (se esco da Join significa che il thread e` terminato e non occorre chiamare 
//    anche Stop). Tuttavia se anche dovesse succedere (sebbene si tratti di un errore concettuale) ne usciamo indenni! Infatti usciti 
//    da Join, il thread e` terminato e poiche` ho gia` chiamato join, _joinedOnce=true. Quando entro nella Stop() trovo ancora _set
//    a true, per cui metto StopThreadEvent in coda, pensando che sia gestito dal thread (che pero` e` gia` terminato....) ma 
//    fortunatamente quando faccio Join esco subito (grazie a _joinedOnce), metto _set a false, _th a zero, pulisco la coda ed esco!
//    Se chiamassi nuovamente Stop (nel distruttore per esempio) questa volta non farei nulla essendo _set=false.

// IMPORTANT: Stop() should never be called inside run()
// ::::::::::::::::::::::::::::::::::::::::::::::::
// IMPORTANTE: Stop() non DEVE MAI essere chiamata dentro run()
void Thread::Stop()
{
    WaitForSingleObject(_lockSet, INFINITE);
    if(_set) {
        WaitForSingleObject(_lockQueue, INFINITE);
        _eventQueue.insert(_eventQueue.end(), std::pair < std::string, std::deque < Event* > > (StopThreadLabel, (std::deque < Event* > (1, (new StopThreadEvent())))));
        _chronologicalLabelSequence.push_front(StopThreadLabel);
        SetEvent(_thereIsAnEvent);
        ReleaseMutex(_lockQueue);
        Join();//N.B.: attenzione a non fare confusione! non sto facendo "autojoin" la classe incapsula i dati ma il thread gira indipendentemente!
        _set=false;//serve per permettere di fare nuovamente Start (ovvero ridare vita al thread che si appoggia alla
                   //classe) permettendo il riutilizzo dell'oggetto che incapsula il thread (se non viene distrutto!)
        _th=0;
        purgeRxQueue();//ripulisco la coda di ricezione
    }
    ReleaseMutex(_lockSet);
}

// IMPORTANT: Join() should never be called inside run()
// ::::::::::::::::::::::::::::::::::::::::::::::::
// IMPORTANTE: Join() non DEVE MAI essere chiamata dentro run()
void Thread::Join()
{
    if(_th && _th != GetCurrentThread()) {
        DWORD ret = WaitForSingleObject(_th, INFINITE);
#ifdef _ZDEBUG
        if(ret == WAIT_FAILED) ziblog(LOG::WRN, "JOIN FAILED (error code = %ld)!", GetLastError());
#endif
    }
#ifdef _ZDEBUG
    else ziblog(LOG::WRN, "MUST START THREAD BEFORE JOIN!");
#endif
}

void Thread::purgeRxQueue()
{
    WaitForSingleObject(_lockQueue, INFINITE);
    std::map < std::string, std::deque < Event* > >::iterator it;
    std::deque < Event* > * evList = NULL;
    for(it = _eventQueue.begin(); it != _eventQueue.end(); it++){
        evList = &(it->second);
        for(size_t i=0; i<evList->size(); i++) {
            delete (*evList)[i];//chiamo il distruttore per ogni puntatore di ogni evento memorizzato in coda
            (*evList)[i]=NULL;
        }
        evList->clear();//ripulisco la coda degli eventi del thread
    }
    _eventQueue.clear();//ho ripulito gli eventi per ogni tipo, ora ripulisco i tipi (nomi degli eventi)
    _chronologicalLabelSequence.clear();//ripulisco _chronologicalLabelSequence
    ReleaseMutex(_lockQueue);
}

void Thread::register2Label(const std::string& label){EventManager::mapLabel2Receiver(label, this);}

void Thread::pushIn(Event* ev)
{
    WaitForSingleObject(_lockSet, INFINITE);
    if(_set) {
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
    ReleaseMutex(_lockSet);
}

Event* Thread::pullOut(int maxWaitMsec)
{
    WaitForSingleObject(_lockQueue, INFINITE);//accesso esclusivo alla coda...
    while(_eventQueue.empty()) {//se la coda e' vuota...
        if(maxWaitMsec == NOWAIT) {
            ReleaseMutex(_lockQueue);//rilascio l'esclusivita' di accesso sulla coda
            return NULL;//la coda e` vuota e il chiamante non vuole che si aspetti sulla coda. Restituisco un puntatore a NULL.
        } else if(maxWaitMsec == WAIT4EVER) {//...resto in attesa sulla variabile condition indefinitamente
			ReleaseMutex(_lockQueue);
			WaitForSingleObject(_thereIsAnEvent, INFINITE);
			WaitForSingleObject(_lockQueue, INFINITE);
		}
        else {//attesa sulla coda per un tempo finito = maxWaitMsec > 0
			DWORD start, end;
			ReleaseMutex(_lockQueue);
			start = GetTickCount();
            //TODO. la pthread_cond_timedwait non ha un equivalente in windows nelle versioni precedenti a vista (quindi
            //nemmeno in win xp). Sotto windows le variabili condition sono arrivate solo dopo vista (... no comment ....)
			if(WaitForSingleObject(_thereIsAnEvent, maxWaitMsec) == WAIT_TIMEOUT) return NULL;
			WaitForSingleObject(_lockQueue, INFINITE);
			end = GetTickCount();
			maxWaitMsec -= (end-start);//aggiorno maxWaitMsec nel caso dovessi ripetere il ciclo di attesa (corsa su _lockQueue e _eventQueue di nuovo empty)
		}
    }
    std::deque < Event* > *evQ = &(_eventQueue.find(_chronologicalLabelSequence.back())->second);/* non serve verificare che it !=
                                                                                                  * _eventQueue.end() perche' c'e'
                                                                                                  * sicuramente almeno una entry
                                                                                                  * corrispondente all'evento in coda.
                                                                                                  * Infatti se sono qui, vuol dire che
                                                                                                  * e` arrivato un evento (altrimenti
                                                                                                  * sarei uscito ritornando NULL al
                                                                                                  * termine dell'attesa sulla condition
                                                                                                  * variabile "_thereIsAnEvent"
                                                                                                  */
    _chronologicalLabelSequence.pop_back();//aggiorno _chronologicalLabelSequence
    Event* ret =  evQ->back();/* nota: non serve controllare la condizione evQ->empty(), infatti un evento c'e` sicuramente, se no sarei
                               * uscito per timeout oppure sarei ancoda sulla variabile condizione "_thereIsAnEvent"
                               */
    evQ->pop_back();//rimuovo dalla coda l'evento
    if(evQ->empty()) _eventQueue.erase(ret->label());/* se non ci sono piu' eventi di questo tipo, rimuovo la relativa lista dalla
                                                        * mappa della coda
                                                        */
    ReleaseMutex(_lockQueue);//rilascio l'esclusivita' di accesso sulla coda
    return ret;
}

Event* Thread::pullOut(const std::string& label, int maxWaitMsec)
{
    std::deque <std::string> labels;
    labels.push_front(label);
    return pullOut(labels, maxWaitMsec);
}

Event* Thread::pullOut(std::deque <std::string> labels, int maxWaitMsec)/* se decido di estrarre un evento di un certo tipo, devo anche
                                                                         * fornire un timeout, per evitare situazioni ambigue (se l'evento
                                                                         * non c'e', resto in attesa indefinita sinche' non arriva quello
                                                                         * specifico evento? Di fatto bloccando tutti? E se vieme richiesto
                                                                         * per errore un evento che non puo' arrivare resto li per sempre?
                                                                         */
{
    labels.push_front(StopThreadLabel);//come spiegato sopra, DEVO sempre e comunque gestire StopThreadLabel
    if(maxWaitMsec == WAIT4EVER) {
#ifdef _ZDEBUG
        ziblog(LOG::WRN, "invalid wait forever for a specific bundle of events... set wait to default value of 1 sec to prevent starvation...");
#endif
        maxWaitMsec = 1000;
    }
    std::map < std::string, std::deque < Event* > >::iterator it;
	int timeout = maxWaitMsec;
	int end, start;
    WaitForSingleObject(_lockQueue, INFINITE);
    while(timeout>0){
        for(size_t i=0; i<labels.size(); i++) {
            if((it = _eventQueue.find(labels[i])) != _eventQueue.end()) {/* ho trovato in coda un evento del tipo richiesto... non serve
                                                                          * nemmeno far partire il timeout!
                                                                          */
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

/*
08/02/16 eliminato metodo alive . Sulla carta qui forse non ci sarebbero stati problemi ma ho voluto mantenere il
parallelismo con la versione linux (quella principale) per cui ho rimosso il metodo alive anche dalla versione win
bool Thread::alive()
{
    if(!_th) return false;
    DWORD isAlive = 0;
    GetExitCodeThread(_th, &isAlive);
    return isAlive==STILL_ACTIVE;
}
*/

/*
26/11/14 metodo kill() eliminato: e` troppo pericoloso e non deve servire mai!
void Thread::kill()
{
    WaitForSingleObject(_lockSet, INFINITE);
    if(_set) {
        if(TerminateThread(_th, 0) == FALSE) ziblog(LOG::ERR, "could not stop thread (error code = %ld)", GetLastError());
        _set=false;//serve per permettere di fare nuovamente Start (ovvero ridare vita al thread che si appoggia alla
                   //classe) permettendo il riutilizzo dell'oggetto che incapsula il thread (se non viene distrutto!)
        _th=0;
    }
    ReleaseMutex(_lockSet);
}
*/

//-------------------------------------------------------------------------------------------
//AutonomousThread
void AutonomousThread::run(){while(!exit) this->singleLoopCycle();}

DWORD WINAPI AutonomousThread::ThreadLoop(LPVOID arg)
{
    ((AutonomousThread *)arg)->run();
    return 0;
}

AutonomousThread::AutonomousThread()
{
    _lockSet = CreateMutex(NULL, FALSE, NULL);
    _th = 0;
    _set=false;
    exit=true;//parto sullo Start(), non nel costruttore!
}

AutonomousThread::~AutonomousThread()
{
#ifdef _ZDEBUG
	WaitForSingleObject(_lockSet, INFINITE);
if(_set) {
        std::cerr<<"Thread loop (run method) MUST BE END before destroying a Thread object\n";
        std::cerr<<"Remember that a Thread loop should terminate only processing a StopThreadEvent\n";
    }
    //ATTENZIONE! Non posso usare ziblog perche` se stessi facendo delete della _instance di log, la emitEvent chiamata 
    //dalla enqueueMessage dentro ziblog fa pushIn che fa lock di _lockSet già lockata causando un deadlock
    ReleaseMutex(_lockSet);
#endif
    Stop();//Non sto embeddando Stop nel distruttore (vedi commento sopra!). La chiamata a Stop fatta qui
           //serve solo come come "paracadute". Se tutto va come deve, entro in Stop, faccio lock di _lockSet,
           //controllo _set che e` false e esco. Se invece il thread sta erroneamente girando, lo stoppo.
           //compilando con il flag _ZDEBUG quest'ultimo caso e`sottolineato come errore!
    CloseHandle(_lockSet);
}

void AutonomousThread::Start()
{
    WaitForSingleObject(_lockSet, INFINITE);
    exit=false;
    if(!_set) {
        _th = CreateThread(NULL, 0, ThreadLoop, this, 0, NULL);
        _set=true;
    }
    ReleaseMutex(_lockSet);
}

#ifdef _ZDEBUG
void AutonomousThread::Start(const std::string& thread_Name)
{
    WaitForSingleObject(_lockSet, INFINITE);
    if(!_set) {
        _th = CreateThread(NULL, 0, ThreadLoop, this, 0, NULL);
        threadName=thread_Name;
        _set=true;
    }
    ReleaseMutex(_lockSet);
}
#endif

// NOTA IMPORTANTE! Mi è successo di chiamare erroneamente Stop() dentro run(). Il risultato e` stato un deadlock! Peraltro difficile da individuare!
// Infatti Join esce subito mentre la purgeRxQueue completa il danno rimuovendo tutti gli eventi, e quindi anche lo StopThreadEvent dalla coda prima 
// che la pullOut possa estrarlo dalla coda e quindi terminare il thread! (pullOut non può correttamente fare nulla perchè c'è il lock su _lockSet). 
// Avevo anche pensato di far ritornare un bool a Join, in modo da rilevare questa situazione e magari non fare la purgeRxQueue in un caso come questo, 
// ma alla fine ho deciso di non farlo perchè la cura sarebbe stata peggiore del male! Infatti il paradigma è che uscito dalla Stop il thread DEVE 
// essere terminato! Se lo lascio ancora li c'è già un errore, perchè chi chiama Stop() confida sul fatto che può dover aspettare ma una volta che Stop 
// ritorna, il thread è terminato. In definitiva la regola è quella di non chiamare Stop MAI dentro run, e se lo si fa l'errore è li e non si può 
// recuperare commettendo un altro errore! E` meglio loggare in tutti i modi per rendere più facile rilevare l'errore anzichè tentare di correggerlo 
// rendendo l'errore semplicemente più subdolo perchè più difficile da individuare!!!!

// IMPORTANT: Stop() should never be called inside run()
// ::::::::::::::::::::::::::::::::::::::::::::::::
// IMPORTANTE: Stop() non DEVE MAI essere chiamata dentro run()
void AutonomousThread::Stop()
{
    WaitForSingleObject(_lockSet, INFINITE);
    if(_set) {
        exit=true;
        Join();//N.B.: attenzione a non fare confusione! non sto facendo "autojoin" i
               //la classe incapsula i dati ma il thread gira indipendentemente!
        _set=false;//serve per permettere di fare nuovamente Start (ovvero ridare vita al thread che si appoggia alla
                   //classe) permettendo il riutilizzo dell'oggetto che incapsula il thread (se non viene distrutto!)
        _th=0;
    }
    ReleaseMutex(_lockSet);
}

void AutonomousThread::Join()
{
    if(_th && _th != GetCurrentThread()) {
        DWORD ret = WaitForSingleObject(_th, INFINITE);
        if(ret == WAIT_FAILED) ziblog(LOG::WRN, "JOIN FAILED (error code = %ld)!", GetLastError());
    }
    else ziblog(LOG::WRN, "MUST START THREAD BEFORE JOIN!");
}

/*
08/02/16 eliminato metodo alive . Sulla carta qui forse non ci sarebbero stati problemi ma ho voluto mantenere il
parallelismo con la versione linux (quella principale) per cui ho rimosso il metodo alive anche dalla versione win
bool AutonomousThread::alive()
{
    if(!_th) return false;
    DWORD isAlive = 0;
    GetExitCodeThread(_th, &isAlive);
    return isAlive==STILL_ACTIVE;
}
*/

//-------------------------------------------------------------------------------------------
//SameThreadAddressSpaceEventReceiver
SameThreadAddressSpaceEventReceiver::SameThreadAddressSpaceEventReceiver()
{
    _set=true;
}

SameThreadAddressSpaceEventReceiver::~SameThreadAddressSpaceEventReceiver()
{
    _set=false;
}

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
#ifdef _ZDEBUG
        ziblog(LOG::WRN, "INVALID EVENT NAME!");
#endif
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
    if(_evtReceivers.find(label) == _evtReceivers.end()){//se il tipo di evento non era ancora stato registrato
        std::deque < Thread *> listeners;
        _evtReceivers.insert(std::pair <std::string, std::deque < Thread *> >(label,listeners));
    }
    std::deque < Thread *>* evtReceiverList = &_evtReceivers[label];/* lista dei tread registrati per ricevere l'evento "label"
                                                                     * (nel caso non ci fosse stato nessun thread registrato, ora
                                                                     * c'e una lista vuota)
                                                                     */

    std::deque < Thread *>::iterator itEvtReceiverList;
    for(itEvtReceiverList = evtReceiverList->begin(); itEvtReceiverList != evtReceiverList->end(); itEvtReceiverList++) {
        if(*itEvtReceiverList == T) {//qualcuno sta facendo per errore una doppia registrazione su uno stesso evento
            WaitForSingleObject(_rwReq, INFINITE);
            if(!--_pendingWriters) SetEvent(_noPendingWriters);
            ReleaseMutex(_rwReq);
            ReleaseMutex(_lockEvtReceivers);
            return;
        }
    }//se sono qui, il ricevitore non e' ancora registrato. Lo registro e rilascio il semaforo.
    _evtReceivers[label].push_front(T);//registro un nuovo gestore per questo tipo di evento
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

/*
EventManager::~EventManager()
{
    _evtReceivers.clear();
    CloseHandle(_rwReq);
    CloseHandle(_lockEvtReceivers);
}
*/
/*
NOTA IMPORTANTE! Il costruttore di EventManager serve ad inizializzare i mutex, condition, ...
e viene invocato tramite l'oggetto "fake" _evMng. Quando il programma esce, vengono distrutte
tutte le variabili static, tra cui i membri di EventManager e in particolare _evtReceivers.
Se tutto viene fatto correttamente (ovvero i vari thread sono stoppati correttamente e non
ci sono memory leak riguardanti puntatori a thread) la _evtReceivers viene ripulita da
unmapReceiver per ogni thread. Se pero` dovesse succedere di dimenticarsi di fare delete di
un thread allocato in heap, non verrebbe chiamata la relativa unmapReceiver con il risultato
che all'uscita dal programma si ha una doppia free! In definitiva il distruttore di EventManager
e` sovrabbondante (all'uscita dal programma come detto le variabili statiche vengono distrutte)
e può provocare crash in caso di leak riguardante il puntatore ad un thread. Poichè la cosa
riguarda l'uscita dal programma, i leak eventuali vengono comunque "risolti" dal sistema
operativo che libera tutta la memoria associata al processo, per cui in definitiva
conviene non avere il distruttire di EventManager che può provocare un crash in uscita.
Ho lasciato comunque il distruttore commentato perche` puo` essere utile scommentarlo
ogni tanto per verificare se ci sono puntatori a thread non deallocati!
*/
//-------------------------------------------------------------------------------------------
}//namespace Z
