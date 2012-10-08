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
#include <errno.h>
#include <deque>
#include <iostream>
#include <map>
#include <string>
#include <string.h>
#include <stdlib.h>
#include <cstring>
#include <sys/time.h>
//-------------------------------------------------------------------------------------------
#define WAIT4EVER -1
#define NOWAIT 0

//Event
class Event {
    friend class Thread;
public:
    virtual ~Event(){};
    Event(std::string eventName);
    std::string eventName();
    void emitEvent();
protected:
    std::string _eventName;
    virtual Event* clone() const = 0;
    void sendEventTo(class Thread* target);/* utilizzato dall'evento timeout che non deve essere
                                            * emesso "in broadcast" ma inviato solo ad uno specifico
                                            * thread destinatario.
                                            */
};
//-------------------------------------------------------------------------------------------
//THREAD
class Thread {
    friend class Event;
private:
    pthread_t _th;
    pthread_mutex_t _lockQueue;
    pthread_mutex_t _standAloneGuarantee;//mutex utilizzato per garantire l'unicità del thread associato alla classe.
                                         //viene utilizzato dentro il metodo Start() per prevenire i danni causati da
                                         //un'eventuale doppia chiamata di Start(). Infatti se ciò accadesse, verrebbero
                                         //creati due thread indipendenti e identici (stesso ciclo run()) ma che condivi-
                                         //derebbero gli stessi dati all'interno della classe con corse, problemi di 
                                         //scrittori/lettori, starvation, deadlock...
    bool _set;
    pthread_cond_t _thereIsAnEvent;
    virtual void run()=0;
    static void *ThreadLoop(void * arg);
    std::map < std::string, std::deque < Event* > > _eventQueue;
    void pushIn(Event*);
    std::deque <std::string > _chronologicalEventNameSequence;/* lista ordinata dei puntatori alle code da utilizzare
                                                               * per estrarre gli eventi in ordine cronologico
                                                               */
protected:
    /*
     * Nota importante: i metodi pullOut dovrebbero essere chiamati SOLO all'interno del ciclo del thread (metodo run()).
     * Infatti, se da un punto di vista tecnico nulla vieta che siano chiamate al di fuori utilizzandole come metodi di
     * un'istanza di tipo Thread, facendo cosi' esse girerebbero nel thread chiamante, e pertanto anche le operazioni di
     * lock/unlock dei mutex della coda del thread girerebbero nel thread chiamante anziche' nel thread possessore della
     * coda. Analogo discorso vale per la StopReceivingAndDiscardReceived().
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
     *  ciclo del thread esistano sinche` il thread non termina. Per questo motivo e` indispensabile che l'evenot StopThreadEvent sia 
     *  notificato sempre quando presente, e che sia gestito subito onde evitare possibili starvation e nei casi piu' gravi deadlock 
     *  (si pensi per esempio al caso di un thread "A" che chiama Stop() su un thread "B" e si blocchi in attesa che "A" termini mentre 
     *  invece "B" sta attendendo eventi di un certo tipo (e solo quelli) da "A": "A" non emettera` nessun evento e "B" non terminera` 
     *  mai => "A" resta bloccato all'infinito con "B".
     *  
     */
    Event* pullOut(std::string eventName, int maxWaitMsec = NOWAIT);/* metodo che restituisce il primo evento disponibile sulla
                                                                     * coda del tipo richiesto, se presente entro il tempo
                                                                     * prestabilito. Se l'evento non viene ricevuto entro il
                                                                     * timeout, il metodo restituisce NULL. Fa eccezione l'evento
                                                                     * "StopThreadEvent" che se presente in coda viene notificato
                                                                     * comunque.
                                                                     */
    Event* pullOut(std::deque <std::string> eventNames, int maxWaitMsec = NOWAIT);/* metodo che restituisce il primo evento
                                                                                   * disponibile sulla coda del tipo di uno
                                                                                   * qualsiasi appartenente alla lista eventNames
                                                                                   * degli eventi. Se l'evento non viene ricevuto
                                                                                   * entro il timeout, il metodo restituisce NULL.
                                                                                   * Fa eccezione l'evento "StopThreadEvent" che, se
                                                                                   * presente in coda, viene notificato comunque.
                                                                                   */
    void StopReceivingAndDiscardReceived();/* interrompe la ricezione di qualsiasi evento deregistrandisi da tutto 
                                            * e elimina tutti gli eventi presenti in coda al momento della chiamata.
                                            */
    Thread();
public:
    virtual ~Thread();
    void Start();
    void Stop();//N.B.: non deve mai essere chiamata dentro run()
    void Join();//N.B.: non deve mai essere chiamata dentro run()
    void register2Event(std::string eventName);
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
    static void mapEvent2Receiver(std::string eventName, Thread *T);
    static void unmapReceiver(Thread *T);
public:
    EventManager();
    ~EventManager();
};
//===========================================================================================
//Alcuni utili eventi generici ...
//
// NOTA: la definizione di un nuovo evento deve essere fatta come segue:
// => deve ereditare Event
//
// => dovrebbe avere un membro statico pubblico const string che serve per identificare 
//    univocamente l'evento. Per gli eventi utilizzabili in diversi contesti (come per esempio
//    il caso, che segue, della generica ricezione/trasmissione di dati) la scelta che ho fatto
//    e' stata di assegnare un prefisso comune al nome degli eventi (nel caso che segue "rxData/
//    txData") seguito da un identificatore che dipende dal contesto (sap - service access point)
//    che distingue due eventi concettualmente identici emessi da entita' differenti, ovvero permette
//    di utilizzare la struttura di uno stesso evento da entita' diverse (per esempio gli eventi
//    txData/dev/ttyS0 e txData/dev/ttyS1 sono entrambi eventi txData ma si riferiscono a due porte
//    seriali diverse). In questo modo pur mantenendo la struttura generale di un evento identica
//    si puo' fare in modo che un utilizzatore possa registrarsi direttamente su un particolare evento
//    (per esempio sugli eventi ricevuti da una certa seriale) senza dover registrarsi su tutti gli
//    eventi per poi andare nel dettaglio e scartare quelli che non interessano.
//    Osservazione: quando un evento non e' "riutilizzabile" nel senso che si tratta di un evento
//    particolare legato ad una certa implementazione/macchina a stati di una applicazione, e' opportuno
//    assegnare un nome senza prefissi-suffissi.
//
// => deve avere il metodo clone che esegue una copia esatta dell'evento. Tale metodo e' praticamente
//    sempre uguale per tutti gli eventi (Event* clone() const{return new NuovoEvento(*this);}) ma e'
//    necessario inserirlo in ogni classe per fare in modo che il polimorfismo agisca sull'effettivo
//    tipo derivato: se lo si mettesse solo nella classe base, la copia avverrebbe del *this della 
//    classe base, ovvero non avrei la copia dell'oggetto derivato. E' utile notare che clone usa
//    il costruttore di copia, ed e' pertanto demandata a questo la gestione delle eventuali allocazioni
//    in heap (regola del BIG 3!)
//
//-------------------------------------------------------------------------------------------
//EVENTO ricezione/trasmissione Dati
class DataEvent : public Event {
protected:
    unsigned char* _buf;
    int _len;
    Event* clone() const{return new DataEvent(*this);}
public:
    enum DataEventId {Tx, Rx, EndOfList};
    const static std::string DataEventName[EndOfList];
    unsigned char* buf(){return _buf;}
    int len(){return _len;}
    DataEvent(DataEventId Id, std::string sap, const unsigned char* buf, int len):Event(DataEventName[Id] + sap)
    {
        _buf = new unsigned char[len];
        memcpy(_buf, buf, len);
        _len = len;
    }
    ~DataEvent(){delete[] _buf; _buf=NULL, _len=0;}
    DataEvent(const DataEvent &obj):Event(obj._eventName)
    {
        _buf = new unsigned char[obj._len];//allocate new space
        memcpy(_buf, obj._buf, obj._len);//copy values
        _len = obj._len;
    }
    DataEvent & operator = (const DataEvent &src)
    {
        _eventName = src._eventName;
        if(this == &src) return *this; //self assignment check...
        if(_buf) delete[] _buf;//deallocate
        _buf = new unsigned char[src._len];//allocate new space
        memcpy(_buf, src._buf, src._len);//copy values
        _len = src._len;
        return *this;
    }
};

//EVENTO notifica errore
class ErrorEvent : public Event {
protected:
    Event* clone() const{return new ErrorEvent(*this);}
public:
    const static std::string ErrorEventName;
    std::string errorMessage;
    ErrorEvent(std::string sap, std::string errorMessage):Event(ErrorEventName + sap), errorMessage(errorMessage){}
};

//EVENTO Stop di un thread
class StopThreadEvent : public Event {
protected:
    Event* clone() const{return new StopThreadEvent(*this);}
public:
    const static std::string StopThreadEventName;
    StopThreadEvent():Event(StopThreadEventName){}
};

//-------------------------------------------------------------------------------------------
#endif	/* _THREAD_H */
