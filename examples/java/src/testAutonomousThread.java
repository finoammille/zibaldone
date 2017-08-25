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
import java.util.concurrent.TimeUnit;
import java.io.*;

class backgroundWorker extends AutonomousThread {
    public void singleLoopCycle()
    {
        System.out.print("HELLO!!! I'm an autonomous thread!!!\n");
        System.out.print("I'm emitting an InfoMsg event with label AutEv!...\n");
        InfoMsg tx = new InfoMsg("AutEv", "Hello Boss!");
        tx.emitEvent();
        try {
            TimeUnit.SECONDS.sleep(1);
        } catch(InterruptedException ex){
            LOG.ziblog(LOG.LogLev.ERR, "InterruptedException (%s)", ex.getMessage());
        }
    }
}

class Boss extends Z.Thread {
    public Boss() {register2Label("AutEv");}
    void help()
    {
        System.out.print("\n*** Autonomous Thread Test ***\n");
        System.out.print("type \"go\" to start autonomous thread\n");
        System.out.print("type \"stop\" to stop autonomous thread\n");
        System.out.print("type \"exit\" to finish test and quit\n");
        System.out.print("type \"help\" to view this!\n");
    }
    public void run()
    {
        help();
        boolean exit=false;
        backgroundWorker AT = new backgroundWorker();//autonomous Thread
        BufferedReader stdin = new BufferedReader(new InputStreamReader(System.in));
        String cmd;
        int count=0;
        while (!exit) {
            Event Ev = pullOut(10);//check thread queue for new events. We don't block on queue but we wait at least 10 msec, then we poll for user input
            if(Ev!=null) {
                LOG.ziblog(LOG.LogLev.INF, "received event with label %s", Ev.label());
                if(Ev instanceof StopThreadEvent) exit=true;
                else if(Ev instanceof InfoMsg) {
                    InfoMsg info = (InfoMsg)Ev;
                    System.out.print("\n\n\nRECEIVED EVENT "+info.getClass().getSimpleName()+"\n");
                    System.out.print("payload event msg ="+info.msg()+"\n");  
                } else LOG.ziblog(LOG.LogLev.ERR, "unexpected event!");
            }
            try {
                cmd = stdin.readLine();
                count ++;
                if(!cmd.isEmpty()) {
                    LOG.ziblog(LOG.LogLev.INF, "cmd = %s, prev empty cmd num = %d", cmd, count-1);//a solo scopo di debug metto il numero di comandi vuoti che corrispondono
                                                                                                  //ai pullOut null che determinano la "reattività" ai comandi (numero di cicli)
                    count=0;
                    if(cmd.equals("go")) AT.Start();
                    else if(cmd.equals("stop")) AT.Stop();
                    else if(cmd.equals("exit")) {
                        AT.Stop();//a prescindere...!
                        exit=true;
                    } else if(cmd.equals("help")) help();
                    else System.out.println("invalid command!");
                    if(!exit) System.out.print("\n>> ");
                }
            } catch (IOException ex) {
                LOG.ziblog(LOG.LogLev.ERR, "IO Exception (%s)", ex.getMessage());
            }
        }
    }
}

class testAutonomousThread {
    void test()
    {
        Boss B=new Boss();
        B.Start();
        B.Join();
    }
}
