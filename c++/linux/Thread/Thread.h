/*
 *
 * zibaldone - a C++/Java library for Thread, Timers and other Stuff
 *
 * http://sourceforge.net/projects/zibaldone/
 *
 * version 3.2.0, February 14th, 2016
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
//-------------------------------------------------------------------------------------------
//Event
class Event {
    friend class Thread;
protected:
    virtual Event* clone() const=0;
    std::string _label;
public:
    std::string label() const;
    Event(const std::string&);
    virtual ~Event();
    void emitEvent();//all thread who registered to this event will receive their own copy
                     //L'emissione dell'evento comporta la notifica a tutti i thread registrati sull'evento
};
//-------------------------------------------------------------------------------------------
//Thread
class Thread {
    friend class Event;
    friend class SameThreadAddressSpaceEventReceiver;
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
    pthread_mutex_t _lockJoin;/* da specifica POSIX: "Joining with a thread that has previously been joined
                               * results in undefined behaviour". _lockJoin viene utilizzato per garantire
                               * che una volta creato un pthread (ovvero chiamata la Start()) questo possa
                               * essere "joinato" una volta sola.
                               */
    bool _joinedOnce;//indica se il thread e` gia` stato joinato (e quindi non puo` esserlo di nuovo)
    bool _set;
    pthread_cond_t _thereIsAnEvent;
    virtual void run()=0;
    static void *ThreadLoop(void * arg);
    std::map < std::string, std::deque < Event* > > _eventQueue;
    void pushIn(Event*);
    std::deque <std::string > _chronologicalLabelSequence;/* lista ordinata dei puntatori alle code da utilizzare
                                                           * per estrarre gli eventi in ordine cronologico
                                                           */
    void purgeRxQueue();//this method deletes all events in the event queue (calling their destructors)
protected:
   /* pullOut methods are protected because they must be called only inside the thread loop (run() method).
    * A call to pullOut sets the thread in sleeping mode until he receives an event, that is until someone
    * emits an Event the thread has registered to. Note that this sleeping mode is not a busy waiting so the
    * O.S. can use the processor and run other task/thread.
    * :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
    * Nota importante: i metodi pullOut sono protected perche` possono essere chiamati SOLO all'interno del ciclo del
    * thread (metodo run()). Infatti, se da un punto di vista tecnico nulla vieta che siano chiamate al di fuori
    * utilizzandole come metodi di un'istanza di tipo Thread, facendo cosi' esse girerebbero nel thread chiamante, e
    * pertanto anche le operazioni di lock/unlock dei mutex della coda del thread girerebbero nel thread chiamante
    * anziche' nel thread possessore della coda.
    */
    Event* pullOut(int maxWaitMsec = WAIT4EVER);/* This variant returns the first available event in the
                                                 * thread queue waiting at least for "maxWaitMsec" msecs.
                                                 * If no event is in the queue within that time the method
                                                 * returns a NULL pointer. The default value "WAIT4EVER"
                                                 * makes the thread wait indefinitely until there is an
                                                 * event in his queue. In the opposite, to peek the queue to
                                                 * see if is there an event without waiting it's possible call
                                                 * pullOut with maxWaitMsec set to zero.
                                                 * ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
                                                 * Metodo che restituisce il primo evento disponibile sulla coda del thread
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
    Event* pullOut(const std::string& label, int maxWaitMsec = NOWAIT);/* This variant works as the previous one
                                                                        * but ignores events other than the
                                                                        * specified label. The other events are
                                                                        * simply left in the queue. The only one
                                                                        * event that is notified to the caller is
                                                                        * the "StopThreadEvent". Keep in mind that
                                                                        * "StopThreadEvent" has priority over all
                                                                        * other events, so "label" is notified only
                                                                        * if there isn't a StopThreadEvent (if both
                                                                        * "StopThreadEvent" and "label" are present
                                                                        * into the thread queue then it's
                                                                        * "StopThreadEvent" that pullOut returns to
                                                                        * the caller). The default value for
                                                                        * maxWaitMsec is now set to zero (no wait).
                                                                        * This decision is due to avoid ambiguous
                                                                        * situation (what if the desired event is not
                                                                        * in queue? Have we to wait indefinitely and
                                                                        * ignore all other events? Clearly not: this
                                                                        * would lead to starvation, then if we want to
                                                                        * check the queue for a specified event then we
                                                                        * have also to give a finite max wait time).
                                                                        * ::::::::::::::::::::::::::::::::::::::::::::::::
                                                                        * Metodo che restituisce il primo evento disponib-
                                                                        * ile sulla coda del tipo richiesto, se presente
                                                                        * entro il tempo prestabilito. Se l'evento non viene
                                                                        * ricevuto entro il timeout, il metodo restituisce
                                                                        * NULL. Fa eccezione l'evento "StopThreadEvent" che
                                                                        * se presente in coda viene notificato comunque.
                                                                        */
    Event* pullOut(std::deque <std::string> labels, int maxWaitMsec = NOWAIT);/* This variant is similar to the previous
                                                                               * one but instead of waiting for a specified
                                                                               * label returns the first available event in
                                                                               * the labels list (with the same conditions
                                                                               * as the pullOut version with only one label,
                                                                               * i.e. maxWaitMsec cannot be set to indefinite
                                                                               * wait and "StopThreadEvent" has the precedence
                                                                               * on all other events)
                                                                               * :::::::::::::::::::::::::::::::::::::::::::::
                                                                               * Metodo che restituisce il primo evento
                                                                               * disponibile sulla coda del tipo di uno
                                                                               * qualsiasi appartenente alla lista labels
                                                                               * degli eventi. Se l'evento non viene ricevuto
                                                                               * entro il timeout, il metodo restituisce NULL.
                                                                               * Fa eccezione l'evento "StopThreadEvent" che,
                                                                               * se presente in coda, viene notificato comunque.
                                                                               */
   /* IMPORTANT: the Stop() method sends the StopThreadEvent and blocks until the target thread exits. This is necessary to
    * ensure that the thread encapsulating object can be destroyed. For this reason the StopThreadEvent must be always
    * returned by pullOut if present, even if not in the desired event list
    */
	/*
	30/01/16 eliminato metodo alive. Infatti se chiamato su un thread già terminato (ovvero esattamente come lo utilizzato,
	per verificare nel distruttore se il thread stava ancora girando) il _th non e` piu` valido e il comportamento della
	pthread_kill e` indefinito (mi e` successo che causasse segmentation fault...)
    bool alive();//tests if thread is alive
                 //dice se il thread e` vivo
	*/
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
#ifndef arm //pthread_setname_np e` utilizzabile con glibc>2.12 nella nostra toolchain la versione e` inferiore...
#ifdef _ZDEBUG
    void Start(const std::string& threadName);//assegna un nome al thread per facilitare il debugging
