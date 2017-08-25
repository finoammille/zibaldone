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

class keylogger extends Z.Thread {
    boolean exit;
    keylogger(){exit=false;}
    public void run()
    {
        System.out.print("Thread Test>>\ntype \"exit\" to finish.\n");
        while(!exit) {
            Event ev=pullOut(1000);
            if(ev!=null) {
                //if here, there was an event!
                LOG.ziblog(LOG.LogLev.INF, "received event with label %s", ev.label());
                if(ev instanceof StopThreadEvent) exit=true;
            }
            BufferedReader stdin = new BufferedReader(new InputStreamReader(System.in));
            try {
                String cmd = stdin.readLine();
                if(cmd.equals("exit")) exit=true;
                else {
                    if(!cmd.isEmpty()) {
                        String label="userText";//label that tags Events (emitted by keylogger) that contains text typed by user
                        InfoMsg tx=new InfoMsg(label, "=>"+(new String(cmd))+"<=");
                        tx.emitEvent();
                        cmd="";
                    }
                }
            } catch(IOException ex){
                LOG.ziblog(LOG.LogLev.ERR, "IO Exception (%s)", ex.getMessage());
            }
        }
    }
}

class parrot extends Z.Thread {
    boolean exit;
    parrot()
    {
        exit=false;
        String label="userText";//label that tags Events (emitted by keylogger) that contains text typed by user
        register2Label(label);//parrot wants to receive a copy of event whose id is the string userText
    }
    public void run()
    {
        while(!exit) {
            Event ev = pullOut();//parrot blocks on queue waiting for events. We can receive only events we have registered to (userText) and stopThreadEventId
            if(ev!=null){
                LOG.ziblog(LOG.LogLev.INF, "received event with label %s", ev.label());
                if(ev instanceof StopThreadEvent) exit=true;
                else if(ev instanceof InfoMsg) {
                    System.out.println("PARROT RECEIVED "+((InfoMsg)ev).msg());
                } else LOG.ziblog(LOG.LogLev.ERR, "unexpected event!"); 
            }
        }
    }
}

class testThreads {
    void test()
    {
        keylogger k=new keylogger();
        k.Start();
        parrot p=new parrot();
        p.Start();
        k.Join();
        p.Stop();// ATTENZIONE! SE NON STOPPO il programma non esce perchè main resta in attesa!!!!
    }
}
