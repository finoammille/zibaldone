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
import java.nio.*;
import java.util.*;
import java.io.*;

class StopAndWaitOverUdpTest extends Z.Thread {
    static void help()
    {
        System.out.print("\nsend <ip> <port> <data>\t=>\tsend datagram with payload=data to ip,port\n");
        System.out.print("\t\t\t\texample: send 172.19.32.200 8989 hello\n");
        System.out.print("help\t\t\t=>\tthis screen\n");
        System.out.print("exit\t\t\t=>\tquit\n");
    }
    int port;
    Z.StopAndWaitOverUdp srv;
    boolean exit;
    public StopAndWaitOverUdpTest(int port)
    {
        this.port=port;
        exit=false;
        srv=new StopAndWaitOverUdp(port);
    }
    void GO()
    {
        register2Label(srv.getRxDataLabel());
        register2Label(SawUdpPktError.sawUdpPktErrorLabel());
        srv.Start();
        Start();//avvio di questo thread
    }
    public void run()
    {
        help();
        System.out.print("\nStopAndWaitOverUdpTest>> ");
        while(!exit){
            Event Ev = pullOut(1000);//max 1 secondo di attesa
            if(Ev!=null){
                LOG.ziblog(LOG.LogLev.INF, "received event with label %s", Ev.label());
                if(Ev instanceof StopThreadEvent) exit=true;
                else if(Ev instanceof UdpPkt) {
                    UdpPkt pkt=(UdpPkt)Ev;
                    ByteBuffer rx=pkt.buf();
                    System.out.print("received:\n");
                    rx.rewind();
                    while(rx.hasRemaining()) System.out.printf("%c", (char)rx.get());
                    System.out.printf("\nfrom %s:%d", pkt.ipAddr(), pkt.udpPort());
                } else if(Ev instanceof SawUdpPktError) {
                    SawUdpPktError err = (SawUdpPktError)Ev;
                    System.out.print("received TX ERROR event\n");
                    System.out.printf("\nfor tx to %s:%d", err.ipAddr(), err.udpPort());
                }
            }
            BufferedReader stdin = new BufferedReader(new InputStreamReader(System.in));
            try {
                String cmd = stdin.readLine();
                String[] cmds = cmd.split("\\s+");
                ArrayList<String> cmdParams = new ArrayList<String>();
                for(int i=0; i<cmds.length; i++) if(!cmds[i].isEmpty()) cmdParams.add(cmds[i]);
                if(!cmdParams.isEmpty()) {
                    if(cmdParams.get(0).equals("exit")) {
                        srv.Stop();
                        exit=true;
                    } else if (cmdParams.get(0).equals("help")) {
                        help();
                    } else if(cmdParams.get(0).equals("send")) {
                        if(cmdParams.size()<4) {
                            System.out.print("\nnot enough parameters\n");
                            help();
                            continue;
                        }
                        String destIpAddr = cmdParams.get(1);
                        int destUdpPort = Integer.parseInt(cmdParams.get(2));
                        String dataToSend = cmd.substring(cmd.indexOf(cmdParams.get(2))+(cmdParams.get(2)).length()).trim();
                        byte[] b = dataToSend.getBytes();
                        ByteBuffer data= ByteBuffer.allocate(b.length);
                        data.put(b);
                        (new UdpPkt(srv.getTxDataLabel(), destUdpPort, destIpAddr, data)).emitEvent();                
                    } else {
                        help();
                        System.out.print("\nunknown command\n");
                    }
                }
                System.out.print("\nStopAndWaitOverUdpTest>> ");
            } catch(IOException ex){
                LOG.ziblog(LOG.LogLev.ERR, "IO Exception (%s)", ex.getMessage());
            }
        }
    }
}

class testStopAndWaitOverUdp {
    void test()
    {
        System.out.print("\ninsert port number ");
        Scanner in = new Scanner(System.in);
        int port = in.nextInt();
        StopAndWaitOverUdpTest sawout = new StopAndWaitOverUdpTest(port);
        sawout.GO();
        sawout.Join();
    }
}

