/*
 *
 * zibaldone - a C++/Java library for Thread, Timers and other Stuff
 *
 * http://sourceforge.net/projects/zibaldone/
 *
 * version 3.1.2, August 29th, 2015
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

package Z;

import java.util.*;
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantLock;
import java.util.concurrent.locks.Condition;
import java.util.concurrent.TimeUnit;
import java.util.logging.*;

/*

IMPORTANT!!

The methods alive(), Start(), Join() and Stop() of the following Thread class should never be called inside the child class constructor.
Infact the above mentioned methods presuppose a fully constructed child class because they are based on the existence of the instance of 
the "_th" member of the same class. The "_th" member is an instance of java.lang.thread to whose constructor it has been passed "this" 
as argument, where, in that context, "this" means the fully constructed (alive and kicking!) instance of the Runnable child class of 
with a well formed run method.

For example, THE FOLLOWING IS WRONG:

class foo extends Thread {
    ...
    public foo()
    {
        //foo constructor
        ...
        Start(); // <= ERROR!
        ...
    }
    ...
}

Obviously, because of what we have yet said, if a class "foo" inherited from Z.Thread have a member object "john" of type "doo" (inherited
from Z.Thread too), there is no contraindication about calling any of the above mentioned methods on the object "john" inside the constructor 
of "foo" (after john has been instantiated). For example the following is ok:

class doo extends Thread {
    ...
}

class foo extends Thread {
    ...
    doo john;
    ...
    public foo()
    {
        //foo constructor
        ...
        john = new doo();
        john.Start(); // <= THIS IS OK!
        ...
    }
    ...
}

*/

public abstract class Thread implements Runnable {
    private final ReentrantLock _lockQueue = new ReentrantLock();
    private final ReentrantLock _lockSet = new ReentrantLock();/* mutex utilizzato per garantire l'unicità del thread associato alla classe.
                                                                * viene utilizzato dentro il metodo Start() per prevenire i danni causati da
                                                                * un'eventuale doppia chiamata di Start(). Infatti se ciò accadesse, verrebbero
                                                                * creati due thread indipendenti e identici (stesso ciclo run()) ma che condivi-
                                                                * derebbero gli stessi dati all'interno della classe con corse, problemi di
                                                                * scrittori/lettori, starvation, deadlock...
                                                                */
    private final Condition _thereIsAnEvent = _lockQueue.newCondition();
    private HashMap<String, ArrayDeque<Event>> _eventQueue=new HashMap<String,ArrayDeque<Event>>();
    private ArrayDeque<String> _chronologicalLabelSequence=new ArrayDeque<String>();/* lista ordinata dei puntatori alle code da utilizzare
                                                                                     * per estrarre gli eventi in ordine cronologico
                                                                                     */
    protected Thread(){_set=false;}
   /*
    * NOTA IMPORTANTE: occorre garantire che il thread sia creato UNA VOLTA SOLA per ogni oggetto di tipo thread istanziato.
    * in caso contrario infatti avrei più thread indipendenti che però condividerebbero gli stessi dati incapsulati nella
    * classe associata al thread. Occorre quindi prevenire che una doppia chiamata a Start sullo stesso oggetto comporti
    * la creazione di due thread (il thread va creato solo LA PRIMA VOLTA che viene chiamato il metodo Start().
    */
    private boolean _set;
    private java.lang.Thread _th;

