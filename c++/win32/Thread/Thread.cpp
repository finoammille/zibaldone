/*
 *
 * zibaldone - a C++/Java library for Thread, Timers and other Stuff
 *
 * Copyright (C) 2012  Antonio Buccino
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
        ziblog(LOG::ERR, "ERROR: an instance of Event cannot have empty label");
        _label = "sob";//assegno un nome per evitare crash.
    }
}

Event::Event(const Event &ev) {_label=ev._label;}

Event::~Event(){_label.clear();}

Event & Event::operator = (const Event &ev)
{
    //if(this == &ev) return *this; //self assignment check... non serve perche` Event e` astratta e non puo` essere quindi istanziata...
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
Thread::~Thread()
{
    if(alive()) {
        //se sono qui e il thread loop e` ancora presente c'e` una violazione delle regole
        //di utilizzo della classe Thread (il thread loop run deve essere obbligatoriamente
        //terminato - chiamando Stop o uscendo - prima di distruggere l'oggetto che incapsula
        //il thread). Emetto un log di errore e provo a fare un recupero disperato....
        ziblog(LOG::ERR, "Thread loop (run method) MUST BE END before destroying a Thread object");
        Stop();//pthread_kill usata con sig=0 dice se _th è vivo. In tal caso chiamo Stop()
    }
    StopReceivingAndDiscardReceived();
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

void Thread::Stop()
{
    WaitForSingleObject(_lockSet, INFINITE);
    if(_set) {
        Event* stopThread = new StopThreadEvent();
        pushIn(stopThread);
        Join();//N.B.: attenzione a non fare confusione! non sto facendo "autojoin" la classe incapsula i dati ma il thread gira indipendentemente!
        _set=false;//serve per permettere di fare nuovamente Start (ovvero ridare vita al thread che si appoggia alla
                   //classe) permettendo il riutilizzo dell'oggetto che incapsula il thread (se non viene distrutto!)
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
    EventManager::unmapReceiver(this);//rimuovo la registrazione del thread da EventManager
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
        ziblog(LOG::WRN, "invalid wait forever for a specific bundle of events... set wait to default value of 1 sec to prevent starvation...");
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

bool Thread::alive()
{
    if(!_th) return false;
    DWORD isAlive = 0;
    GetExitCodeThread(_th, &isAlive);
    return isAlive==STILL_ACTIVE;
}

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
    /*
    26/11/14 metodo kill() eliminato: e` troppo pericoloso e non deve servire mai!
    kill();//non servirebbe. Diventa necessaria solo se viene fornita una singleLoopCycle fatta di un ciclo infinito, In tal caso
           //anche mettere exit=true non fa terminare il thread che continuerebbe a girare a causa del ciclo infinito. L'unica
           //soluzione diventa allora la kill, che non crea nessun problema se il thread e` gia` terminato correttamente (ovvero
           //se e` stata rispettata la regola di fornire la funzione che implementa un singolo ciclo del thread e non il loop
           //infinito!
    */
    ReleaseMutex(_lock);
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

//EventManager::~EventManager(){_evtReceivers.clear();}
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
