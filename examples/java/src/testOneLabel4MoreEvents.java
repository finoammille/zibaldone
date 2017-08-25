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
import java.nio.*;
import java.util.*;

class sharedLabel {
    public static final String label = "sharedLabel";
}

class anEvent extends Event {
    public ByteBuffer allocatedArea;
    int integer;
    String aString;
    ArrayList<Integer> andVectorToo;
    public anEvent(int bufferSize, String aStringValue)
    {
        super(sharedLabel.label);
        integer=bufferSize;
        aString=aStringValue;
        allocatedArea=ByteBuffer.allocate(integer);
        for(int i=0; i<integer; i++) allocatedArea.put(i, (byte)i);
        andVectorToo=new ArrayList<Integer>();
        for(int i=0; i<100; i++) andVectorToo.add(i);
    }
}

class Sender extends Z.Thread {
    public void help()
    {
        System.out.print("\n*** one label for more events Test!.... type \"exit\" to finish ***\n");
        System.out.print("N.B.: all emitted events have label = "+sharedLabel.label+"\n");
        System.out.print("Available commands:\n");
        System.out.print("exit => terminate test and exit\n");
        System.out.print("anEvent => emits anEvent Event\n");
        System.out.print("rawBuffer => emits RawByteBufferData event\n");
        System.out.print("timeout => emits TimeoutEvent event\n");
        System.out.print("error => emits ERROR event\n");
        System.out.print("sonOfaBitch => emits InfoMsg event with NO LABEL\n");
        System.out.print("help => this screen\n");
    }
    public void run()
    {
        help();
        boolean exit=false;
        String cmd;
        System.out.print("\n>>");
        while(!exit) {
            Event Ev = pullOut(100);//check thread queue for new events. We don't block on queue but we wait at least 100 msec, then we poll for user input
            if(Ev!=null) {
                LOG.ziblog(LOG.LogLev.INF, "received event with label %s", Ev.label());
                if(Ev instanceof StopThreadEvent) exit=true;
                else LOG.ziblog(LOG.LogLev.ERR, "unexpected event!");
            }
            try {
                BufferedReader stdin = new BufferedReader(new InputStreamReader(System.in));
                cmd = stdin.readLine();
                if(!cmd.isEmpty()) {
                    LOG.ziblog(LOG.LogLev.INF, "cmd = %s", cmd);
                    if(cmd.equalsIgnoreCase("exit")) exit=true;
                    else if(cmd.equalsIgnoreCase("anEvent")) {
                        anEvent aEv = new anEvent(1024, "stuff here...");
                        System.out.print("\nSender is emitting event "+aEv.getClass().getSimpleName()+" with label \""+aEv.label()+"\"\n");
                        System.out.print("allocated area: size ="+Integer.toString(aEv.integer)+"\ncontent is:\n");
                        for(int i=0; i<aEv.integer; i++) {
                            System.out.print(aEv.allocatedArea.get(i)+" ");
                            if((i%20)==0) System.out.print("\n");
                        }
                        System.out.print("integer = "+aEv.integer+"\n");
                        System.out.print("aString = "+aEv.aString+"\n");
                        System.out.print("andVectorToo:\n");
                        for(int i=0; i<100; i++) {
                            System.out.print(aEv.andVectorToo.get(i)+" ");
                            if((i%20)==0) System.out.print("\n");
                        }
                        aEv.emitEvent();
                        System.out.print("\nevent emitted!!!\n");
                    } else if(cmd.equalsIgnoreCase("rawBuffer")) {
                        ArrayList<Byte> bufferData = new ArrayList<Byte>();
                        for(byte i=0; i<10; i++) bufferData.add(new Byte(i));
                        RawByteBufferData rawBuffer = new RawByteBufferData(sharedLabel.label, bufferData);
                        System.out.print("\nSender is emitting event "+rawBuffer.getClass().getSimpleName()+" with label "+rawBuffer.label()+"\"\n");
                        rawBuffer.emitEvent();
                        System.out.print("\nevent emitted!!!\n");
                    } else if(cmd.equalsIgnoreCase("timeout")) {
                        TimeoutEvent te=new TimeoutEvent(sharedLabel.label);
                        System.out.print("\nSender is emitting event "+te.getClass().getSimpleName()+" with label "+te.label()+"\"\n");
                        te.emitEvent();
                        System.out.print("\nevent emitted!!!\n");
                    } else if(cmd.equalsIgnoreCase("error")) {
                        ZibErr e = new ZibErr(sharedLabel.label, "causa dell'errore = ....");
                        System.out.print("\nSender is emitting event "+e.getClass().getSimpleName()+" with label "+e.label()+"\"\n");
                        e.emitEvent();
                        System.out.print("\nevent emitted!!!\n");
                    } else if(cmd.equalsIgnoreCase("sonOfaBitch")) {
                        InfoMsg sob = new InfoMsg("", "this is a InfoMsg with label empty!!!! zibaldone will treat this event as a sob!");
                        System.out.print("\nSender is emitting event "+sob.getClass().getSimpleName()+" with label "+sob.label()+"\"\n");
                        sob.emitEvent();
                        System.out.print("\nevent emitted!!!\n");
                    } else if(cmd.equalsIgnoreCase("help")) help();
                    else System.out.println("unknown command!");
                    if(!exit) System.out.print("\n>> ");
                }
            } catch (IOException ex) {
                LOG.ziblog(LOG.LogLev.ERR, "IO Exception (%s)", ex.getMessage());
            }
        }
    }
}

