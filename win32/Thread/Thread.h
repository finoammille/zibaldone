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

#ifndef _THREAD_H
#define	_THREAD_H
//-------------------------------------------------------------------------------------------
#include <windows.h>
#include <errno.h>
#include <vector>
#include <deque>
#include <iostream>
#include <map>
#include <string>
#include <string.h>
#include <stdlib.h>
#include <cstring>
//-------------------------------------------------------------------------------------------

namespace Z
{
//-------------------------------------------------------------------------------------------
const int WAIT4EVER = -1;
const int NOWAIT = 0;
//-------------------------------------------------------------------------------------------
class Event {
    friend class Thread;
protected:
    virtual Event* clone() const=0;
    std::string _label;
public:
    std::string label() const;
    Event(const std::string&);
    virtual ~Event();
    Event(const Event &);
    Event & operator = (const Event &);
    void emitEvent();//L'emissione dell'evento comporta la notifica a tutti i thread registrati sull'evento
};
//-------------------------------------------------------------------------------------------
//Thread
class Thread {
    friend class Event;
private:
    HANDLE _th;
    HANDLE _lockQueue;
    HANDLE _lockSet;/* mutex utilizzato per garantire l'unicità del thread associato alla classe.
					 * viene utilizzato dentro il metodo Start() per prevenire i danni causati da
                     * un'eventuale doppia chiamata di Start(). Infatti se ciò accadesse, verrebbero
                     * creati due thread indipendenti e identici (stesso ciclo run()) ma che condivi-
                     * derebbero gli stessi dati all'interno della classe con corse, problemi di
                     * scrittori/lettori, starvation, deadlock...
					 */
    bool _set;
    HANDLE _thereIsAnEvent;
    virtual void run()=0;
	static DWORD WINAPI ThreadLoop(LPVOID arg);
    std::map < std::string, std::deque < Event* > > _eventQueue;
    void pushIn(Event*);
    std::deque <std::string > _chronologicalLabelSequence;/* lista ordinata dei puntatori alle code da utilizzare
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
    *  ciclo del thread esistano sinche` il thread non termina. Per questo motivo e` indispensabile che l'evenot StopThreadEvent sia
    *  notificato sempre quando presente, e che sia gestito subito onde evitare possibili starvation e nei casi piu' gravi deadlock
    *  (si pensi per esempio al caso di un thread "A" che chiama Stop() su un thread "B" e si blocchi in attesa che "A" termini mentre
    *  invece "B" sta attendendo eventi di un certo tipo (e solo quelli) da "A": "A" non emettera` nessun evento e "B" non terminera`
    *  mai => "A" resta bloccato all'infinito con "B".
    *
    */
    Event* pullOut(const std::string& label, int maxWaitMsec = NOWAIT);/* metodo che restituisce il primo evento disponibile sulla
                                                                          * coda del tipo richiesto, se presente entro il tempo
                                                                          * prestabilito. Se l'evento non viene ricevuto entro il
                                                                          * timeout, il metodo restituisce NULL. Fa eccezione l'evento
                                                                          * "StopThreadEvent" che se presente in coda viene notificato
                                                                          * comunque.
                                                                          */
    Event* pullOut(std::deque <std::string> labels, int maxWaitMsec = NOWAIT);/* metodo che restituisce il primo evento
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
    bool alive();//dice se il thread e` vivo
    
    /*26/11/14 metodo kill() eliminato: e` troppo pericoloso e non deve servire mai!*/
	/*void kill();*//* termina il thread in modo "improvviso". Questo metodo nonrmalmente non andrebbe utilizzato (per questo motivo non
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
    void Stop();//N.B.: non deve mai essere chiamata dentro run()
    void Join();//N.B.: non deve mai essere chiamata dentro run()
    void register2Label(const std::string& label);
private:
    //secondo la "Law of The Big Three" del c++, se si definisce uno tra: distruttore, costruttore di copia o operatore di
    //assegnazione, probabilmente occorrera' definire anche i due restanti (e non usare quelli di default cioe`!). Tuttavia
    //in questo caso un Thread non puo` essere assegnato o copiato, per cui il rispetto della suddetta regola avviene
    //rendendoli privati in modo da prevenirne un utilizzo involontario!
    Thread(const Thread &);//non ha senso ritornare o passare come parametro un thread per valore
    Thread & operator = (const Thread &);//non ha senso assegnare un thread per valore
};
//-------------------------------------------------------------------------------------------
//AutonomousThread
/*
    AutonomousThread e` la classe base per un tipo particolare di Thread, caratterizzato da:

    a) non riceve nessun evento, pertanto non sono disponibili i metodi register2Label, pullOut

    b) viene stoppato tramite la chiamata Stop()

    c) non permette il Join() per evitare il rischio di un deadlock.
       Per capirne la ragione, ipotizziamo lo scenario seguente:
       sia A un thread di tipo AutonomousThread (ovvero istanza di una classe derivata da AutonomousThread che di suo non e`
       istanziabile essendo astratta). Sia B un thread "normale" (derivato da Thread). Supponiamo che non ci siano altri thread.
       Se B fa Join() su A, B aspetta che A termini, ma A non terminera` mai perche` nessuno puo` stopparlo (l'unico era B ma
       ha fatto Join()...). Chiaramente potrebbe esserci un terzo thread C che stoppi A sbloccando anche B, ma il rischio di avere
       un deadlock e` troppo elevato per lasciare disponibile il metodo Join

    b) il metodo puramente virtuale singleLoopCycle e` la funzione che implementa il SINGOLO CICLO del thread AuthonomousThread.
       tale metodo viene ripetuto all'infinito sinche` non viene chiamato lo Stop() del thread. singleLoopCycle NON DEVE 
       CONTENERE UN CICLO INFINITO
*/
class AutonomousThread : protected Thread {
    HANDLE _lock;
    bool exit;
    void run();
    virtual void singleLoopCycle()=0;
public:
    AutonomousThread();
    void Start();
    void Stop();    
};
//-------------------------------------------------------------------------------------------
//EVENT MANAGER
class EventManager {
    friend class Event;
    friend class Thread;
private:
    static HANDLE _rwReq;
    static HANDLE _noPendingReaders;
    static HANDLE _noPendingWriters;
    static int _pendingReaders;
    static int _pendingWriters;
    static HANDLE _lockEvtReceivers;
    static std::map < std::string, std::deque < Thread *> > _evtReceivers;
    static void mapLabel2Receiver(const std::string& label, Thread *T);
    static void unmapReceiver(Thread *T);
public:
    EventManager();
    //~EventManager();
};
//-------------------------------------------------------------------------------------------
}//namespace Z
#endif	/* _THREAD_H */
