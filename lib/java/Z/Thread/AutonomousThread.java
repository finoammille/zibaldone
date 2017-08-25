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

import java.util.concurrent.locks.ReentrantLock;

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
public abstract class AutonomousThread {
    private final ReentrantLock _lockExit = new ReentrantLock(); 
    private boolean exit=true;
    public abstract void singleLoopCycle();
    private class worker extends Thread {
        public void run() {while(!exit) singleLoopCycle();}
        public void Start(){super.Start();}
        public void Stop(){super.Stop();}
    } 
    worker w;
    public AutonomousThread()
    {
        w = new worker();
    }
    public void Start()
    {
        _lockExit.lock();
        exit=false;
        w.Start();
        _lockExit.unlock();
    }
    public void Stop()
    {
        _lockExit.lock();
        if(!exit) exit=true;
        w.Stop();//in realtà ho già fatto lo stop mettendo exit a true, visto che non gestisco l'evento di stop. 
                 //Chiamo Stop() su w per pulire code e tutto
        _lockExit.unlock();
    }
}