    void pushIn(Event ev)
    {
        _lockSet.lock();//se sto facendo pushIn di StopThreadEvent, ovvero sono dentro Stop() il thread sta 
                        //lockando due volte _lockSet... nessun problema pero` perche` _lockSet e` reentrant!
        try {
            if(_set == true) {
                _lockQueue.lock();
                try {
                    ArrayDeque<Event> evts=_eventQueue.get(ev.label());
                    if(evts==null) {
                        evts=new ArrayDeque<Event>();
                        _eventQueue.put(ev.label(), evts);
                    }
                    evts.add(ev);
                    _chronologicalLabelSequence.addFirst(ev.label());
                    _thereIsAnEvent.signal();
                } finally {
                    _lockQueue.unlock();
                }
            }
        } finally {
            _lockSet.unlock();
        }
    }
   /* ------------------------------------------------------------------------------------------------------------------
    * pullOut methods are protected because they must be called only inside the thread loop (run() method).
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
   /*
    * This method returns the first available event in the thread queue waiting at least for "maxWaitMsec" msecs.
    * If no event is in the queue within that time the method returns a NULL pointer. To peek the queue to see if
    * is there an event without waiting it's possible call pullOut with maxWaitMsec set to zero.
    * ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
    * Metodo che restituisce il primo evento disponibile sulla coda del thread aspettando al massimo "maxWaitMsec"
    * millisecondi: se non arriva nessun evento entro tale timeout la funzione restituisce NULL.
    */
    protected Event pullOut(int maxWaitMsec)
    {
        Event ev=null;
        _lockQueue.lock();//accesso esclusivo alla coda...
        try {
            if (_eventQueue.isEmpty()) {//se la coda e' vuota...
                boolean spuriousWakeup=false;
                do {
                    try {
                        if(!_thereIsAnEvent.await(maxWaitMsec, TimeUnit.MILLISECONDS)) return null;//coda vuota e non e` arrivato nulla.
                    } catch (InterruptedException ex) {
                        LOG.ziblog(LOG.LogLev.ERR, "InterruptedException(%s)", ex.getMessage());
                        return null;
                    }
                    //if we are here it means the condition has became true. We have only to check it to be sure it's not a spurious wakeup
                    if(_chronologicalLabelSequence.isEmpty()) spuriousWakeup=true;
                    else spuriousWakeup=false;
                } while (spuriousWakeup);
            }
            String eventLabel=_chronologicalLabelSequence.removeFirst();
            ArrayDeque<Event> evQ = _eventQueue.get(eventLabel);
            ev=evQ.removeFirst();
            if(evQ.isEmpty()) _eventQueue.remove(eventLabel);//se non ci sono piu' eventi di questo tipo ri-,
                                                             //muovo la relativa lista dalla mappa della coda
            return ev;
        } finally {
            _lockQueue.unlock();
        }
    }
   /* ------------------------------------------------------------------------------------------------------------------
    * This method returns the first available event in the thread queue waiting
    * indefinitely until there is an event in his queue
    * ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
    * Metodo che restituisce il primo evento disponibile sulla coda del thread
    * restando indefinitamente in attesa sulla coda sinche' non arriva un evento.
    */
    protected Event pullOut()
    {
        Event ev=null;
        _lockQueue.lock();//accesso esclusivo alla coda...
        try {
            if (_eventQueue.isEmpty()) {//se la coda e' vuota...
                boolean spuriousWakeup=false;
                do {
                    try {
                        _thereIsAnEvent.await();
                    } catch (InterruptedException ex) {
                        LOG.ziblog(LOG.LogLev.ERR, "InterruptedException(%s)", ex.getMessage());
                        return null;
                    }
                    //if we are here it means the condition has became true. We have only to check it to be sure it's not a spurious wakeup
                    if(_chronologicalLabelSequence.isEmpty()) spuriousWakeup=true;
                    else spuriousWakeup=false;
                } while (spuriousWakeup);
            }
            String eventLabel=_chronologicalLabelSequence.removeFirst();
            ArrayDeque<Event> evQ = _eventQueue.get(eventLabel);
            ev=evQ.removeFirst();
            if(evQ.isEmpty()) _eventQueue.remove(eventLabel);//se non ci sono piu' eventi di questo tipo ri-,
                                                             //muovo la relativa lista dalla mappa della coda
            return ev;
        } finally {
            _lockQueue.unlock();
        }
    }
   /* ------------------------------------------------------------------------------------------------------------------
    * NOTA IMPORTANTE SU pullOut con filtro sugli eventi: il metodo Stop() invia l'evento StopThreadEvent e resta
    * BLOCCATO sinche` il thread target non termina. Questa scelta e` necessaria per garantire che l'oggetto che
    * incapsula il thread possa essere distrutto: ho la certezza di poter distruggere l'oggetto senza danni una volta
    * che il metodo Stop() e` ritornato perche' ho la certezza che a quel punto il thread e` terminato. Infatti serve a
    * garantire che i dati della classe su cui si appoggia il ciclo del thread esistano sinche` il thread non termina.
    * Per questo motivo e` indispensabile che l'evento StopThreadEvent sia notificato sempre quando presente, e che sia
    * gestito subito onde evitare possibili starvation e nei casi piu' gravi deadlock (si pensi per esempio al caso di un
    * thread "A" che chiama Stop() su un thread "B" e si blocchi in attesa che "A" termini mentre invece "B" sta attendendo
    * eventi di un certo tipo (e solo quelli) da "A": "A" non emettera` nessun evento e "B" non terminera` mai => "A" resta
    * bloccato all'infinito con "B".
    */
   /*
    * This variant works as the previous one but ignores events other than the specified label. The other events are simply left in
    * the queue. The only one event that is notified to the caller is the "StopThreadEvent". Keep in mind that "StopThreadEvent"
    * has priority over all other events, so "label" is notified only if there isn't a StopThreadEvent (if both "StopThreadEvent"
    * and "label" are present into the thread queue then it's "StopThreadEvent" that pullOut returns to the caller).
    * ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
    * Metodo che restituisce il primo evento disponibile sulla coda del tipo richiesto, se presente entro il tempo prestabilito. Se
    * l'evento non viene ricevuto entro il timeout, il metodo restituisce NULL. L'evento "StopThreadEvent", se presente in coda,
    * viene notificato comunque anche se non nella lista.
    */
    protected Event pullOut(String label, int maxWaitMsec)
    {
        ArrayDeque<String> labels=new ArrayDeque<String>();
        labels.addFirst(label);
        return pullOut(labels, maxWaitMsec);
    }
   /* ------------------------------------------------------------------------------------------------------------------
    * This variant returns the first event, with label equal to that specified in the argument, if available in the
    * queue or null if that event is not present in the queue. This method does not wait but returns immediately. The
    * "StopThreadEvent" has priority over all other events, so "label" is notified only if there isn't a StopThreadEvent
    * (if both "StopThreadEvent" and "label" are present into the thread queue then it's "StopThreadEvent" that pullOut
    * returns to the caller).
    * ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
    * Metodo che restituisce il primo evento disponibile sulla coda del tipo richiesto, se presente oppure null nel caso
    * in cui non sia presente. Fa eccezione l'evento "StopThreadEvent"  che se presente in coda viene notificato comunque.
    */
    protected Event pullOut(String label)
    {
        ArrayDeque<String> labels=new ArrayDeque<String>();
        labels.addFirst(label);
        return pullOut(labels);
    }
   /* ------------------------------------------------------------------------------------------------------------------
    * This variant returns the first available event within the labels list (the conditions are the same as for the
    * pullOut version with only one label, i.e. "StopThreadEvent" has the precedence on all other events)
    * ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
    * Metodo che restituisce il primo evento disponibile sulla coda del tipo di uno qualsiasi appartenente alla lista
    * labels degli eventi. Se l'evento non viene ricevuto entro il timeout, il metodo restituisce NULL. L'evento
    * "StopThreadEvent" se presente in coda, viene notificato comunque.
    */
    protected Event pullOut(ArrayDeque<String> labels, int maxWaitMsec)
    {
        labels.addFirst(StopThreadEvent.StopThreadLabel);//come spiegato sopra, DEVO sempre e comunque gestire StopThreadEvent
        if(maxWaitMsec<0) {
            LOG.ziblog(LOG.LogLev.WRN, "invalid maxWaitMsec (%d) ... set to default value of 1 sec to prevent starvation...", maxWaitMsec);
            maxWaitMsec=1000;
        }
        boolean timeout=false;
        long maxWaitNsec = TimeUnit.MILLISECONDS.toNanos(maxWaitMsec);
        Event ev=null;
        while(!timeout) {
            _lockQueue.lock();//accesso esclusivo alla coda...
            try {
                if (!_eventQueue.isEmpty()) {//se la coda non e` vuota vedo se c'e` un evento del tipo richiesto
                    Iterator<String> it=labels.iterator();
                    while(it.hasNext()) {
                        String label=it.next();
                        ArrayDeque<Event> evQ = _eventQueue.get(label);
                        if(evQ!=null) {//trovato un evento che risponde ai requisiti!
                            ev=evQ.removeFirst();
                            if(evQ.isEmpty()) _eventQueue.remove(label);//quello appena estratto era l'ultimo evento di quel tipo in coda
                            if(!_chronologicalLabelSequence.remove(label)) LOG.ziblog(LOG.LogLev.ERR, "event %s was not present in chronological sequence", label);
                            return ev;
                        }
                    }
                }
                //se sono qui significa che:
                //a) l'evento richiesto non c'e` perche` la coda e` vuota oppure perche` l'evento richiesto non e` presente
                //b) ho ancora tempo disponibile per aspettare sulla coda se arriva l'evento (timeout=false)
                //N.B.: eventuali spurious wakeup sono automaticamente gestiti perche` prima di procedere controllo comunque
                //      se la coda _eventQueue e` vuota e nel caso ripeto await (timeout > 0)
                try {
                    maxWaitNsec=_thereIsAnEvent.awaitNanos(maxWaitNsec);
                    timeout = (maxWaitNsec>0);
                } catch (InterruptedException ex) {
                    LOG.ziblog(LOG.LogLev.ERR, "InterruptedException(%s)", ex.getMessage());
                }
            } finally {
                _lockQueue.unlock();
            }
        }
        //uscita per timeout!
        return ev;
    }
   /* ------------------------------------------------------------------------------------------------------------------
    * This variant returns the first event, within the labels list, if available in the queue or null if is not present
    * in the queue. This method does not wait but returns immediately. The "StopThreadEvent" has priority over all other
    * events, so "label" is notified only if there isn't a StopThreadEvent
    * ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
    * Metodo che restituisce il primo evento disponibile sulla coda del tipo di uno qualsiasi appartenente alla lista
    * labels degli eventi oppure null nel caso in cui non sia persente. Fa eccezione l'evento "StopThreadEvent" che, se
    * presente in coda, viene notificato comunque.
    */
    protected Event pullOut(ArrayDeque<String> labels)
    {
        return pullOut(labels, 0);
    }
   /* ------------------------------------------------------------------------------------------------------------------
    * this method stops rx of any event, deregisters from all events and thrashes all events in the event queue
    * ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
    * Interrompe la ricezione di qualsiasi evento deregistrandisi da tutto e elimina tutti gli eventi presenti in coda
    * al momento della chiamata.
    */
    protected void purgeRxQueue()
    {
        _lockQueue.lock();
        try {
            Iterator<Map.Entry<String, ArrayDeque<Event>>> it=_eventQueue.entrySet().iterator();
            while(it.hasNext()) it.next().getValue().clear();//ripulisco la coda degli eventi del thread
            _eventQueue.clear();//ho ripulito gli eventi per ogni tipo, ora ripulisco i tipi (nomi degli eventi)
        } finally {
            _lockQueue.unlock();
        }
    }

