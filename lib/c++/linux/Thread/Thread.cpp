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
class EvMng{//ha il solo scopo di chiamare il costruttore di EventManager che inizializza i mutex, condition, ... EvMng e` privato perche` non deve essere accessibile.
    EventManager EvMng;
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
    pthread_mutex_lock(&EventManager::_rwReq);
    //here I use "while(EventManager::_pendingWriters)" instead of "if(EventManager::_pendingWriters)" (the natural logic
    //choice would be "if") to handle potential (extremely rare) spurious wakeup (see POSIX-pthread_cond_wait specification)
    while(EventManager::_pendingWriters) pthread_cond_wait(&EventManager::_noPendingWriters, &EventManager::_rwReq);
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
    _joinedOnce=false;
    pthread_mutex_init(&_lockQueue, NULL);
    pthread_mutex_init(&_lockSet, NULL);
    pthread_mutex_init(&_lockJoin, NULL);
    pthread_cond_init(&_thereIsAnEvent, NULL);
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
    pthread_mutex_lock(&_lockSet);
    if(_set) {
        std::cerr<<"Thread loop (run method) MUST BE END before destroying a Thread object\n";
        std::cerr<<"Remember that a Thread loop should terminate only processing a StopThreadEvent\n";
    }
    //ATTENZIONE! Non posso usare ziblog perche` se stessi facendo delete della _instance di log, la emitEvent chiamata 
    //dalla enqueueMessage dentro ziblog fa pushIn che fa lock di _lockSet già lockata causando un deadlock
    pthread_mutex_unlock(&_lockSet);
#endif
    Stop();//Non sto embeddando Stop nel distruttore (vedi commento sopra!). La chiamata a Stop fatta qui
           //serve solo come come "paracadute". Se tutto va come deve, entro in Stop, faccio lock di _lockSet,
           //controllo _set che e` false e esco. Se invece il thread sta erroneamente girando, lo stoppo.
           //compilando con il flag _ZDEBUG quest'ultimo caso e`sottolineato come errore!
    EventManager::unmapReceiver(this);//rimuovo la registrazione del thread da EventManager
    pthread_mutex_destroy(&_lockQueue);
    pthread_mutex_destroy(&_lockSet);
    pthread_mutex_destroy(&_lockJoin);
}

//NOTA su ThreadLoop: un metodo statico significa che ce n'e` una sola copia! Non che puo' essere chiamato solo in un thread!
//Quindi ognuno chiama la funzione e all'uscita dalla run() chiamata da ThreadLoop ritrova il SUO arg, ovvero il puntatore al
//allo stesso oggetto che conteneva quella particolare run())
void * Thread::ThreadLoop(void * arg)
{
    ((Thread *)arg)->run();
    return NULL;
}

//NOTA IMPORTANTE: occorre garantire che il thread sia creato UNA VOLTA SOLA per ogni oggetto di tipo thread istanziato.
//in caso contrario infatti avrei più thread indipendenti che però condividerebbero gli stessi dati incapsulati nella
//classe associata al thread. Occorre quindi prevenire che una doppia chiamata a Start sullo stesso oggetto comporti
//la creazione di due thread (il thread va creato solo LA PRIMA VOLTA che viene chiamato il metodo Start().
void Thread::Start()
{
    pthread_mutex_lock(&_lockSet);
    if(!_set) {
        pthread_mutex_lock(&_lockJoin);
        pthread_create (&_th, NULL, ThreadLoop, this);
        _joinedOnce=false;
        pthread_mutex_unlock(&_lockJoin);
        _set=true;
    }
    pthread_mutex_unlock(&_lockSet);
}

#ifndef arm //pthread_setname_np e` utilizzabile con glibc>2.12 nella nostra toolchain la versione e` inferiore...
#ifdef _ZDEBUG
void Thread::Start(const std::string& threadName)
{
    pthread_mutex_lock(&_lockSet);
    if(!_set) {
        pthread_mutex_lock(&_lockJoin);
        pthread_create (&_th, NULL, ThreadLoop, this); 
        if(pthread_setname_np(_th, threadName.c_str())!=0) ziblog(LOG::ERR, "pthread_setname_np (%s, %s)", threadName.c_str(), strerror(errno));
        _joinedOnce=false;
        pthread_mutex_unlock(&_lockJoin);
        _set=true;
    }
    pthread_mutex_unlock(&_lockSet);
}
#endif
#endif

