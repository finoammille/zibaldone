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
public class SameThreadAddressSpaceEventReceiver {
    private class facade4thread extends Thread {
        public void run(){}
    }
    facade4thread f4t;
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

    protected SameThreadAddressSpaceEventReceiver()//lo specificatore di accesso e` protected in modo che la classe sia 
                                                   //a tutti gli effetti un'interfaccia per il mondo esterno a zibaldone
    {
        f4t = new facade4thread();
        f4t.Start();//necessario per impostare _set a true e abilitare la coda alla ricezione degli eventi.
                    //In realta` il thread termina subito, non essendoci un ciclo infinito (run e` vuota)
    }

    //In SameThreadAddressSpaveEventReceiver it's not available the pullOut method without arguments
    //Nella SameThreadAddressSpaceEventReceiver non viene resa disponibile la chiamata a pullOut senza parametri.

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
    protected Event pullOut(int maxWaitMsec) {return f4t.pullOut(maxWaitMsec);}

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
    protected Event pullOut(String label, int maxWaitMsec) {return f4t.pullOut(label, maxWaitMsec);}

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
    protected Event pullOut(String label) {return f4t.pullOut(label);}

   /* ------------------------------------------------------------------------------------------------------------------
    * This variant returns the first available event within the labels list (the conditions are the same as for the
    * pullOut version with only one label, i.e. "StopThreadEvent" has the precedence on all other events)
    * ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
    * Metodo che restituisce il primo evento disponibile sulla coda del tipo di uno qualsiasi appartenente alla lista
    * labels degli eventi. Se l'evento non viene ricevuto entro il timeout, il metodo restituisce NULL. L'evento
    * "StopThreadEvent" se presente in coda, viene notificato comunque.
    */
    protected Event pullOut(ArrayDeque<String> labels, int maxWaitMsec) {return f4t.pullOut(labels, maxWaitMsec);}
    
   /* ------------------------------------------------------------------------------------------------------------------
    * This variant returns the first event, within the labels list, if available in the queue or null if is not present
    * in the queue. This method does not wait but returns immediately. The "StopThreadEvent" has priority over all other
    * events, so "label" is notified only if there isn't a StopThreadEvent
    * ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
    * Metodo che restituisce il primo evento disponibile sulla coda del tipo di uno qualsiasi appartenente alla lista
    * labels degli eventi oppure null nel caso in cui non sia persente. Fa eccezione l'evento "StopThreadEvent" che, se
    * presente in coda, viene notificato comunque.
    */
    protected Event pullOut(ArrayDeque<String> labels) {return f4t.pullOut(labels);}

    /*
     * the register2Label method registers the thread to the event identified by the specified label. After register2Label
     * has been called on the thread whenever any thread emits an event with the specified label the registered thread 
     * will receive a copy of that event in his event queue.
     * ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
     * il metodo register2Label registra il thread sull'evento identificato dalla label specificata. Dopo che la 
     * register2Label e` stata chiamata sul thread, non appena un qualsiasi thread emette un evento con la label specificata
     * il thread registrato ricevera` una copia di tale evento sulla sua coda eventi
     */
    public void register2Label(String label) {f4t.register2Label(label);}
};