#endif
#endif
    void Stop();/* IMPORTANT: Stop() should never be called inside run()
                 * ::::::::::::::::::::::::::::::::::::::::::::::::
                 * IMPORTANTE: Stop() non DEVE MAI essere chiamata dentro run()
                 */
    void Join();/* IMPORTANT: Join() should never be called inside run()
                 * ::::::::::::::::::::::::::::::::::::::::::::::::
                 * IMPORTANTE: Join() non DEVE MAI essere chiamata dentro run()
                 */ 
    void register2Label(const std::string& label);
private:
    /*
     * the well known big 3 rule of C++ says:
     * "if your class needs to define the destructor or the copy constructor or the copy assigment operator then it should
     * probably explicitly define all three" In our case since a Thread cannot be assigned or copied, we declare the copy
     * constructor and the assignment operator as private. This way we can prevent and easily detect any inadverted abuse
     * ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
     * secondo la "Law of The Big Three" del c++, se si definisce uno tra: distruttore, costruttore di copia o operatore di
     * assegnazione, probabilmente occorrera' definire anche i due restanti (e non usare quelli di default cioe`!). Tuttavia
     * in questo caso un Thread non puo` essere assegnato o copiato, per cui il rispetto della suddetta regola avviene
     * rendendoli privati in modo da prevenirne un utilizzo involontario!
     */
    Thread(const Thread &);//non ha senso ritornare o passare come parametro un thread per valore
    Thread & operator = (const Thread &);//non ha senso assegnare un thread per valore
};
//-------------------------------------------------------------------------------------------
//AutonomousThread
/*
    AutonomousThread is a thread characterized by

    a) it does no receive any event. Therefore the methods register2Label, pullOut are not available

    b) it's stopped by call to Stop()

    c) Does not allow Join() to prevent deadlock risk.
       To understand the reason, consider the following scenario:
       let A be an object Thread instance of type AutonomousThread. Let B be a "normal" Thread
       object. Suppose there are no more threads. If B joins A then B waits A to end, but A will
       never end since nobody can stop him! (the only one who could was B but joined....).

    d) the pure virtual method singleLoopCycle is the function that implements the SINGLE LOOP CYCLE
       of AutonomousThread. This function is continuously repeated until the Stop() method is called.

    REM: singleLoopCycle shall NOT be an infinite loop

    :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

    AutonomousThread e` la classe base per un tipo particolare di Thread, caratterizzato da:

    a) non riceve nessun evento, pertanto non sono disponibili i metodi register2Label, pullOut

    b) viene stoppato tramite la chiamata Stop()

    c) non permette il Join() (dichiarato private quindi) per evitare il rischio di un deadlock.
       Per capirne la ragione, ipotizziamo lo scenario seguente:
       sia A un thread di tipo AutonomousThread (ovvero istanza di una classe derivata da AutonomousThread
       che di suo non e` istanziabile essendo astratta). Sia B un thread "normale" (derivato da Thread).
       Supponiamo che non ci siano altri thread. Se B fa Join() su A, B aspetta che A termini, ma A non
       terminera` mai perche` nessuno puo` stopparlo (l'unico era B ma ha fatto Join()...). Chiaramente
       potrebbe esserci un terzo thread C che stoppi A sbloccando anche B, ma il rischio di avere un
       deadlock e` troppo elevato per lasciare disponibile il metodo Join

    b) il metodo puramente virtuale singleLoopCycle e` la funzione che implementa il SINGOLO CICLO del
       thread AuthonomousThread. Tale metodo viene ripetuto all'infinito sinche` non viene chiamato lo
       Stop() del thread. singleLoopCycle NON DEVE CONTENERE UN CICLO INFINITO
*/
class AutonomousThread {
    pthread_t _th;
    pthread_mutex_t _lockSet;
    pthread_mutex_t _lockJoin;
    bool _joinedOnce;
    bool _set;
    bool exit;
	/*30/01/16 eliminato metodo alive. Infatti se chiamato su un thread già terminato (ovvero esattamente come lo utilizzato,
	per verificare nel distruttore se il thread stava ancora girando) il _th non e` piu` valido e il comportamento della
	pthread_kill e` indefinito (mi e` successo che causasse segmentation fault...)
    bool alive();//dice se il thread e` vivo*/
    static void *ThreadLoop(void * arg);
    void run();
    virtual void singleLoopCycle()=0;
    void Join();//not available in AutonomousThread... it's used only internally
protected:
    AutonomousThread();
public:
    ~AutonomousThread();
    void Start();
#ifndef arm //pthread_setname_np e` utilizzabile con glibc>2.12 nella nostra toolchain la versione e` inferiore...
#ifdef _ZDEBUG
    void Start(const std::string& threadName);//assegna un nome al thread per facilitare il debugging
#endif
#endif
    void Stop();
private:
    /*
     * the well known big 3 rule of C++ says:
     * "if your class needs to define the destructor or the copy constructor or the copy assigment operator then it should
     * probably explicitly define all three" In our case since a Thread cannot be assigned or copied, we declare the copy
     * constructor and the assignment operator as private. This way we can prevent and easily detect any inadverted abuse
     * ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
     * secondo la "Law of The Big Three" del c++, se si definisce uno tra: distruttore, costruttore di copia o operatore di
     * assegnazione, probabilmente occorrera' definire anche i due restanti (e non usare quelli di default cioe`!). Tuttavia
     * in questo caso un Thread non puo` essere assegnato o copiato, per cui il rispetto della suddetta regola avviene
     * rendendoli privati in modo da prevenirne un utilizzo involontario!
     */
    AutonomousThread(const AutonomousThread &);//non ha senso ritornare o passare come parametro un thread per valore
    AutonomousThread & operator = (const AutonomousThread &);//non ha senso assegnare un thread per valore
};
//-------------------------------------------------------------------------------------------
//SameThreadAddressSpaceEventReceiver
/*
    IMPORTANT!
    An oject instance of SameThreadAddressSpaceEventReceiver MUST BE USED ONLY BY ONE SINGLE
    external thread, otherwise the behaviour will be unpredictable.

    explanation:
    SameThreadAddressSpaceEventReceiver provides the ability to receive Events in the SAME
    ADDRESS SPACE where is located. An object of type SameThreadAddressSpaceEventReceiver has
    one event queue and the methods to get event from that queue. If more threads use the same
    object instance, they will use the same event queue too. If an event becomes available, the
    first thread who arrives it extracts that event, leaving the others empty-handed! Moreover,
    there is no way to predict who will be that thread! Obviously, there is no problem in using
    more than one SameThreadAddressSpaceEventReceiver in the same thread and/or in different
    threads, the only limitation beeing that said above that is each object must be used by one
    thread (who can use more objects, and more threads can use their own instances).

    :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

    NOTA IMPORTANTE:
    un oggetto di tipo SameThreadAddressSpaceEventReceiver DEVE ESSERE USATO SOLO DA UN THREAD
    ESTERNO. La chiamata dei metodi di un oggetto di tipo SameThreadAddressSpaceEventReceiver
    da parte di piu` thread contemporaneamente provoca risultati impredicibili.

    Spiegazione:

    SameThreadAddressSpaceEventReceiver e` la classe base che fornisce i metodi per ricevere
    eventi NELLO STESSO SPAZIO DI INDIRIZZAMENTO nella quale e` istanziata.

    Il nome "SameThreadAddressSpaceEventReceiver" vuole sottolineare questa avvertenza
    importante.

    IMPORTANTE: lo scopo di SameThreadAddressSpaceEventReceiver e` quello di fornire un modo
    per permettere l'iterazione tra un thread "non zibaldone" con il mondo thread zibaldone.
    una classe derivata da SameThreadAddressSpaceEventReceiver NON E` MULTI-THREAD SAFE.
    Affinche` il risultato sia quello atteso occorre che un istanza di un oggetto derivato
    da SameThreadAddressSpaceEventReceiver sia utilizzata da UN SOLO THREAD (ovviamente nulla
    vieta di avere piu` istanze di oggetti derivati da) SameThreadAddressSpaceEventReceiver
    a condizione che ognuno di questi oggetti sia utilizzato dal proprio thread dedicato.

    Tipicamente un thread non appartenente al mondo zibaldone, utilizza un oggetto di tipo
    SameThreadAddressSpaceEventReceiver per implementare dei metodi che interagiscano con
    il mondo dei thread zibaldone inviando eventi di richiesta e ricevano le risposte all'in-
    terno del metodo stesso, in modo che il chiamate riceva la risposta come se questa fosse
    stata eseguita nel metodo stesso, senza sapere che in realta` e` avvenuta l'interazione
    con un altro thread. Ovviamente il chiamante deve essere consapevole che il metodo non
    ritornera` immediatamente dovendo attendere via pullOut la risposta dal thread alla quale
    ha fatto la richiesta con la emitEvent.

    Il motivo per cui l'istanza di un oggetto di tipo SameThreadAddressSpaceEventReceiver non
    puo` essere utilizzata da piu` thread contemporaneamente e` che (vedi anche OSSERVAZIONI -
    punto 2) la coda e` una sola, e sebbene gestita correttamente con semafori, se piu` thread
    fossero in attesa di un evento, solamente il primo arrivato lo estrarrebbe, mentre gli
    altri non saprebbero nulla. Inoltre eventuali modifiche di variabili membro interne
    avverrebbero in modo concorrenziale senza protezione.

    Avevo persato di introdurre un semaforo con due metodi per entrare e uscire da una sezione
    critica, ma cio` puo` generare dei deadlock se l'utilizzatore della classe inserisse la
    pullOut all'interno di una sezione critica.

    La classe fornisce a chi la deriva tutta l'infrastruttura (coda eventi, gestione accesso
    concorrente, ...) per interagire con i thread NELLO STESSO SPAZIO DI INDIRIZZAMENTO in
    cui si trova l'istanza della classe. Infatti la gestione degli eventi avviene esattamente
    come per i thread, questo significa che occorre essere certi del fatto che il thread
    produttore degli eventi ai quali ci si registra risieda nello stesso spazio di
    indirizzamento. Questo risultato tipicamente puo` essere ottenuto istanziando e lanciando
    i thread collaboranti all'interno della classe stessa, in modo che quando viene istanziato
    un oggetto di tipo SameThreadAddressSpaceEventReceiver (ovvero derivato da tale classe)
    questo istanzi nello stesso spazio di indirizzamento anche i thread con i quali vuole
    collaborare. In altre parole, SameThreadAddressSpaceEventReceiver NON E` UN THREAD per cui
    i metodi che vengono chiamati girano nel thread del chiamante! Affinche` il risultato
    sia quello atteso (ricezione degli eventi desiderati) occorre che il chiamante condivida
    lo spazio di indirizzamento con i thread coinvolti.

    Riassumendo, usare una stessa istanza di un oggetto di tipo SameThreadAddressSpaceEventReceiver
    in piu` thread porta a risultati impredicibili.

    In questo contesto risulta ancora piu` chiaro il perche` della raccomandazione di chiamare la
    pullOut di un Thread solo dentro la run: perche` cosi` facendo si garantische che ogni oggetto
    di tipo Thread veda chiamate le proprie pullOut solo all'interno di un thread (il suo) evitando
    situazioni come quella evidenziata qui sopra relativa all'estrazione in ordine non prevedibile
    degli eventi dalla coda da parte di piu` thread.

    Utilizzando invece la pullOut solo dentro la run si garantisce che l'interazione del thread con
    il mondo esterno avvenga solo tramite eventi: la run fa tutto e la classe che la contiene viene
    utilizzata come supporto.

    Nel caso di SameThreadAddressSpaceEventReceiver si puo` pensare che tutto funzioni correttamente
    se e solo se gli viene "prestato" solo un thread esterno che fa le veci di run()
*/
class SameThreadAddressSpaceEventReceiver : private Thread {
    void run(){}
protected:
    /*
     *  SameThreadAddressSpaceEventReceiver "ripubblica" (sempre con modalita` di accesso protected)
     *  il sottoinsieme dei metodi di Thread che sono necessari alla gestione della coda di eventi.
     *  I metodi relativi alla gestione del ciclo del Thread (run, Start, Stop, ...) non devono essere
     *  mai chiamati per cui vengono riproposti in modalita` private (non accessibili dal derivante).
     *  Il metodo run (puramente virtuale nella classe base Thread, e che quindi deve essere
     *  obbligatoriamente implementato) non fa nulla, dato che non puo` essere creato nessun thread
     *  in SameThreadAddressSpaceEventReceiver essendo Start ad accesso private (la pthread_create
     *  viene chiamata dentro Start). Infine osserviamo che nemmeno Stop puo` essere chiamata, dato
     *  che il ciclo di vita del thread e` governato dal chiamante.
     *  Praticamente SameThreadAddressSpaceEventReceiver semplicemente rende disponibile al derivante
     *  l'infrastruttura per ricevere eventi dai thread istanziati derivando Thread senza interferire
     *  in nessun modo con il resto delle funzionalita` della classe derivante che restano inalterate.
     *
     *
     *  OSSERVAZIONI:
     *
     *  1) Il metodo pullOut senza timeout non viene reso disponibile. Infatti non sarebbe corretto
     *     bloccare per un tempo indefinito un metodo che viene dichiarato come non appartenente ad
     *     un thread ben definito! Inoltre, poiche` l'accesso alla coda avviene all'interno del thread
     *     chiamante, se il processo che istanzia un oggetto di tipo SameThreadAddressSpaceEventReceiver
     *     utilizzasse piu`riferimenti a tale oggetto in maniera concorrenziale all'interno di suoi
     *     thread (piu` puntatori al medesimo oggetto), tali thread si troverebbero a lockare la stessa
     *     risorsa (la coda degli eventi fornita da SameThreadAddressSpaceEventReceiver). Sebbene la
     *     cosa potrebbe essere fatta consapevolmente dal chiamante, e` opportuno richiedere a quest'
     *     ultimo l'esplicita impostazione di un timeout finito (anche arbitrariamente lungo).
     *
     *  2) Se piu` thread utilizzassero una stessa istanza di SameThreadAddressSpaceEventReceiver, di
     *     fatto sharerebbero una stessa coda di eventi (quella presente nell'istanza condivisa di
     *     SameThreadAddressSpaceEventReceiver). Dal punto di vista formale del controllo dell'accesso
     *     realizzato tramite il lock del mutex _lockQueue, questo fatto non costituirebbe un problema:
     *     il mutex e` li esattamente per questo! A titolo di esempio, si pensi all'emitEvent, che fa
     *     lock su tale mutex prima di inserire un nuovo evento in coda!
     *     La differenza pero` e` che nel caso della emitEvent, piu` thread contemporaneamente cercano
     *     di scrivere in coda (e il mutex ne regola l'accesso), mentre nel caso di piu` riferimenti
     *     contemporanei allo stesso oggetto di tipo SameThreadAddressSpaceEventReceiver da parte di
     *     piu` thread, l'accesso concorrenziale viene fatto in lettura (la emitEvent scrive in coda,
     *     la pullOut legge dalla coda). Questo accesso concorrenziale in lettura genera situazioni non
     *     predicibili, dato che viene meno l'ipotesi (piu` produttori ma un solo consumatore) su cui
     *     si basa il paradigma di gestione degli Eventi via Observer. Per convincersene, basta pensare
     *     che quando piu` thread cercano di estrarre lo stesso evento dalla stessa coda (magari
     *     semplicemente il primo disponibile) in realta` solo il primo thread che arriva estrae l'evento
     *     (e non e` dato sapere a priori chi, poiche` dipende dallo scheduler) mentre gli altri restano
     *     "a bocca asciutta" (l'evento non c'e` piu` perche` l'ha preso un'altro arrivato prima) e si
     *     genera la classica situazione di starvation. Non e` nemmeno garantito che prima o poi tutti
     *     ricevano un evento, dato che potrebbe arrivare per primo sempre lo stesso thread, lasciando
     *     gli altri thread fermi per tempo indefinito sul mutex (starvation degenerata in deadlock).
     */