// OSSERVAZIONI:
// => Mi è successo di chiamare erroneamente Stop() dentro run(). Il risultato e` stato un deadlock! Peraltro difficile da individuare! 
//    Infatti Join esce subito (ret=EDEADLK), mentre la purgeRxQueue completa il danno rimuovendo tutti gli eventi, e quindi anche lo 
//    StopThreadEvent dalla coda prima che la pullOut possa estrarlo dalla coda e quindi terminare il thread! (pullOut non può 
//    correttamente fare nulla perchè c'è il lock su _lockSet). Avevo anche pensato di far ritornare un bool a Join, in modo da rilevare 
//    questa situazione e magari non fare la purgeRxQueue in un caso come questo, ma alla fine ho deciso di non farlo perchè la cura 
//    sarebbe stata peggiore del male! Infatti il paradigma è che uscito dalla Stop il thread DEVE essere terminato! Se lo lascio ancora 
//    li c'è già un errore, perchè chi chiama Stop() confida sul fatto che può dover aspettare ma una volta che Stop ritorna, il thread 
//    è terminato. In definitiva la regola è quella di non chiamare Stop MAI dentro run, e se lo si fa l'errore è li e non si può 
//    recuperare commettendo un altro errore! E` meglio loggare in tutti i modi per rendere più facile rilevare l'errore anzichè tentare 
//    di correggerlo rendendo l'errore semplicemente più subdolo perchè più difficile da individuare!!!!
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
    pthread_mutex_lock(&_lockSet);
    if(_set) {
        pthread_mutex_lock(&_lockQueue);
        _eventQueue.insert(_eventQueue.end(), std::pair < std::string, std::deque < Event* > > (StopThreadLabel, (std::deque < Event* > (1, (new StopThreadEvent())))));
        _chronologicalLabelSequence.push_front(StopThreadLabel);
        pthread_cond_signal(&_thereIsAnEvent);
        pthread_mutex_unlock(&_lockQueue);
        Join();//N.B.: attenzione a non fare confusione! non sto facendo "autojoin" la classe incapsula i dati ma il thread gira indipendentemente!
        _set=false;//serve per permettere di fare nuovamente Start (ovvero ridare vita al thread che si appoggia alla
                   //classe) permettendo il riutilizzo dell'oggetto che incapsula il thread (se non viene distrutto!)
        _th=0;
        purgeRxQueue();//ripulisco la coda di ricezione
    }
    pthread_mutex_unlock(&_lockSet);
}

// IMPORTANT: Join() should never be called inside run()
// ::::::::::::::::::::::::::::::::::::::::::::::::
// IMPORTANTE: Join() non DEVE MAI essere chiamata dentro run()
void Thread::Join()
{
    pthread_mutex_lock(&_lockJoin);
    if(!_joinedOnce) {
        if(_th) {
            //se _th=0, la pthread_join fa segmentation fault. Non puo` succedere per una corsa con Stop perche` anche
            //in Stop prima di mettere _th=0 viene chiamata Join che si fermerebbe sulla _lockJoin. L'unica corsa 
            //possibile e` se Join fosse chiamata insieme a Start... in tal caso se arriva prima Start la join fa
            //il suo lavoro (thread e` partito) se al contrario arriva prima la join trova _th ancora = zero e esce
            //(corretto: il thread non e` ancora partito). Quindi non servono precauzioni! Se qualcuno invece potesse
            //fare il contrario (mettermi _th a zero dopo che ho controllato con l'if e deciso che e` diverso da zero)
            //sarebbero dolori!!! (segmentation fault). Per proteggermi da questa situazione potrei copiare il valore
            //di _th in una variabile locale prima di fare if ma dopo il lock di _lockJoin: in questo caso non avrei
            //la garanzia di avere l'ultimo valore aggiornato ma avrei la garanzia che nessuno possa cambiarmi il
            //valore di _th sotto il culo dopo averlo controllato con l'if... Tuttavia poiche` ho analizzato la questione
            //e deciso che nessuno puo` mettermi _th a zero (chi potrebbe, ovvero Stop, come detto non ce la fa perche`
            //si blocca su join.. se _th fosse messo a zero in Stop prima di chiamare Join potrebbe succedere, ma cosi`
            //non e` possibile) lascio tutto cosi`: mi basta questo commento nel caso mi venissero dubbi in futuro!
            int ret = pthread_join(_th, NULL);
            _joinedOnce=true;
#ifdef _ZDEBUG
            if(ret == EDEADLK) ziblog(LOG::ERR, "thread cannot join himself");
#endif
        }
    }
    pthread_mutex_unlock(&_lockJoin);
}

