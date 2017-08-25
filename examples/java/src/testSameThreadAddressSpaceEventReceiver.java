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

class reqEv extends Event {
    private String msg;
    public reqEv(String info)
    {
        super("reqEv");
        msg=new String(info);
    }
    public String getMsg() {return new String(msg);}
}

class respEv extends Event {
    private String msg;
    public respEv(String info)
    {
        super("respEv");
        msg=new String(info);
    }
    public String getMsg() {return new String(msg);}
}

class worker extends Z.Thread {
    boolean exit;
    public void run()
    {
        while(!exit) {
            Event Ev = pullOut();
            if(Ev!=null){
                LOG.ziblog(LOG.LogLev.INF, "received event with label %s", Ev.label());
                if(Ev instanceof StopThreadEvent) exit=true;
                else if(Ev instanceof reqEv) {
                    reqEv ev = (reqEv)Ev;
                    System.out.print("...WORKER received info message:"+ev.getMsg()+"\n");
                    String resp="*** ok! *** received the request "+ev.getMsg();
                    (new respEv(resp)).emitEvent();
                } else LOG.ziblog(LOG.LogLev.ERR, "### unexpected event! ###");
            }
        }
    }
    public worker()
    {
        exit=false;
        register2Label("reqEv");
    }
}

class delegator extends SameThreadAddressSpaceEventReceiver {
    public delegator() {register2Label("respEv");}
    public void request(int msecWait)
    {
        System.out.print("...purge rx queue....\n");
        Event discarded;
        int count=0;
        while ((discarded=pullOut(0))!=null) {//instant peek and discard
            System.out.print("### discarded event "+discarded.label()+" ###\n");
            discarded=null;
            count++;
        }
        if(count==0) System.out.print("*** nothing discarded!!! ***\n");
        System.out.print("... processing request("+Integer.toString(msecWait)+")\n");
        String infoMsg="request"+Integer.toString(msecWait);
        (new reqEv(infoMsg)).emitEvent();
        Event Ev=pullOut(msecWait);
        if(Ev!=null){
            LOG.ziblog(LOG.LogLev.INF, "received event with label %s", Ev.label());
            if(Ev instanceof respEv) {
                respEv ev = (respEv)Ev;
                System.out.print("*** received RESPONSE message: *** "+ev.getMsg()+" *** for request("+Integer.toString(msecWait)+")\n");
            } else LOG.ziblog(LOG.LogLev.ERR, "unexpected event!");
            
        } else System.out.print("### timeout elapsed!! for request("+Integer.toString(msecWait)+") ###\n");
    }
    public void requestWithoutResponse()
    {
        System.out.print("...purge rx queue....\n");
        Event discarded;
        int count=0;
        while ((discarded=pullOut(0))!=null) {//instant peek and discard
            System.out.print("### discarded event "+discarded.label()+" ###\n");
            discarded=null;
            count++;
        }
        if(count==0) System.out.print("*** nothing discarded!!! ***\n");
        System.out.print("... processing requestWithoutResponse\n");
        (new InfoMsg("ciao", "ciao")).emitEvent();
        Event Ev=pullOut(1000);
        if(Ev!=null){
            LOG.ziblog(LOG.LogLev.INF, "received event with label %s", Ev.label());
            if(Ev instanceof respEv) {
                respEv ev = (respEv)Ev;
                System.out.print("*** received RESPONSE message: *** "+ev.getMsg()+" *** and label "+Ev.label()+"\n");
            } else LOG.ziblog(LOG.LogLev.ERR, "unexpected event!");
        } else System.out.print("### timeout elapsed!! for requestWithoutResponse ###\n");
    }
}

class testSameThreadAddressSpaceEventReceiver {
    void help()
    {
        System.out.print("\nr(msec) => call d.request(msec);\n");
        System.out.print("reqWithoutResp => call d.requestWithoutResponse();\n");
        System.out.print("help => this screen\n");
        System.out.print("e => exit\n\n");
    }
    void test()
    {
        help();
        boolean exit=false;
        worker w = new worker();
        w.Start();
        delegator d = new delegator();
        String cmd;
        BufferedReader stdin = new BufferedReader(new InputStreamReader(System.in));
        System.out.print("\n>> ");
        while(!exit) {
            try {
                cmd = stdin.readLine();
                if(!cmd.isEmpty()) {
                    if(cmd.equals("e")) {
                        w.Stop();
                        exit=true;
                    } else if(cmd.equals("reqWithoutResp")) d.requestWithoutResponse();
                    else if(cmd.equals("help")) help();
                    else if(cmd.charAt(0)=='r') {
                        int i1=cmd.indexOf('(');
                        int i2=cmd.indexOf(')');
                        if(i1>0 && i2>0) {
                            int msec = Integer.parseInt(cmd.substring(i1+1, i2).trim());
                            d.request(msec);
                        } else {
                            System.out.print("\nsyntax error in command "+cmd+"\n");
                            help();
                        }
                    } else System.out.print("\nunknown command "+cmd+"\n");
                    if(!exit) System.out.print("\n>> ");
                }
            } catch (IOException ex) {
                LOG.ziblog(LOG.LogLev.ERR, "IO Exception (%s)", ex.getMessage());
            } catch (NumberFormatException ex) {
                System.out.print("\ninvalid number\n");
                System.out.print("\n>> ");
                LOG.ziblog(LOG.LogLev.ERR, "NumberFormatException (%s)", ex.getMessage());
            }
        }
    }
}