    //N.B.: alive() should never be called inside the child class constructor
    protected boolean alive() {return (_th!=null) ? _th.isAlive() : false;}//tests if thread is alive
                                                                          //dice se il thread e` vivo
    
    //N.B.: Start() should never be called inside the child class constructor
    public void Start()
    {
        _lockSet.lock();
        try {
            if(_set == false) {
                _th=new java.lang.Thread(this);
                _th.start();
                _set=true;
            }
        } finally {
            _lockSet.unlock();
        }
    }

    //N.B.: Join() should never be called inside the child class constructor
    public void Join()
    {
        try {
            if(_th!=null) _th.join();
        } catch (InterruptedException ex) {
            LOG.ziblog(LOG.LogLev.ERR, "InterruptedException(%s)", ex.getMessage());
        }
    }

    //N.B.: Stop() should never be called inside the child class constructor
    public void Stop()
    {
        _lockSet.lock();
        try {
            if(_set == true) {
                pushIn(new StopThreadEvent());
                //24.03.16 => superdipendenzacircolare!!!! Ho sostituito la chiamata a Join() (che indendo
                //essere quella di Thread, poche righe sopra) con l'utilizzo diretto di _th.join() perche`
                //in caso di override di Stop, la chiamata di Stop con super da una classe derivata provoca
                //una ricorsione circolare causata dal fatto che entro in Stop di Thread (classe base) con
                //un tipo derivato che poi chiama Join con "la propria impronta" ovvero Join derivato anziche`
                //quello della classe base (che vorrei invece fosse chiamato) se poi (com'e` successo in Udp.java
                //quest'ultimo (il Join derivato) chiama a sua volta Stop... entro in un loop ricorsivo che
                //termina con StackOverflowException!
                try {
                    _th.join();//nota: qui non serve controllare che _th != null perche` se _set=true
                               //(e c'e` il lock a prevenire corse..) allora sicuramente _th non e` null!
                } catch (InterruptedException ex) {
                    LOG.ziblog(LOG.LogLev.ERR, "InterruptedException(%s)", ex.getMessage());
                }
                _set=false;
                _th=null;
                purgeRxQueue();//ripulisco la coda di ricezione
                EventManager.unmapReceiver(this);//rimuovo la registrazione del thread da EventManager
            }
        } finally {
            _lockSet.unlock();
        }
    }
    /*
     * the register2Label method registers the thread to the event identified by the specified label. After register2Label
     * has been called on the thread whenever any thread emits an event with the specified label the registered thread 
     * will receive a copy of that event in his event queue.
     * ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
     * il metodo register2Label registra il thread sull'evento identificato dalla label specificata. Dopo che la 
     * register2Label e` stata chiamata sul thread, non appena un qualsiasi thread emette un evento con la label specificata
     * il thread registrato ricevera` una copia di tale evento sulla sua coda eventi
     */
    public void register2Label(String label)
    {
        EventManager.mapLabel2Receiver(label, this);
    }
    //------------------------------------------------------------------------------------------------------------------
}