void Thread::purgeRxQueue()
{
    pthread_mutex_lock(&_lockQueue);
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
    pthread_mutex_unlock(&_lockQueue);
}

void Thread::register2Label(const std::string& label){EventManager::mapLabel2Receiver(label, this);}

void Thread::pushIn(Event* ev)
{
    pthread_mutex_lock(&_lockSet);
    if(_set) {
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
    pthread_mutex_unlock(&_lockSet);
}

Event* Thread::pullOut(int maxWaitMsec)
{
    pthread_mutex_lock(&_lockQueue);//accesso esclusivo alla coda...
    if (_eventQueue.empty()) {//se la coda e' vuota...
        if(maxWaitMsec == NOWAIT) {
            pthread_mutex_unlock(&_lockQueue);//rilascio l'esclusivita' di accesso sulla coda
            return NULL;//la coda e` vuota e il chiamante non vuole che si aspetti sulla coda. Restituisco un puntatore a NULL.
        } else if(maxWaitMsec == WAIT4EVER) {
            while(_eventQueue.empty()) pthread_cond_wait(&_thereIsAnEvent, &_lockQueue);
            //resto in attesa sulla variabile condition indefinitamente.
            //Il while serve per gestire eventuali spurious wakeup
        } else {//attesa sulla coda per un tempo finito = maxWaitMsec > 0
            time_t s = maxWaitMsec/1000;
            long ns = (maxWaitMsec%1000)*1000000;
            struct timespec tp;
            clock_gettime(CLOCK_REALTIME,&tp);
            s += tp.tv_sec;
            ns += tp.tv_nsec;
            s += (ns/1000000000);
            ns %= 1000000000;
            struct timespec ts;//compilo ts
            ts.tv_nsec = ns;
            ts.tv_sec  = s;
            bool spuriousWakeup=false;
            do {
                if(pthread_cond_timedwait(&_thereIsAnEvent, &_lockQueue, &ts)) {
                    pthread_mutex_unlock(&_lockQueue);//rilascio l'esclusivita' di accesso sulla coda
                    return NULL;//timeout di attesa senza che siano stati ricevuti eventi sulla coda. Restituisco un puntatore a NULL.
                } else {
                    //e` arrivato un evento! Oppure si tratta di un (raro) spurious wakeup, nel qual 
                    //caso devo continuare l'attesa sino alla naturale scadenza del timeout. 
                    //NOTA: potrei controllare direttamente _eventQueue, ma dato che l'informazione 
                    //e` equivalente, controllo _chronologicalLabelSequence (dovrebbe essere piu` 
                    //efficiente: la _eventQueue e` una mappa di stringhe e deque di eventi, mentre la 
                    //_chronologicalLabelSequence e` una semplice deque di stringhe!
                    if(_chronologicalLabelSequence.empty()) spuriousWakeup=true;
                    else spuriousWakeup=false;
                }
            } while (spuriousWakeup);
        }
    }
    /*
    Facendo il test dei "trentatre` threddini" occasionalmente avevo un crash! Dopo aver controllato tutto piu` volte e non aver
    trovato nulla di sbagliato, ho scoperto che quello che causava il crash era uno "spurious wakeup"!

    qui di seguito un estratto dalla specifica POSIX:

    "[...] When using condition variables there is always a Boolean predicate involving shared variables associated with each 
    condition wait that is true if the thread should proceed. Spurious wakeups from the pthread_cond_timedwait() or 
    pthread_cond_wait() functions may occur. Since the return from pthread_cond_timedwait() or pthread_cond_wait() does not 
    imply anything about the value of this predicate, the predicate should be re-evaluated upon such return. [...] "

    Lo spurious wakeup faceva si` che mi trovassi nella riga:

    std::deque < Event* > *evQ = &(_eventQueue.find(_chronologicalLabelSequence.back())->second);

    supponendo (in questo caso a torto) che _eventQueue non potesse essere vuota, e che quindi it fosse valido. A causa dei 
    (fortunatamente molto rari ma invevitabili) spurious wakeup, questa situazione causava un crash (per esempio per accesso
    ad una locazione di memoria non valida (l'indirizzo della find su _eventQueue), oppure per pop_back da coda vuota, ...)

    Il ciclo do while sulla pthread_cond_timedwait ha lo scopo di gestire correttamente l'eventualita` di spuriousWakeup:

    Se la pthread_cond_timedwait ritorna un valore diverso da zero, significa che qualcosa e` andato storto (tipicamente
    e` scaduto il timeout o era gia` scaduto all'atto della chiamata della pthread_cond_timedwait) e il comportamento corretto
    e` uscire ritornando NULL al chiamante (nessun evento)

    Se invece la pthread_cond_timedwait ritorna zero, nella stragrande maggioranza dei casi significa che e` arrivato un evento,
    ovvero che _chronologicalLabelSequence passa da essere vuota ad avere un elemento (la condizione _eventQueue.empty() 
    verificata all'inizio della funzione, prima di fare la pthread_cond_wait significa anche _chronologicalLabelSequence vuota, 
    dato che sono perfettamente allineate). 
    
    In rarissimi casi, tuttavia, ovvero quando succede uno spurious wakeup, la pthread_cond_wait ritorna zero, ma non ci sono eventi, 
    ovvero _chronologicalLabelSequence resta vuota (condizione che posso quindi usare per individuare gli spurious wakeup). In questi
    casi devo semplicemente ritornare nella pthread_cond_wait con gli stessi parametri che avevo. Se c'e` ancora tempo, ripeto 
    semplicemente il ciclo (gestendo anche un eventuale praticamente impossibile doppio spurious wakeup!!!), se invece quando ripeto
    la pthread_cond_wait il timer e` nel frattempo scaduto, pthread_cond_wait ritorna un valore diverso da zero e mi trovo nel caso
    precedente, ovvero esco ritornando NULL al chiamante.
    */
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
    pthread_mutex_unlock(&_lockQueue);//rilascio l'esclusivita' di accesso sulla coda
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
    labels.push_front(StopThreadLabel);//StopThreadEvent MUST be always handled
                                       //come spiegato sopra, DEVO sempre e comunque gestire StopThreadEvent
    if(maxWaitMsec == WAIT4EVER) {
#ifdef _ZDEBUG
        ziblog(LOG::WRN, "invalid wait forever for a specific bundle of events... set wait to default value of 1 sec to prevent starvation...");
#endif
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
    struct timespec ts;//compilo ts
    ts.tv_nsec = ns;
    ts.tv_sec  = s;
    pthread_mutex_lock(&_lockQueue);
    while (!timeout){
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
                pthread_mutex_unlock(&_lockQueue);
                return ret;
            }
        }
        bool spuriousWakeup=false;
        do {
            if(pthread_cond_timedwait(&_thereIsAnEvent, &_lockQueue, &ts)) {
                pthread_mutex_unlock(&_lockQueue);//rilascio l'esclusivita' di accesso sulla coda
                return NULL;//timeout scaduto senza aver ricevuto l'evento voluto. Ritorno NULL.
            } else {
                //e` arrivato un evento! Oppure si tratta di un (raro) spurious wakeup, nel qual 
                //caso devo continuare l'attesa sino alla naturale scadenza del timeout. 
                //NOTA: potrei controllare direttamente _eventQueue, ma dato che l'informazione 
                //e` equivalente, controllo _chronologicalLabelSequence (dovrebbe essere piu` 
                //efficiente: la _eventQueue e` una mappa di stringhe e deque di eventi, mentre la 
                //_chronologicalLabelSequence e` una semplice deque di stringhe!
                if(_chronologicalLabelSequence.empty()) spuriousWakeup=true;
                else spuriousWakeup=false;
            }
        } while (spuriousWakeup);
        clock_gettime(CLOCK_REALTIME,&tp);/* se sono qui, e' arrivato un evento sulla coda. Resta da controllare se e' quello voluto,
                                           * e in caso contrario riarmare l'attesa per il tempo restante
                                           */
        timeout = (tp.tv_sec > ts.tv_sec) || ((tp.tv_sec == ts.tv_sec) && (tp.tv_nsec >= ts.tv_nsec));
    }
    pthread_mutex_unlock(&_lockQueue);
    return NULL;/* ho ricevuto qualcosa ma tale ricezione e' avvenuta proprio allo scadere di maxWaitTimeout. In tal caso, potrebbe succedere
                 * che il tempo residuo ts della pthread_cond_timedwait sia talmente esiguo da scadere quando l'esecuzione arriva alla chiamata
                 * di clock_gettime per cui timeout = true. In tal caso tutto va esattamente come se fosse scaduto il timeout prima della ricezione
                 * dell'evento atteso, solo che mi trovo fuori da "while (!timeout) anziche', come normalmente accadrebbe in questi casi, dentro
                 * l'if(pthread_cond_timedwait(&_thereIsAnEvent, &_lockQueue, &ts)). Occorre allora rilasciare il semaforo e ritornare NULL
                 * esattamente come se l'evento atteso fosse arrivato troppo in ritardo (non e' cosi' in realta', ma e' esattamente come se fosse cosi',
                 * e questo e' l'unico modo di trattare l'evento mantenendo la coerenza rispetto al chiamante. D'altronde la colpa e' dovuta al tempo
                 * materiale per eseguire le istruzioni tra la pthread_cond_timedwait e la clock_gettime: e' il "bello" della programmazione real-time!)
                 */
}

