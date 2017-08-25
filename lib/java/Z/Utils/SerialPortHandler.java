/*
 *
 * zibaldone - a C++/Java library for Thread, Timers and other Stuff
 *
 * http://sourceforge.net/projects/zibaldone/
 *
 * version 3.2.0, February 14th, 2016
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

import gnu.io.*;
import java.io.*;
import java.util.TooManyListenersException;
import java.nio.ByteBuffer;

/*
 * SerialPortHandler implements a thread who handles a serial port
 *
 * 1) SerialPortHandler emits an event of type RawByteBufferData with label = "rxDataEventxxx",
 *    where "xxx" uniquely identifies the serial port managed by SerialPortHandler (e.g.
 *    \dev\ttyS0), every time it receives a byte-burst data from the port. The event name (label)
 *    can be obtained by calling getRxDataLabel method. As usual, who wants to receive the event,
 *    has to register to the event.
 *
 * 2) SerialPortHandler registers to receive events of type RawByteBufferData with label =
 *    "txDataEventxxx", where "xxx" uniquely identifies the serial port managed by
 *    SerialPortHandler (e.g. \dev\ttyS0). The event name (label) can be obtained by calling
 *    getTxDataLabel method: every time someone wants to transmit something over the serial port, he
 *    has to emit a RawByteBufferData event with this label.
 *
 * 3) SerialPortHandler emits a zibErr event with label = "serialPortErrorEventxxx", where "xxx"
 *    uniquely identifies the serial port managed by SerialPortHandler (e.g. \dev\ttyS0), in case
 *    of error. The event label can be obtained by means of the method getSerialPortErrorLabel
 *
 * ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
 *
 * SerialPortHandler implementa un thread che gestisce la porta seriale.
 *
 * 1) SerialPortHandler emette un evento di tipo RawByteBufferData con label = "rxDataEventxxx"
 *    dove "xxx" identifica univocamente la porta seriale gestita da SerialPortHandler (x es.
 *    /dev/ttyS0) ogni volta che riceve un burst di byte sulla porta. Il nome dell'evento (label)
 *    e' ottenibile invocando il metodo getRxDataLabel. Chi desidera ricevere l'evento deve registrarsi.
 *
 * 2) SerialPortHandler e` registrato per ricevere eventi di tipo RawByteBufferData con label =
 *    "txDataEventxxx" dove "xxx" identifica univocamente la porta seriale gestita da SerialPortHandler
 *    (x es. Quindi ogni volta che si vuole trasmettere dei byte sulla porta basta emettere un evento di
 *    tipo RawByteBufferData avente come label la stringa identificativa ottenibile invocando il metodo
 *    getTxDataLabel.
 *
 * 3) SerialPortHandler emette un evento zibErr con label = "serialPortErrorEventxxx" dove "xxx"
 *    identifica univocamente la porta seriale gestita da SerialPortHandler (x es. /dev/ttyS0) in caso di
 *    errore Il nome dell'evento (label) e' ottenibile invocando il metodo getSerialPortErrorLabel
 */

