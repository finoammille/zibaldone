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
import java.text.*;

class UdpTest extends Z.Thread {
    static void help()
    {
        System.out.print("\nsend <ip> <port> <data>\t=>\tsend datagram with payload=data to ip,port\n");
        System.out.print("\t\t\t\texample: send 172.19.32.200 8989 hello\n");
        System.out.print("help\t\t\t=>\tthis screen\n");
        System.out.print("exit\t\t\t=>\tquit\n");
    }
    boolean exit;
    Z.Udp srv;
    void GO()
    {
        register2Label(srv.getRxDataLabel());
        srv.Start();
        Start();
    }

    public UdpTest(int port)
    {
        srv=new Z.Udp(port);
        exit=false;
    }
    public void run()
    {
        help();
        while(!exit) {
            System.out.print("\nUdpTest>> ");
            Event ev=pullOut(1000);
            if(ev!=null) {
                //if here, there was an event!
                LOG.ziblog(LOG.LogLev.INF, "received event with label %s", ev.label());
                if(ev instanceof StopThreadEvent) exit=true;
                else if(ev instanceof UdpPkt) {
                    UdpPkt pkt=(UdpPkt)ev;
                    System.out.print("received:\n");
                    ByteBuffer rx=pkt.buf();
                    if(rx.hasArray()) {
                        try {
                            byte[] rawData=rx.array();
                            for(int i=0; i<rawData.length; i++) {
                                System.out.printf("0x%02X ", rawData[i]);
                                if((i+1)%10==0) System.out.println();
                            }
                            System.out.println();
                            System.out.print("in ascii format: ");
                            String stringData = new String(rawData, "UTF-8");
                            System.out.println(stringData);
                        } catch (ReadOnlyBufferException e) {
                            LOG.ziblog(LOG.LogLev.ERR, "ReadOnlyBufferException (%s)", e.getMessage());
                        } catch (UnsupportedEncodingException e) {
                            LOG.ziblog(LOG.LogLev.ERR, "UnsupportedEncodingException (%s)", e.getMessage());
                        }
                    }
                    System.out.print("\nfrom "+pkt.ipAddr()+":"+pkt.udpPort()+"\n");
                }
            }
            BufferedReader stdin = new BufferedReader(new InputStreamReader(System.in));
            try {
                String cmd = stdin.readLine();
                if(cmd.equals("exit")) {
                    srv.Stop();
                    exit=true;
                } else {
                    ArrayList<String> cmdParams = new ArrayList<String>();
                    BreakIterator bi=BreakIterator.getWordInstance();
                    bi.setText(cmd);
                    int start=bi.first();
                    for (int end = bi.next();
                        end != BreakIterator.DONE;
                        start = end, end = bi.next()) {
                        String param=cmd.substring(start,end);
                        if(Character.isLetterOrDigit(param.charAt(0))) cmdParams.add(param);
                        if(cmdParams.size()==3 && cmd.length()>end+1) {
                            String payload=cmd.substring(end+1);
                            cmdParams.add(payload);
                            break;
                        }
                    }
                    if(cmdParams.size()<4) {
                        help();
                        if(!cmdParams.isEmpty()) System.out.print("\ninvalid command\n");
                        continue;
                    } else {
                        if(cmdParams.get(0).equals("send")) {
                            String destIpAddr = cmdParams.get(1);
                            int destUdpPort;
                            try {
                                destUdpPort = Integer.parseInt(cmdParams.get(2));
                            } catch (NumberFormatException e) {
                                System.out.print("\ninvalid port\n");
                                LOG.ziblog(LOG.LogLev.ERR, "NumberFormatException (%s)", e.getMessage());
                                continue;
                            }
                            byte[] b = cmdParams.get(3).getBytes();
                            ByteBuffer buf= ByteBuffer.allocate(b.length);
                            buf.put(b);
                            (new UdpPkt(srv.getTxDataLabel(), destUdpPort, destIpAddr, buf)).emitEvent();
                        } else {
                            help();
                            System.out.print("\nunknown command\n");
                            continue;
                        }
                    }
                }
            } catch(IOException ex){
                LOG.ziblog(LOG.LogLev.ERR, "IO Exception (%s)", ex.getMessage());
            }
        }
    }
}

class testUdp {
    void test()
    {
        int port;
        System.out.print("insert port number ");
        BufferedReader stdin = new BufferedReader(new InputStreamReader(System.in));
        String Port;
        try {
            Port = stdin.readLine();
        } catch(IOException ex){
            LOG.ziblog(LOG.LogLev.ERR, "IO Exception (%s)", ex.getMessage());
            return;
        }
        try {
            port = Integer.parseInt(Port);
        } catch (NumberFormatException e) {
            UdpTest.help();
            LOG.ziblog(LOG.LogLev.ERR, "NumberFormatException (%s)", e.getMessage());
            return;
        }
        UdpTest U=new UdpTest(port);
        U.GO();
        U.Join();
    }
}