/*
30/01/16 eliminato metodo alive. Infatti se chiamato su un thread già terminato (ovvero esattamente come lo utilizzato,
per verificare nel distruttore se il thread stava ancora girando) il _th non e` piu` valido e il comportamento della
pthread_kill e` indefinito (mi e` successo che causasse segmentation fault...)
bool Thread::alive() {return (_th && !pthread_kill(_th, 0));}//pthread_kill usata con sig=0 dice se _th è vivo
*/

/*
26/11/14 metodo kill() eliminato: e` troppo pericoloso e non deve servire mai!
void Thread::kill()
{
    pthread_mutex_lock(&_lockSet);
    if(_set) {
        pthread_cancel(_th);
        _set=false;//serve per permettere di fare nuovamente Start (ovvero ridare vita al thread che si appoggia alla
                   //classe) permettendo il riutilizzo dell'oggetto che incapsula il thread (se non viene distrutto!)
        _th=0;
    }
    pthread_mutex_unlock(&_lockSet);
}
*/

//-------------------------------------------------------------------------------------------
//AutonomousThread
void AutonomousThread::run(){while(!exit) this->singleLoopCycle();}

void * AutonomousThread::ThreadLoop(void * arg)
{
    ((AutonomousThread *)arg)->run();
    return NULL;
}

