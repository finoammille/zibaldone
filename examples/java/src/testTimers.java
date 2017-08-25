/*
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

import Z.*;
import java.io.*;
import java.util.Iterator;
import java.util.ArrayDeque;

class testTimerThread extends Z.Thread {
    public void help()
    {
        System.out.println("Timers Test!.... press Q to exit");
        System.out.println("Available commands:");
        System.out.println("n(timerId, duration_msec) => start new timer");
        System.out.println("s(timerId) => stop timer");
        System.out.println("l => list timer");
        System.out.println("h => help: print this screen...");
    }
    private ArrayDeque<Z.Timer> TimerList=new ArrayDeque<Z.Timer>();

    private void removeTimerFromList(String timerId)
    {
        Iterator<Z.Timer> itr=TimerList.iterator();
        while(itr.hasNext()) {
            Z.Timer tm=itr.next();
            if(timerId.equals(tm.getTimerId())) {
                TimerList.remove(tm);
                return;
            }
        }
    }

    public void run()
    {
        java.text.DateFormat dateFormat=new java.text.SimpleDateFormat("dd/MM/yyyy HH:mm:ss");
        help();
        boolean exit=false;
        BufferedReader stdin = new BufferedReader(new InputStreamReader(System.in));
        String cmd;
        while(!exit) {
            try {
                Event ev=pullOut(1000);
                if(ev!=null) {
                    //if here, there was an event!
                    LOG.ziblog(LOG.LogLev.INF, "received event with label %s", ev.label());
                    if(ev instanceof StopThreadEvent) exit=true;
                    else if (ev instanceof TimeoutEvent) {
                        TimeoutEvent to = (TimeoutEvent)ev;
                        String timerId=to.label();
                        System.out.println("timer "+timerId+" expired! at "+dateFormat.format(new java.util.Date()));
                        removeTimerFromList(timerId);
                    }
                }
                cmd = stdin.readLine();
                if(!cmd.isEmpty()) {
                    if(cmd.equals("q") || cmd.equals("Q")) {
                        exit=true;
                        Iterator<Z.Timer> itr=TimerList.iterator();
                        while(itr.hasNext()) {
                            Z.Timer t=itr.next();
                            t.Stop();
                            System.out.println("Stopped timer "+t.getTimerId()+" with duration "+t.getDuration()+" msec");
                        }
                    } else if(cmd.equals("h") || cmd.equals("H")) help();
                    else if (cmd.equals("l") || cmd.equals("L")) {
                        System.out.println("timer list:");
                        Iterator<Z.Timer> itr=TimerList.iterator();
                        while(itr.hasNext()) {
                            Z.Timer tm=itr.next();
                            System.out.println("timer: "+tm.getTimerId()+", duration:"+tm.getDuration());
                        }
                    } else if(cmd.charAt(0)=='n' || cmd.charAt(0)=='N' || cmd.charAt(0)=='s' || cmd.charAt(0)=='S') {
                        int i1=cmd.indexOf('(');
                        int i2=cmd.indexOf(',');
                        int i3=cmd.indexOf(')');
                        if(i1>0 && i3>0) {
                            if(cmd.charAt(0)=='s' || cmd.charAt(0)=='S') {
                                String timerId = cmd.substring(i1+1, i3);
                                Iterator<Z.Timer> itr=TimerList.iterator();
                                while(itr.hasNext()) {
                                    Z.Timer tm=itr.next();
                                    if(tm.getTimerId().equals(timerId)) {
                                        tm.Stop();
                                        removeTimerFromList(timerId);
                                        System.out.println("stopped timer "+timerId);
                                        break;
                                    }
                                }
                            } else if(cmd.charAt(0)=='n' || cmd.charAt(0)=='N') {
                                String timerId = cmd.substring(i1+1, i2);
                                int duration = Integer.parseInt((cmd.substring(i2+1, i3)).trim());
                                Z.Timer t=null;
                                boolean restart=false;
                                Iterator<Z.Timer> itr=TimerList.iterator();
                                while(itr.hasNext()) {
                                    t=itr.next();
                                    if(timerId.equals(t.getTimerId())) {
                                        t.Stop();
                                        System.out.println("Timer "+timerId+" already present has been restarted with new value...");
                                        restart=true;
                                        break;
                                    }
                                }
                                if(restart==false) {
                                    t=new Z.Timer(timerId);
                                    TimerList.add(t);
                                }
                                register2Label(t.getTimerId());
                                t.Start(duration);
                                System.out.println("started Timer "+timerId+" at "+dateFormat.format(new java.util.Date()));
                            } else System.out.println("invalid command!");
                        }
                    }
                    cmd="";
                }
            } catch(IOException ex){
                LOG.ziblog(LOG.LogLev.ERR, "IO Exception (%s)", ex.getMessage());
                exit=true;
            }
            System.out.println("now it's: "+dateFormat.format(new java.util.Date()));
        }
        LOG.ziblog(LOG.LogLev.INF, "test finished!");
    }
}

class testTimers {
    void test()
    {
        testTimerThread tTt=new testTimerThread();
        tTt.Start();
        tTt.Join();
        tTt.Stop();
    }
}
