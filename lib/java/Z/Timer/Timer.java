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

public class Timer {
    private final String timerId;
    private int duration;
    private java.util.Timer timer;
    private final ReentrantLock _lockDuration = new ReentrantLock();

    private class onTimeout extends java.util.TimerTask {
        @Override
        public void run()
        {
            (new TimeoutEvent(timerId)).emitEvent();
            Stop();
        }
    }

    public Timer(String TimerId)
    {
        timerId = new String(TimerId);
        duration=-1;
        timer=null;
    }

    public Timer(String TimerId, int Duration)
    {
        timerId = new String(TimerId);
        duration=Duration;
        timer=null;
    }

    public String getTimerId() {return new String(timerId);}

    public int getDuration() {return duration;}

    //nota: il synchronized mi serve per evitare corse tra Start e Stop (tra due Start non
    //possono esserci corse perche` c'e` _lockDuration.lock() all'inizio di entrambe
    public synchronized void Start()
    {
        _lockDuration.lock();
        try {
            if(duration<0) {
                //nè il costruttore ne il chiamante di Start hanno assegnato un valore utile a _duration
                LOG.ziblog(LOG.LogLev.WRN, "timer %s has non valid duration value", timerId);
                return;
            }
            timer = new java.util.Timer();
            timer.schedule(new onTimeout(), duration);
        } finally {
            _lockDuration.unlock();
        }
    }

    public synchronized void Start(int msec)
    {
        _lockDuration.lock();
        try {
            duration=msec;
        } finally {
            _lockDuration.unlock();
        }
        Start();
    }

    public synchronized void Stop()
    {
        if(timer!=null){
            timer.cancel();
            timer=null;
        }
    }
}