AutonomousThread::AutonomousThread()
{
    pthread_mutex_init(&_lockSet, NULL);
    pthread_mutex_init(&_lockJoin, NULL);
    _th = 0;
    _set=false;
    _joinedOnce=false;
    exit=true;//parto sullo Start(), non nel costruttore!
}

AutonomousThread::~AutonomousThread()
{
#ifdef _ZDEBUG
    pthread_mutex_lock(&_lockSet);
    if(_set) {
        std::cerr<<"Thread loop (run method) MUST BE END before destroying a Thread object\n";
        std::cerr<<"Remember that a Thread loop should terminate only processing a StopThreadEvent\n";
    }
    //ATTENZIONE! Non posso usare ziblog perche` se stessi facendo delete della _instance di log, la emitEvent chiamata 
    //dalla enqueueMessage dentro ziblog fa pushIn che fa lock di _lockSet già lockata causando un deadlock
    pthread_mutex_unlock(&_lockSet);
#endif
    Stop();//Non sto embeddando Stop nel distruttore (vedi commento sopra!). La chiamata a Stop fatta qui
           //serve solo come come "paracadute". Se tutto va come deve, entro in Stop, faccio lock di _lockSet,
           //controllo _set che e` false e esco. Se invece il thread sta erroneamente girando, lo stoppo.
           //compilando con il flag _ZDEBUG quest'ultimo caso e`sottolineato come errore da un ziblog
    pthread_mutex_destroy(&_lockSet);
    pthread_mutex_destroy(&_lockJoin);
}