public class SerialPortHandler extends Thread {
    private SerialPort sp;
    private Reader reader;
    private boolean exit;
    private int TIME_OUT=2000; //max wait in msec (2 sec then) waiting for open port
    public enum Parity {ParityNone, ParityEven, ParityOdd , ParitySpace, ParityMark}
    public enum FlowControl {FlowControlNone, FlowControlHardware, FlowControlXonXoff}
    public SerialPortHandler (String portName,
                              int baudRate,
                              Parity parity,
                              int dataBits,
                              int stopBits,
                              FlowControl flwctrl,
                              boolean toggleDtr,
                              boolean toggleRts)
    {
        CommPortIdentifier portId;
        try {
            portId=CommPortIdentifier.getPortIdentifier(portName);
        } catch(NoSuchPortException ex) {
            LOG.ziblog(LOG.LogLev.ERR, "SerialPortHandler constructor - NoSuchPortException(%s)", ex.getMessage());
            return;
        }
        try {
            sp=(SerialPort) portId.open(this.getClass().getName(), TIME_OUT);//using class name for the appName
        } catch(PortInUseException ex) {
            LOG.ziblog(LOG.LogLev.ERR, "SerialPortHandler constructor - PortInUseException(%s)", ex.getMessage());
            return;
        }
        int jparity;
        switch(parity) {
            case ParityNone: jparity=SerialPort.PARITY_NONE; break;
            case ParityEven: jparity=SerialPort.PARITY_EVEN; break;
            case ParityOdd: jparity=SerialPort.PARITY_ODD; break;
            case ParitySpace: jparity=SerialPort.PARITY_SPACE; break;
            case ParityMark: jparity=SerialPort.PARITY_MARK; break;
            default: LOG.ziblog(LOG.LogLev.ERR, "invalid parity %d", parity); return;
        } 
        int jdataBits;
        switch(dataBits) {
            case 5: jdataBits=SerialPort.DATABITS_5; break;
            case 6: jdataBits=SerialPort.DATABITS_6; break;
            case 7: jdataBits=SerialPort.DATABITS_7; break;
            case 8: jdataBits=SerialPort.DATABITS_8; break;
            default: LOG.ziblog(LOG.LogLev.ERR, "invalid dataBits %d", dataBits); return;
        }
        int jstopBits;
        switch(stopBits) {//non supporto stopBit=1.5 perche` non e` piu` utilizzato (vedi anche lato c++)
            case 1: jstopBits=SerialPort.STOPBITS_1; break;
            case 2: jstopBits=SerialPort.STOPBITS_2; break;
            default: LOG.ziblog(LOG.LogLev.ERR, "invalid stopBits %d", stopBits); return;
        }
        int jflwctrl;
        switch(flwctrl) {
            case FlowControlNone: jflwctrl=SerialPort.FLOWCONTROL_NONE; break;
            case FlowControlHardware: jflwctrl=SerialPort.FLOWCONTROL_RTSCTS_IN | SerialPort.FLOWCONTROL_RTSCTS_OUT; break;
            case FlowControlXonXoff: jflwctrl=SerialPort.FLOWCONTROL_XONXOFF_IN | SerialPort.FLOWCONTROL_XONXOFF_OUT; break;
            default: LOG.ziblog(LOG.LogLev.ERR, "invalid flwctrl %d", flwctrl); return;
        }
        try {
            sp.setSerialPortParams(baudRate, jdataBits, jstopBits, jparity);
            sp.setFlowControlMode(jflwctrl);
            if(toggleDtr) sp.setDTR(true);
            if(toggleRts) sp.setRTS(true);
        } catch(UnsupportedCommOperationException  ex) {
            LOG.ziblog(LOG.LogLev.ERR, "SerialPortHandler constructor Exception(%s)", ex.getMessage());
            return;
        }
        exit=false;
        reader=new Reader();
        register2Label(getTxDataLabel());
    }
    public SerialPortHandler (String portName,
                              int baudRate,
                              Parity parity,
                              int dataBits,
                              int stopBits,
                              FlowControl flwctrl)
    {
        //overload con toggleDtr e toggleRts a false (in java non sono possibili argomenti di default...)
        this(portName, baudRate, parity, dataBits, stopBits, flwctrl, false, false);
    }
    public String getTxDataLabel()
    {
        return "spTxDataEvent"+sp.getName();
    }
    public String getRxDataLabel()
    { 
        return "spRxDataEvent"+sp.getName();
    }
    public void run()
    {
        while(!exit) {
            Event ev = pullOut();
            if(ev!=null) {
                LOG.ziblog(LOG.LogLev.INF, "received event with label %s", ev.label());
                if(ev instanceof StopThreadEvent) {
                    exit=true;
                    try {sp.getOutputStream().close();}
                    catch(IOException ex){LOG.ziblog(LOG.LogLev.ERR, "stop/close error - IOException(%s)", ex.getMessage());}
                    reader.StopListening();
                } else if(ev instanceof RawByteBufferData) {
                    byte[] txBytes = new byte[((RawByteBufferData)ev).len()];
                    ByteBuffer txbb = ((RawByteBufferData)ev).buf();
                    txbb.rewind();
                    txbb.get(txBytes);//carico il contenuto di txbb in txBytes
                    try {sp.getOutputStream().write(txBytes, 0, txBytes.length);}
                    catch(IOException ex){LOG.ziblog(LOG.LogLev.ERR, "send error - IOException(%s)", ex.getMessage());}
                } else LOG.ziblog(LOG.LogLev.ERR, "unhandled event(%s)", ev.label());
            }
        }
    }
    private class Reader implements SerialPortEventListener {
        private boolean exit;
        InputStream istream;
        public Reader()
        {
            try {
                istream=sp.getInputStream();
                sp.addEventListener(this);
                sp.notifyOnDataAvailable(true);
            } catch (TooManyListenersException ex) {
                LOG.ziblog(LOG.LogLev.ERR, "TooManyListenersException(%s)", ex.getMessage());
            } catch(IOException ex){
                LOG.ziblog(LOG.LogLev.ERR, "IOException(%s)", ex.getMessage());
            }
        }
        public void StopListening()
        {
            sp.removeEventListener();
            sp.close();
            try {istream.close();}
            catch(IOException ex){LOG.ziblog(LOG.LogLev.ERR, "IOException(%s)", ex.getMessage());}
        }
        @Override
        public void serialEvent (SerialPortEvent sev)
        {
            int sevType=sev.getEventType();
            LOG.ziblog(LOG.LogLev.INF, "serialEvent type = %d", sevType);
            if(sevType == SerialPortEvent.DATA_AVAILABLE) {
                byte[] rxBytes = new byte[1024];//more than necessary ...
                try {
                    if(istream.available() > 0) {
                        int n = istream.read(rxBytes, 0, 1024);
                        if(n==-1) {
                            LOG.ziblog(LOG.LogLev.WRN, "end of stream reached");
                        } else {
                            RawByteBufferData rx = new RawByteBufferData(getRxDataLabel(), (ByteBuffer.allocate(n)).put(rxBytes, 0, n));
                            rx.emitEvent();
                        }
                    }
                } catch(IOException ex){LOG.ziblog(LOG.LogLev.ERR, "IOException(%s)", ex.getMessage());}
            }
        }
    }
}
