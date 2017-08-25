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
import java.text.*;
import gnu.io.*;

class SerialPortTest extends Z.Thread {
    boolean exit;
    Z.SerialPortHandler sph;
    static void help()
    {
        System.out.print("\n***************************************************************************\n");
        System.out.print("SerialPortTest - available commands:\n");
        System.out.print(" list   =>   list serial ports\n\n");
        System.out.print(" open <port> <baudRate> <parity> <dataBits> <stopBits> <flowControl>   =>   (re)open port <port>\n");
        System.out.print("      baudRate=9600, 38400, 115200\n");
        System.out.print("      parity=none, even, odd, space, mark\n");
        System.out.print("      dataBits=5, 6, 7, 8\n");
        System.out.print("      stopBits=1, 2\n");
        System.out.print("      flowControl=none, hardware, xonxoff\n");
        System.out.print("      example: open /dev/ttyS0 115200 none 8 1 none\n");
        System.out.print("      example: open /dev/ttyS0 115200 (re)opens port /dev/ttyS0 with default parameters (N81N)\n\n");
        System.out.print(" send <data>   =>   sends <data> over opened serial port\n\n");
        System.out.print(" help   =>   this screen\n\n");
        System.out.print(" exit   =>   quit\n");
        System.out.print("***************************************************************************\n");
    }
    public void open(String portName,
                int baudRate, 
                SerialPortHandler.Parity parity, 
                int dataBits, 
                int stopBits, 
                SerialPortHandler.FlowControl flwctrl)
    {
        if(sph!=null) {
            sph.Stop();
            sph=null;
            Stop();//autostop to remove previous registration relative to last opened port
            Start();//fresh restart
        }
        sph=new Z.SerialPortHandler(portName, baudRate, parity, dataBits, stopBits, flwctrl);
        register2Label(sph.getRxDataLabel());
        sph.Start();
    }
    public void SerialPortTest()
    {
        sph=null;
        exit=false;
    }
    public void run()
    {
        help();
        while(!exit) {
            System.out.print("\nSerialPortTest>> ");
            Event ev=pullOut(1000);
            if(ev!=null) {
                //if here, there was an event!
                LOG.ziblog(LOG.LogLev.INF, "received event with label %s", ev.label());
                if(ev instanceof StopThreadEvent) LOG.ziblog(LOG.LogLev.INF, "port reopen..."); //here usually we exit, but this time 
                                                                                                //we only exit at "exit" user command.
                                                                                                //this StopThreadEvent is originated 
                                                                                                //by a Stop() called on a (re)open 
                                                                                                //(i.e. a open with sph!=nulll)
                else if(ev instanceof RawByteBufferData) {
                    RawByteBufferData rxData=(RawByteBufferData)ev;
                    System.out.print("received:\n");
                    ByteBuffer rx=rxData.buf();
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
                        if(sph!=null) sph.Stop();
                        exit=true;
                    } else if (cmdParams.get(0).equals("help")) {
                        help();
                    } else if (cmdParams.get(0).equals("list")) {
                        System.out.print("\n");
                        Enumeration pList = CommPortIdentifier.getPortIdentifiers();
                        while (pList.hasMoreElements()) {
                            CommPortIdentifier cpi = (CommPortIdentifier) pList.nextElement();
                            if(cpi.getPortType() == CommPortIdentifier.PORT_SERIAL) System.out.print(cpi.getName()+"\n");
                        }
                    } else if(cmdParams.get(0).equals("open")) {
                        if(cmdParams.size()!=3 && cmdParams.size()!=7) {
                            System.out.print("\ntoo less parameters or too many to use default ones\n");
                            help();
                            continue;
                        } 
                        String portName = cmdParams.get(1);
                        int baudRate = Integer.parseInt(cmdParams.get(2));
                        SerialPortHandler.Parity parity = SerialPortHandler.Parity.ParityNone;//default value
                        int dataBits = 8;
                        int stopBits = 1;
                        SerialPortHandler.FlowControl flwctrl = SerialPortHandler.FlowControl.FlowControlNone;//default value
                        if(cmdParams.size() == 7) {
                            //override default params
                            if(cmdParams.get(3).equals("none")) parity = SerialPortHandler.Parity.ParityNone;
                            else if(cmdParams.get(3).equals("even")) parity = SerialPortHandler.Parity.ParityEven;
                            else if(cmdParams.get(3).equals("odd")) parity = SerialPortHandler.Parity.ParityOdd;
                            else if(cmdParams.get(3).equals("space")) parity = SerialPortHandler.Parity.ParitySpace;
                            else if(cmdParams.get(3).equals("mark")) parity = SerialPortHandler.Parity.ParityMark;
                            dataBits = Integer.parseInt(cmdParams.get(4));
                            stopBits = Integer.parseInt(cmdParams.get(5));
                            if(cmdParams.get(6).equals("none")) flwctrl = SerialPortHandler.FlowControl.FlowControlNone;
                            else if(cmdParams.get(6).equals("hardware")) flwctrl = SerialPortHandler.FlowControl.FlowControlHardware;
                            else if(cmdParams.get(6).equals("xonxoff")) flwctrl = SerialPortHandler.FlowControl.FlowControlXonXoff;
                        }
                        System.out.print("\nopen port "+portName+
                                         " baudRate="+baudRate+
                                         " parity="+parity+
                                         " dataBits="+dataBits+
                                         " stopBits="+stopBits+
                                         " flwctrl="+flwctrl+"\n");
                        open(portName, baudRate, parity, dataBits, stopBits, flwctrl);
                    } else if(cmdParams.get(0).equals("send")) {
                        if(sph==null) {
                            System.out.print("\nopen a port before trying to send data over it!\n");
                            help();
                            continue;
                        }
                        String dataToSend = cmd.substring(cmd.indexOf("send")+4).trim();
                        System.out.print("\nsending "+dataToSend+"\n");
                        byte[] b = dataToSend.getBytes();
                        ByteBuffer buf= ByteBuffer.allocate(b.length);
                        buf.put(b);
                        (new RawByteBufferData(sph.getTxDataLabel(), buf)).emitEvent();
                    } else {
                        help();
                        System.out.print("\nunknown command\n");
                        continue;
                    }
                }
            } catch(IOException ex){
                LOG.ziblog(LOG.LogLev.ERR, "IO Exception (%s)", ex.getMessage());
            }
        }
    }
}

class testSerialPort {
    void test()
    {
        SerialPortTest S=new SerialPortTest();
        S.Start();
        S.Join();
    }
}