void AutonomousThread::Start()
{
    pthread_mutex_lock(&_lockSet);
    exit=false;
    if(!_set) {
        pthread_mutex_lock(&_lockJoin);
        pthread_create (&_th, NULL, ThreadLoop, this);
        _joinedOnce=false;
        pthread_mutex_unlock(&_lockJoin);
        _set=true;
    }
    pthread_mutex_unlock(&_lockSet);
}

#ifndef arm //pthread_setname_np e` utilizzabile con glibc>2.12 nella nostra toolchain la versione e` inferiore...
#ifdef _ZDEBUG
void AutonomousThread::Start(const std::string& threadName)
{
    pthread_mutex_lock(&_lockSet);
    exit=false;
    if(!_set) {
        pthread_mutex_lock(&_lockJoin);
        pthread_create (&_th, NULL, ThreadLoop, this); 
        if(pthread_setname_np(_th, threadName.c_str())!=0) ziblog(LOG::ERR, "pthread_setname_np (%s, %s)", threadName.c_str(),strerror(errno));
        _joinedOnce=false;
        pthread_mutex_unlock(&_lockJoin);
        _set=true;
    }
    pthread_mutex_unlock(&_lockSet);
}
#endif
#endif

// NOTA IMPORTANTE! Mi è successo di chiamare erroneamente Stop() dentro run(). Il risultato e` stato un deadlock! Peraltro difficile da individuare!
// Infatti Join esce subito (ret=EDEADLK), mentre la purgeRxQueue completa il danno rimuovendo tutti gli eventi, e quindi anche lo StopThreadEvent
// dalla coda prima che la pullOut possa estrarlo dalla coda e quindi terminare il thread! (pullOut non può correttamente fare nulla perchè c'è il lock
// su _lockSet). Avevo anche pensato di far ritornare un bool a Join, in modo da rilevare questa situazione e magari non fare la purgeRxQueue in un caso
// come questo, ma alla fine ho deciso di non farlo perchè la cura sarebbe stata peggiore del male! Infatti il paradigma è che uscito dalla Stop il 
// thread DEVE essere terminato! Se lo lascio ancora li c'è già un errore, perchè chi chiama Stop() confida sul fatto che può dover aspettare ma una
// volta che Stop ritorna, il thread è terminato. In definitiva la regola è quella di non chiamare Stop MAI dentro run, e se lo si fa l'errore è li
// e non si può recuperare commettendo un altro errore! E` meglio loggare in tutti i modi per rendere più facile rilevare l'errore anzichè tentare di
// correggerlo rendendo l'errore semplicemente più subdolo perchè più difficile da individuare!!!!

// IMPORTANT: Stop() should never be called inside run()
// ::::::::::::::::::::::::::::::::::::::::::::::::
// IMPORTANTE: Stop() non DEVE MAI essere chiamata dentro run()
void AutonomousThread::Stop()
{
    pthread_mutex_lock(&_lockSet);
    if(_set) {
        exit=true;
        Join();//N.B.: attenzione a non fare confusione! non sto facendo "autojoin" i
               //la classe incapsula i dati ma il thread gira indipendentemente!
        _set=false;//serve per permettere di fare nuovamente Start (ovvero ridare vita al thread che si appoggia alla
                   //classe) permettendo il riutilizzo dell'oggetto che incapsula il thread (se non viene distrutto!)
        _th=0;
    }
    pthread_mutex_unlock(&_lockSet);
}

