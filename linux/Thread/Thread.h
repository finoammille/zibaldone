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

#ifndef _THREAD_H
#define	_THREAD_H
//-------------------------------------------------------------------------------------------
#include "pthread.h"
#include <signal.h>
#include <errno.h>
#include <vector>
#include <deque>
#include <iostream>
#include <map>
#include <string>
#include <stdlib.h>
#include <cstring>
#include <sys/time.h>
//-------------------------------------------------------------------------------------------

namespace Z
{
//-------------------------------------------------------------------------------------------
const int WAIT4EVER = -1;
const int NOWAIT = 0;
const std::string StopThreadEventId = "StopThreadEvent";//Evento che deve essere OBBLIGATORIAMENTE gestito da tutti i thread: quando
                                                        //un thread riceve l'evento StopThreadEvent deve uscire prima possibile.
/*

EVENT

Un evento e` un buffer di byte identificato da una stringa. La semantica del buffer e` delagata all'utilizzatore, Event si fa
carico solo di contenere i byte in una struttura compatibile con il framework della gestione degli eventi. In particolare Event
fa una propria copia del buffer passatogli tramite il costruttore ed e` pertanto compito del chiamante liberare, se necessario,
la memoria dopo aver passato i byte Event.

Osservazione: l'implementazione degli eventi come buffer di byte consente sostanzialmente tutto! Infatti il contenuto potrebbe
essere l'elenco dei parametri da passare al costruttore di una classe identificata univocamente da EventId (caso tipico), ma
potrebbe contenere qualsiasi cosa, un immagine, un file..... L'emissione di un evento non e` legata alla presenza di nessun thread
(anche se e` ovviamente ragionevole che ci sia almeno un ricevitore!),  per emettere un evento basta chiamare il metodo ''emitEvent''.
Quando viene chiamato questo metodo, il gestore degli eventi (event manager) si fa carico di fare una copia indipendente dell'evento
emesso sulla coda di ogni thread che si era precedentemente registrato dichiarandosi interessato all'evento stesso. Questo approccio
ha il pregio della semplicita` ma per contro occorre ricordare che l'individuazione di un evento avviene SOLO PER NOME per cui
bisogna fare attenzione ai nomi degli eventi quando un thread si registra per riceverli (lo fa dicendo il nome degli eventi che vuole
ricevere): registrarsi su un nome sbagliato (magari a causa di una lettera minuscola/maiuscola) causa la non ricezione dell'evento!

25/11/13
Ci sono due modi per definire un evento:

1) istanziare un oggetto di tipo Event

2) derivare da EventObject.

   EventObject deriva da Event, per cui tutti i metodi per la gestione degli eventi (pullOut, emitEvent, ...) sono utilizzabili esattamente
   come con Event

   In questo caso un evento è un oggetto qualsiasi identificato da una stringa. Se si definisce un evento derivando
   da EventObject occorre implementare il metodo clone() che deve fare una deep copy dell'oggetto. Se si rispetta la regola del big 3 (se  si
   definisce uno qualsiasi tra distruttore, costruttore di copia, overload dell'operatore di assegnazione allora occorre quasi sicuramente
   definire anche gli altri due) allora il metodo clone puo` banalmente essere definito nella classe derivata come

   EventObject* clone()const{return new EventObject(*this);}

   Ovviamente non posso definirlo nella classe EventObject (in cui clone è puramente virtuale per appunto imporre l'implementazione nelle classi
   derivate) perchè clonerebbe EventObject invece della classe derivata.

   ATTENZIONE! l'overload dell'operatore di assegnazione cosi` come il costruttore di copia devono fare la deep copy dei dati dell'oggetto che deriva
   da EventObject e devono copiare l' eventId altrimenti l'evento una volta copiato o assegnato non sarà più identificabile!

   In altri termini:

   sia Ev la classe derivata da EventObject. 
   
   L'overload del costruttore di copia (se necessario) dovrà avere la forma:

   Ev::Ev(const Ev& obj):EventObject(obj)
   {
       ...
   }

   oppure, 

   Ev::Ev(const Ev& obj):EventObject(_eventId)
   {
       ...
   }

   Per quando riguarda invece l'overload dell'operatore di assegnazione, la forma dovrà essere:

   Ev & Ev::operator = (const Ev &src)
   {
       EventObject::operator= (src);

       ...

   }

   oppure

   Ev & Ev::operator = (const Ev &src)
   {
       
       _eventId=src._eventId;

       ...

   }

*/
class Event {
    friend class Thread;
    unsigned char* _buf;
    int _len;
    virtual Event* clone() const;
protected:
    std::string _eventId;
public:
    unsigned char* buf() const;
    int len() const;
    std::string eventId() const;
    Event(const std::string& eventId, const unsigned char* buf=NULL, const int len=0);
    Event(const std::string& eventId, const std::vector<unsigned char>&);
    virtual ~Event();
    Event(const Event &);
    Event & operator = (const Event &);
    void emitEvent();//L'emissione dell'evento comporta la notifica a tutti i thread registrati sull'evento
};

class EventObject : public Event {
protected:
    virtual Event* clone() const=0;
public:
    EventObject(const std::string &);
    EventObject & operator = (const EventObject &);
};

//-------------------------------------------------------------------------------------------
//THREAD
class Thread {
    friend class Event;
private:
    pthread_t _th;
    pthread_mutex_t _lockQueue;
    pthread_mutex_t _lockSet;/* mutex utilizzato per garantire l'unicità del thread associato alla classe.
                              * viene utilizzato dentro il metodo Start() per prevenire i danni causati da
                              * un'eventuale doppia chiamata di Start(). Infatti se ciò accadesse, verrebbero
                              * creati due thread indipendenti e identici (stesso ciclo run()) ma che condivi-
                              * derebbero gli stessi dati all'interno della classe con corse, problemi di
                              * scrittori/lettori, starvation, deadlock...
                              */
    bool _set;
    pthread_cond_t _thereIsAnEvent;
    virtual void run()=0;
    static void *ThreadLoop(void * arg);
    std::map < std::string, std::deque < Event* > > _eventQueue;
    void pushIn(Event*);
    std::deque <std::string > _chronologicalEventIdSequence;/* lista ordinata dei puntatori alle code da utilizzare
                                                             * per estrarre gli eventi in ordine cronologico
                                                             */
protected:
   /*
    * Nota importante: i metodi pullOut sono protected perche` possono essere chiamati SOLO all'interno del ciclo del
    * thread (metodo run()). Infatti, se da un punto di vista tecnico nulla vieta che siano chiamate al di fuori
    * utilizzandole come metodi di un'istanza di tipo Thread, facendo cosi' esse girerebbero nel thread chiamante, e
    * pertanto anche le operazioni di lock/unlock dei mutex della coda del thread girerebbero nel thread chiamante
    * anziche' nel thread possessore della coda. Analogo discorso vale per la StopReceivingAndDiscardReceived().
    */
    Event* pullOut(int maxWaitMsec = WAIT4EVER);/* metodo che restituisce il primo evento disponibile sulla coda del thread
                                                 * aspettando al massimo "maxWaitMsec" millisecondi: se non arriva nessun
                                                 * evento entro tale timeout la funzione restituisce NULL.
                                                 * Nota: se chiamata senza specificare la durata massima dell'attesa (maxWaitMsec)
                                                 * pullOut resta indefinitamente in attesa sulla coda sinche' non arriva un evento.
                                                 * Se viceversa maxWaitMsec = 0, pullOut controlla se c'e' un evento gia' in coda
                                                 * e ritorna subito.
                                                 */
   /*
    *  NOTA IMPORTANTE SU pullOut con filtro sugli eventi: il metodo Stop() invia l'evento StopThreadEvent e resta BLOCCATO sinche`
    *  il thread target non termina. Questa scelta e` necessaria per garantire che l'oggetto che incapsula il thread possa essere
    *  distrutto: ho la certezza di poter distruggere l'oggetto senza danni una volta che il metodo Stop() e` ritornato perche' ho
    *  la certezza che a quel punto il thread e` terminato. Infatti serve a garantire che i dati della classe su cui si appoggia il
    *  ciclo del thread esistano sinche` il thread non termina. Per questo motivo e` indispensabile che l'evento StopThreadEvent sia
    *  notificato sempre quando presente, e che sia gestito subito onde evitare possibili starvation e nei casi piu' gravi deadlock
    *  (si pensi per esempio al caso di un thread "A" che chiama Stop() su un thread "B" e si blocchi in attesa che "A" termini mentre
    *  invece "B" sta attendendo eventi di un certo tipo (e solo quelli) da "A": "A" non emettera` nessun evento e "B" non terminera`
    *  mai => "A" resta bloccato all'infinito con "B".
    *
    */
    Event* pullOut(const std::string& eventId, int maxWaitMsec = NOWAIT);/* metodo che restituisce il primo evento disponibile sulla
                                                                          * coda del tipo richiesto, se presente entro il tempo
                                                                          * prestabilito. Se l'evento non viene ricevuto entro il
                                                                          * timeout, il metodo restituisce NULL. Fa eccezione l'evento
                                                                          * "StopThreadEvent" che se presente in coda viene notificato
                                                                          * comunque.
                                                                          */
    Event* pullOut(std::deque <std::string> eventIds, int maxWaitMsec = NOWAIT);/* metodo che restituisce il primo evento
                                                                                 * disponibile sulla coda del tipo di uno
                                                                                 * qualsiasi appartenente alla lista eventIds
                                                                                 * degli eventi. Se l'evento non viene ricevuto
                                                                                 * entro il timeout, il metodo restituisce NULL.
                                                                                 * Fa eccezione l'evento "StopThreadEvent" che, se
                                                                                 * presente in coda, viene notificato comunque.
                                                                                 */
    void StopReceivingAndDiscardReceived();/* interrompe la ricezione di qualsiasi evento deregistrandisi da tutto
                                            * e elimina tutti gli eventi presenti in coda al momento della chiamata.
                                            */
    bool alive();//dice se il thread e` vivo
    void kill();/* termina il thread in modo "improvviso". Questo metodo nonrmalmente non andrebbe utilizzato (per questo motivo non
                 * e` un metodo public e si trova nella sezione protected della classe. Il metodo infatti "uccide" letteralmente
                 * il thread in modo asincrono senza saper nulla di cosa stia facendo. Va utilizzato solo a condizione di essere
                 * assolutamente sicuri che il thread "morituro" non possa IN NESSUN PUNTO DEL SUO LOOP generare:
                 * - memory leak, dangling reference (ovvero non alloca nulla in area heap)
                 * - deadlock (cioè non fa lock di nessun mutex, ovvero NON usa la pullOut)
                 * - ...
                 * oppure si tratta di un intervento "disperato" che viene seguito dalla terminazione dell'intero processo.
                 */
    Thread();
public:
    virtual ~Thread();
    void Start();
    void Stop();//N.B.: non dovrebbe mai essere chiamata dentro run().
    void Join();//N.B.: non dovrebbe mai essere chiamata dentro run()
    void register2Event(const std::string& eventId);
private:
    //secondo la "Law of The Big Three" del c++, se si definisce uno tra: distruttore, costruttore di copia o operatore di
    //assegnazione, probabilmente occorrera' definire anche i due restanti (e non usare quelli di default cioe`!). Tuttavia
    //in questo caso un Thread non puo` essere assegnato o copiato, per cui il rispetto della suddetta regola avviene
    //rendendoli privati in modo da prevenirne un utilizzo involontario!
    Thread(const Thread &);//non ha senso ritornare o passare come parametro un thread per valore
    Thread & operator = (const Thread &);//non ha senso assegnare un thread per valore
};
//-------------------------------------------------------------------------------------------
//EVENT MANAGER
class EventManager {
    friend class Event;
    friend class Thread;
private:
    static pthread_mutex_t _rwReq;
    static pthread_cond_t _noPendingReaders;
    static pthread_cond_t _noPendingWriters;
    static int _pendingReaders;
    static int _pendingWriters;
    static pthread_mutex_t _lockEvtReceivers;
    static std::map < std::string, std::deque < Thread *> > _evtReceivers;
    static void mapEvent2Receiver(const std::string& eventId, Thread *T);
    static void unmapReceiver(Thread *T);
public:
    EventManager();
    ~EventManager();
};
//-------------------------------------------------------------------------------------------
}//namespace Z
#endif	/* _THREAD_H */