//Receiver is a thread
class Receiver extends Z.Thread {
    public void run()
    {
        while(true) {
            Event Ev = pullOut();//Receiver blocks on queue waiting for events. We can receive only events we have registered to (sharedLabel) and stopThread
            if(Ev!=null){
                LOG.ziblog(LOG.LogLev.INF, "received event with label %s", Ev.label());
                if(Ev instanceof StopThreadEvent) return;
                else if(Ev instanceof anEvent) {
                    //this event has no meaningful Ev.buf() and Ev.len() (the first is NULL and the second is zero): it is the object itself that is what I want!
                    anEvent ev = (anEvent)Ev;
                    System.out.print("\nReceiver RECEIVED EVENT "+ev.getClass().getSimpleName()+"\n");
                    System.out.print("allocated area: size ="+ev.integer+"\ncontent is:\n");
                    for(int i=0; i<ev.integer; i++) {
                        System.out.print(ev.allocatedArea.get(i)+" ");
                        if((i%20)==0) System.out.print("\n");
                    }
                    System.out.print("integer = "+ev.integer+"\n");
                    System.out.print("aString = "+ev.aString+"\n");
                    System.out.print("andVectorToo:\n");
                    for(int i=0; i<100; i++) {
                        System.out.print(ev.andVectorToo.get(i)+" ");
                        if((i%20)==0) System.out.print("\n");
                    }
                } else if(Ev instanceof RawByteBufferData) {
                    RawByteBufferData ev = (RawByteBufferData)Ev;
                    System.out.print("\nReceiver RECEIVED EVENT "+ev.getClass().getSimpleName()+"\n");
                    System.out.print("buffer Length = "+ev.len()+"\n");
                    System.out.print("data:\n");
                    for(int i=0; i<ev.len(); i++) System.out.print(ev.buf().get(i));
                } else if(Ev instanceof TimeoutEvent) {
                    TimeoutEvent ev = (TimeoutEvent)Ev;
                    System.out.print("\nReceiver RECEIVED EVENT "+ev.getClass().getSimpleName()+"\n");
                    System.out.print("timerId="+ev.timerId()+"\n");
                } else if(Ev instanceof ZibErr) {
                    ZibErr ev = (ZibErr)Ev;
                    System.out.print("\nReceiver RECEIVED EVENT "+ev.getClass().getSimpleName()+"\n");
                    System.out.print("error message ="+ev.errorMsg()+"\n");
                } else if(Ev instanceof InfoMsg) {
                    InfoMsg ev = (InfoMsg)Ev;
                    System.out.print("\nReceiver RECEIVED EVENT "+ev.getClass().getSimpleName()+"\n");
                    System.out.print("msg ="+ev.msg()+"\n");
                } else LOG.ziblog(LOG.LogLev.ERR, "unexpected event!"); 
            }
        }
    }
    
    public Receiver()
    {
        register2Label(sharedLabel.label);//Receiver wants to receive a copy of events whose label is the string sharedLabel
        register2Label("sob");//this time Receiver also welcomes sons of bitch
    }
}

class testOneLabel4MoreEvents {
    void test()
    {
        Sender sender = new Sender();        //prepare a Sender Thread
        Receiver receiver = new Receiver();  //prepare Receiver Thread

        //let's start all of them:
        sender.Start();
        receiver.Start();

        //all ends whenever sender exits
        sender.Join();
        receiver.Stop();
    }
}
