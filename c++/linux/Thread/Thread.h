/*
 *
 * zibaldone - a C++ library for Thread, Timers and other Stuff
 * http://sourceforge.net/projects/zibaldone/
 *
 * Copyright (C) 2012  ilant (ilant@users.sourceforge.net)
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
    Event(const Event &);
    Event & operator = (const Event &);
    void emitEvent();//all thread who registered to this event will receive their own copy
};
//-------------------------------------------------------------------------------------------
//Thread
class Thread {
    friend class Event;
private:
    pthread_t _th;
    pthread_mutex_t _lockQueue;
    pthread_mutex_t _lockSet;
    bool _set;
    pthread_cond_t _thereIsAnEvent;
    virtual void run()=0;
    static void *ThreadLoop(void * arg);
    std::map < std::string, std::deque < Event* > > _eventQueue;
    void pushIn(Event*);
    std::deque <std::string > _chronologicalLabelSequence;
protected:
   /* pullOut methods are protected because they must be called only inside the thread loop (run() method).
    * A call to pullOut sets the thread in sleeping mode until he receives an event, that is until someone
    * emits an Event the thread has registered to. Note that this sleeping mode is not a busy waiting so the
    * O.S. can use the processor and run other task/thread.
    */
    Event* pullOut(int maxWaitMsec = WAIT4EVER);/* This variant returns the first available event in the
                                                 * thread queue waiting at least for "maxWaitMsec" msecs.
                                                 * If no event is in the queue within that time the method
                                                 * returns a NULL pointer. The default value "WAIT4EVER"
                                                 * makes the thread wait indefinitely until there is an
                                                 * event in his queue. In the opposite, to peek the queue to
                                                 * see if is there an event without waiting it's possible call
                                                 * pullOut with maxWaitMsec set to zero.
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
                                                                        */
    Event* pullOut(std::deque <std::string> labels, int maxWaitMsec = NOWAIT);/* This variant is similar to the previous
                                                                               * one but instead of waiting for a specified
                                                                               * label returns the first available event in
                                                                               * the labels list (with the same conditions
                                                                               * as the pullOut version with only one label,
                                                                               * i.e. maxWaitMsec cannot be set to indefinite
                                                                               * wait and "StopThreadEvent" has the precedence
                                                                               * on all other events)
                                                                               */
   /* IMPORTANT: the Stop() method sends the StopThreadEvent and blocks until the target thread exits. This is necessary to
    * ensure that the thread encapsulating object can be destroyed. For this reason the StopThreadEvent must be always
    * returned by pullOut if present, even if not in the desired event list
    */
    void StopReceivingAndDiscardReceived();/* this method stops rx of any event, deregisters from all events and thrashes
                                            * all events in the event queue
                                            */
    bool alive();//tests if thread is alive
    Thread();
public:
    virtual ~Thread();
    void Start();
    void Stop();
    void Join();
    void register2Label(const std::string& label);
private:
    /*
     * the well known big 3 rule of C++ says:
     * "if your class needs to define the destructor or the copy constructor or the copy assigment operator then it should
     * probably explicitly define all three" In our case since a Thread cannot be assigned or copied, we declare the copy
     * constructor and the assignment operator as private. This way we can prevent and easily detect any inadverted abuse
     */
    Thread(const Thread &);
    Thread & operator = (const Thread &);
};
//-------------------------------------------------------------------------------------------
//AutonomousThread
/*
    AutonomousThread is a thread characterized by

    a) it does no receive any event. Therefore the methods register2Label, pullOut are not available

    b) it's stopped by call to Stop()

    c) Does not allow Join() to prevent deadlock risk.
       To understand the reason, consider the following scenario:
       let A be an object Thread instance of type AutonomousThread. Let B be a "normal" Thread object. Suppose there are
       no more threads. If B joins A then B waits A to end, but A will never end since nobody can stop him! (the only one
       who could was B but joined....).

    d) the pure virtual method singleLoopCycle is the function that implements the SINGLE LOOP CYCLE of AutonomousThread.
       This function is continuously repeated until the Stop() method is called.

    REM: singleLoopCycle shall NOT be an infinite loop
*/
class AutonomousThread : protected Thread {
    pthread_mutex_t _lock;
    bool exit;
    void run();
    virtual void singleLoopCycle()=0;
public:
    AutonomousThread();
    void Start();
    void Stop();
};
//-------------------------------------------------------------------------------------------
//SameThreadAddressSpaceEventReceiver
/*

    IMPORTANT!
    An oject instance of SameThreadAddressSpaceEventReceiver MUST BE USED ONLY BY ONE SINGLE external thread, otherwise the
    behaviour will be unpredictable.

    explanation:
    SameThreadAddressSpaceEventReceiver provides the ability to receive Events in the SAME ADDRESS SPACE where is located.
    An object of type SameThreadAddressSpaceEventReceiver has one event queue and the methods to get event from that queue.
    If more threads use the same object instance, they will use the same event queue too. If an event becomes available, the
    first thread who arrives it extracts that event, leaving the others empty-handed! Moreover, there is no way to predict
    who will be that thread! Obviously, there is no problem in using more than one SameThreadAddressSpaceEventReceiver in the
    same thread and/or in different threads, the only limitation beeing that said above that is each object must be used by
    one thread (who can use more objects, and more threads can use their own instances).
*/
class SameThreadAddressSpaceEventReceiver : private Thread {
    void run(){}
protected:
    Event* pullOut(int maxWaitMsec);/* returns the first available event in the thread queue waiting at least for "maxWaitMsec"
                                     * msecs. If no event is in the queue within that time the method returns a NULL pointer.
                                     * Note: to peek the queue to see if is there an event without waiting it's possible call
                                     * pullOut with maxWaitMsec set to zero.
                                     */
    Event* pullOut(const std::string& label, int maxWaitMsec = NOWAIT);// waits for the event identified by label
    Event* pullOut(std::deque <std::string> labels, int maxWaitMsec = NOWAIT);/* wait for the first available event between
                                                                               * those identified by labels
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
