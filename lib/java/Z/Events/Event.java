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
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantReadWriteLock;


//----------------------------------------------------------------------------------------------------------------------
// Event
public class Event {
    private final String _label;
    public String label() {return new String(_label);}
    public Event(String label)
    {
        if(label.isEmpty()) {
            LOG.ziblog(LOG.LogLev.ERR, "ERROR: an instance of Event cannot have empty label");
            label = "sob";//the event label is mandatory...
        }
        _label= new String(label);
    }

    //when emitEvent() is called, all thread who registered to this event will receive it
    public void emitEvent()
    {
        if(_label.equals(StopThreadEvent.StopThreadLabel)) {
            LOG.ziblog(LOG.LogLev.ERR, "Invalid emitEvent with label=StopThreadLabel (it's reserved). Call Stop() instead");
            return;
        }
        EventManager._rrwLock.readLock().lock();
        try {
            ArrayDeque<Thread> listeners=EventManager._evtReceivers.get(_label);
            if(listeners!=null)
                for(Iterator<Thread> itEvtReceiverList = listeners.iterator(); itEvtReceiverList.hasNext();)
                    itEvtReceiverList.next().pushIn(this);
        } finally {
            EventManager._rrwLock.readLock().unlock();
        }
    }
}
//----------------------------------------------------------------------------------------------------------------------
// EventManager
class EventManager {
    final static ReentrantReadWriteLock _rrwLock = new ReentrantReadWriteLock();
    static HashMap<String, ArrayDeque<Thread>> _evtReceivers=new HashMap<String,ArrayDeque<Thread>>();
    static void mapLabel2Receiver(String label, Thread T)
    {
        _rrwLock.writeLock().lock();
        try {
            if(_evtReceivers.get(label)==null) _evtReceivers.put(new String(label), new ArrayDeque<Thread>());//tipo di evento non ancora stato registrato
            ArrayDeque<Thread> listeners=_evtReceivers.get(label);//se non ci fosse stata ora c'e`
            for(Iterator itEvtReceiverList = listeners.iterator(); itEvtReceiverList.hasNext();)
                if(itEvtReceiverList.next()==T) return ;//someone is trying to register twice to the same event...
                                                        //qualcuno sta facendo per errore una doppia registrazione su uno stesso evento
            //if we're here then the receiver is not registered.
            //se sono qui, il ricevitore non e' ancora registrato. Lo registro e rilascio il semaforo.
            _evtReceivers.get(label).add(T);//registro un nuovo gestore per questo tipo di evento
        } finally {
            _rrwLock.writeLock().unlock();
        }
    }

    static void unmapReceiver(Thread T)
    {
        _rrwLock.writeLock().lock();
        try {
            for(Iterator<Map.Entry<String, ArrayDeque<Thread>>> it = _evtReceivers.entrySet().iterator(); it.hasNext();) {
                Map.Entry<String, ArrayDeque<Thread>> entry = it.next();
                entry.getValue().remove(T);//se c'e` lo rimuove se no non fa nulla
                if(entry.getValue().isEmpty()) it.remove();//se la lista dei listeners e` vuota la rimuovo
            }
        } finally {
            _rrwLock.writeLock().unlock();
        }
    }
}
