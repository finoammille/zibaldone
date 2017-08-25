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

class TcpTestServer extends Z.Thread {
    int port;
    TcpServer srv;
    TcpConnHandler conn;
    boolean exit;

    public TcpTestServer(int port)
    {
        this.port=port;
        exit=false;
        conn=null;
        srv = new TcpServer(port);
    }
    public void startListening()
    {
        conn=srv.Accept();
        LOG.ziblog(LOG.LogLev.INF, "connection active");//se sono qui, la connessione e` stata effettuata dato che Accept() e` bloccante...
        register2Label(conn.getRxDataLabel());
        conn.Start();
        Start();
    }
    public void run()
    {
        System.out.print("\nSocketTest>>\ntype \"exit\" to finish.\n");
        while(!exit) {
            Event ev=pullOut(1000);
            if(ev!=null) {
                //if here, there was an event!
                LOG.ziblog(LOG.LogLev.INF, "received event with label %s", ev.label());
                if(ev instanceof StopThreadEvent) exit=true;
                else if(ev instanceof RawByteBufferData) {
                    RawByteBufferData rx = (RawByteBufferData)ev;
                    System.out.print("received:\n");
                    ByteBuffer bb=rx.buf();
                    if(bb.hasArray()) {
                        try {
                            byte[] rawData=bb.array();
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
                    System.out.print("\nfrom socket on port "+port+"\n");
                }
            }
            BufferedReader stdin = new BufferedReader(new InputStreamReader(System.in));
            try {
                String cmd = stdin.readLine();
                if(cmd.equals("exit")) {
                    conn.Stop();
                    exit=true;
                } else if (!cmd.isEmpty()) {
                    byte[] b = cmd.getBytes();
                    ByteBuffer buf= ByteBuffer.allocate(b.length);
                    buf.put(b);
                    (new RawByteBufferData(conn.getTxDataLabel(), buf)).emitEvent();
                }
            } catch(IOException ex){
                LOG.ziblog(LOG.LogLev.ERR, "IO Exception (%s)", ex.getMessage());
            }
        }
    }
}

class TcpTestClient extends Z.Thread {
    String destIpAddr;
    int port;
    TcpClient cli;
    TcpConnHandler conn;
    boolean exit;
    public TcpTestClient(String destIpAddr, int port)
    {
        this.destIpAddr=destIpAddr;
        this.port=port;
        exit=false;
        conn=null;
        cli = new TcpClient(destIpAddr, port);
    }
    public boolean connect()
    {
        conn=cli.Connect();
        if(conn==null) {
            LOG.ziblog(LOG.LogLev.WRN, "connection failed: is server active?");
            return false;
        }
        LOG.ziblog(LOG.LogLev.INF, "connection active");
        register2Label(conn.getRxDataLabel());
        conn.Start();
        Start();
        return true;
    }
    public void run()
    {
        System.out.print("\nSocketTest>>\ntype \"exit\" to finish.\n");
        while(!exit) {
            Event ev=pullOut(1000);
            if(ev!=null) {
                //if here, there was an event!
                LOG.ziblog(LOG.LogLev.INF, "received event with label %s", ev.label());
                if(ev instanceof StopThreadEvent) exit=true;
                else if(ev instanceof RawByteBufferData) {
                    RawByteBufferData rx = (RawByteBufferData)ev;
                    System.out.print("received:\n");
                    ByteBuffer bb=rx.buf();
                    if(bb.hasArray()) {
                        try {
                            byte[] rawData=bb.array();
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
                    System.out.print("\nfrom socket on port "+port+"\n");
                }
            }
            BufferedReader stdin = new BufferedReader(new InputStreamReader(System.in));
            try {
                String cmd = stdin.readLine();
                if(cmd.equals("exit")) {
                    conn.Stop();
                    exit=true;
                } else if (!cmd.isEmpty()) {
                    byte[] b = cmd.getBytes();
                    ByteBuffer buf= ByteBuffer.allocate(b.length);
                    buf.put(b);
                    (new RawByteBufferData(conn.getTxDataLabel(), buf)).emitEvent();
                }
            } catch(IOException ex){
                LOG.ziblog(LOG.LogLev.ERR, "IO Exception (%s)", ex.getMessage());
            }
        }
    }
}

class testTcp {
    void help()
    {
        System.out.print("\n1 => start listening server\n");
        System.out.print("2 => create client and connect to listening server\n");
        System.out.print("q => quit\n");
    }

    void test()
    {
        help();
        while(true) {
            BufferedReader stdin = new BufferedReader(new InputStreamReader(System.in));
            try {
                String cmd = stdin.readLine();
                if(cmd.equals("1")) {//server
                    int port;
                    System.out.print("insert port number ");
                    String Port=stdin.readLine();
                    try {
                        port = Integer.parseInt(Port);
                    } catch (NumberFormatException e) {
                        System.out.print("invalid port number\n");
                        LOG.ziblog(LOG.LogLev.ERR, "NumberFormatException (%s)", e.getMessage());
                        return;
                    }
                    TcpTestServer server = new TcpTestServer(port);
                    System.out.print("\n...wait for incoming connection");
                    server.startListening();
                    System.out.print("\n...connection done!");
                    server.Join();
                    return;//se sono qui, il server e` uscito.
                } else if(cmd.equals("2")) {//client
                    System.out.print("insert destination IP address (server) ");
                    String destIpAddr=stdin.readLine();
                    int port;
                    System.out.print("insert port number ");
                    String Port=stdin.readLine();
                    try {
                        port = Integer.parseInt(Port);
                    } catch (NumberFormatException e) {
                        System.out.print("invalid port number\n");
                        LOG.ziblog(LOG.LogLev.ERR, "NumberFormatException (%s)", e.getMessage());
                        return;
                    }
                    TcpTestClient client = new TcpTestClient(destIpAddr, port);
                    System.out.print("\n...trying to connect");
                    if (client.connect()==true){
                        System.out.print("\n...connection done!");
                        client.Join();
                    } else System.out.print("\n... connection FAILED.");
                    return;//se sono qui, il client e` uscito
                } else if(cmd.equals("q")) return;
                else {
                    System.out.print("\nunknown choice\n");
                    help();
                }
            } catch(IOException ex){
                LOG.ziblog(LOG.LogLev.ERR, "IO Exception (%s)", ex.getMessage());
            }
        }
    }
}