// IMPORTANT: Join() should never be called inside run()
// ::::::::::::::::::::::::::::::::::::::::::::::::
// IMPORTANTE: Join() non DEVE MAI essere chiamata dentro run()
void AutonomousThread::Join()
{ 
    pthread_mutex_lock(&_lockJoin);
    if(!_joinedOnce) { 
        if(_th) {
            //se _th=0, la pthread_join fa segmentation fault. Non puo` succedere per una corsa con Stop perche` anche
            //in Stop prima di mettere _th=0 viene chiamata Join che si fermerebbe sulla _lockJoin. L'unica corsa 
            //possibile e` se Join fosse chiamata insieme a Start... in tal caso se arriva prima Start la join fa
            //il suo lavoro (thread e` partito) se al contrario arriva prima la join trova _th ancora = zero e esce
            //(corretto: il thread non e` ancora partito). Quindi non servono precauzioni! Se qualcuno invece potesse
            //fare il contrario (mettermi _th a zero dopo che ho controllato con l'if e deciso che e` diverso da zero)
            //sarebbero dolori!!! (segmentation fault). Per proteggermi da questa situazione potrei copiare il valore
            //di _th in una variabile locale prima di fare if ma dopo il lock di _lockJoin: in questo caso non avrei
            //la garanzia di avere l'ultimo valore aggiornato ma avrei la garanzia che nessuno possa cambiarmi il
            //valore di _th sotto il culo dopo averlo controllato con l'if... Tuttavia poiche` ho analizzato la questione
            //e deciso che nessuno puo` mettermi _th a zero (chi potrebbe, ovvero Stop, come detto non ce la fa perche`
            //si blocca su join.. se _th fosse messo a zero in Stop prima di chiamare Join potrebbe succedere, ma cosi`
            //non e` possibile) lascio tutto cosi`: mi basta questo commento nel caso mi venissero dubbi in futuro!
            int ret = pthread_join(_th, NULL);
            _joinedOnce=true;
#ifdef _ZDEBUG
            if(ret == EDEADLK) ziblog(LOG::ERR, "thread cannot join himself");
#endif
        }
    }
    pthread_mutex_unlock(&_lockJoin);
}

/*
30/01/16 eliminato metodo alive. Infatti se chiamato su un thread già terminato (ovvero esattamente come lo utilizzato,
per verificare nel distruttore se il thread stava ancora girando) il _th non e` piu` valido e il comportamento della
pthread_kill e` indefinito (mi e` successo che causasse segmentation fault...)
bool AutonomousThread::alive(){return (_th && !pthread_kill(_th, 0));}//pthread_kill usata con sig=0 dice se _th è vivo
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
#ifdef _ZDEBUG
        ziblog(LOG::WRN, "INVALID EVENT NAME!");
#endif
        return;
    }
    pthread_mutex_lock(&_rwReq);
    //here I use "while(_pendingReaders)" instead of "if(_pendingReaders)" (the natural logic choice would be 
    //"if") to handle potential (extremely rare) spurious wakeup (see POSIX-pthread_cond_wait specification)
    while(_pendingReaders) pthread_cond_wait(&_noPendingReaders, &_rwReq);
    _pendingWriters++;
    pthread_mutex_unlock(&_rwReq);
    pthread_mutex_lock(&_lockEvtReceivers);
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
        if(*itEvtReceiverList == T) {//someone is trying to register twice to the same event...
                                     //qualcuno sta facendo per errore una doppia registrazione su uno stesso evento
            pthread_mutex_lock(&_rwReq);
            if(!--_pendingWriters) pthread_cond_signal(&_noPendingWriters);
            pthread_mutex_unlock(&_rwReq);
            pthread_mutex_unlock(&_lockEvtReceivers);
            return;
        }
    }//if we're here then the receiver is not registered.
     //se sono qui, il ricevitore non e' ancora registrato. Lo registro e rilascio il semaforo.
    _evtReceivers[label].push_front(T);//registro un nuovo gestore per questo tipo di evento
    pthread_mutex_lock(&_rwReq);
    if(!--_pendingWriters) pthread_cond_signal(&_noPendingWriters);
    pthread_mutex_unlock(&_rwReq);
    pthread_mutex_unlock(&_lockEvtReceivers);
}

void EventManager::unmapReceiver(Thread *T)
{
    pthread_mutex_lock(&_rwReq);
    //here I use "while(_pendingReaders)" instead of "if(_pendingReaders)" (the natural logic choice would be 
    //"if") to handle potential (extremely rare) spurious wakeup (see POSIX-pthread_cond_wait specification)
    while(_pendingReaders) pthread_cond_wait(&_noPendingReaders, &_rwReq);
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

/*
EventManager::~EventManager()
{
    _evtReceivers.clear();
    pthread_mutex_destroy(&_rwReq);
    pthread_mutex_destroy(&_lockEvtReceivers);
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