    SameThreadAddressSpaceEventReceiver();
    ~SameThreadAddressSpaceEventReceiver();

    //In SameThreadAddressSpaveEventReceiver it's not available the pullOut method without arguments
    //Nella SameThreadAddressSpaceEventReceiver non viene resa disponibile la chiamata a pullOut senza parametri.
    Event* pullOut(int maxWaitMsec);/* returns the first available event in the thread queue waiting at least for
                                     *"maxWaitMsec" msecs. If no event is in the queue within that time the method
                                     * returns a NULL pointer.
                                     * Note: to peek the queue to see if is there an event without waiting it's
                                     * possible call pullOut with maxWaitMsec set to zero.
                                     * ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
                                     * Metodo che restituisce il primo evento disponibile sulla coda del thread
                                     * aspettando al massimo "maxWaitMsec" millisecondi: se non arriva nessun
                                     * evento entro tale timeout la funzione restituisce NULL.
                                     * Nota: se maxWaitMsec = 0, pullOut controlla se c'e' un evento gia' in
                                     * coda (peek) e ritorna subito.
                                     */
    Event* pullOut(const std::string& label, int maxWaitMsec = NOWAIT);/* Waits for the event identified by label
                                                                        * ::::::::::::::::::::::::::::::::::::::::::::::::::::::::
                                                                        * Metodo che restituisce il primo evento disponibile sulla
                                                                        * coda del tipo richiesto, se presente entro il tempo
                                                                        * prestabilito. Se l'evento non viene ricevuto entro il
                                                                        * timeout, il metodo restituisce NULL. Fa eccezione l'evento
                                                                        * "StopThreadEvent" che se presente in coda viene notificato
                                                                        * comunque.
                                                                        */
    Event* pullOut(std::deque <std::string> labels, int maxWaitMsec = NOWAIT);/* Waits for the first available event between
                                                                               * those identified by labels
                                                                               * :::::::::::::::::::::::::::::::::::::::::::::
                                                                               * Metodo che restituisce il primo evento
                                                                               * disponibile sulla coda del tipo di uno
                                                                               * qualsiasi appartenente alla lista labels
                                                                               * degli eventi. Se l'evento non viene ricevuto
                                                                               * entro il timeout, il metodo restituisce NULL.
                                                                               * Fa eccezione l'evento "StopThreadEvent" che, se
                                                                               * presente in coda, viene notificato comunque.
                                                                               */
public:
    void register2Label(const std::string& label);
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
    static void mapLabel2Receiver(const std::string& label, Thread *T);
    static void unmapReceiver(Thread *T);
public:
    EventManager();
    //~EventManager();
};
//-------------------------------------------------------------------------------------------
}//namespace Z
#endif	/* _THREAD_H */
