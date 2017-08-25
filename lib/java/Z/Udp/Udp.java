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

import java.net.*;
import java.net.SocketException;
import java.io.IOException;
import java.nio.ByteBuffer;

//------------------------------------------------------------------------------
/*
Use:

1) instantiate a Udp object specifying the listening udp port.
2) to receive a datagram you have to register on event of type UdpPkt
   whose label is returned by getRxDataLabel(). Note that UdpPkt
   contains the information about ip address and port of sender.
3) to transmit a datagram you have to emit an Event of type UdpPkt with
   label set to the label returned by the getTxDataLabel() method. Note
   that u have to specify in the UdpPkt event the ip address and udp port
   that uniquely identifies the destination.

REM: UDP is connectionless so it's up to you handle any transmission error
     (e.g. by mean of a stop and wait protocol)

::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

Utilizzo:

1) istanziare Udp specificando la porta Udp su cui il thread e` in ascolto per
   ricevere pacchetti Udp. L'indirizzo ip non viene specificato perche` e` sot-
   tointeso che sia localhost (non posso ricevere pacchetti destinati ad un ip
   diverso!)

   N.B.: ricordarsi di far partire il thread con Start come al solito!

2) per ricevere dati occorre registrarsi sull'evento di tipo UdpPkt la cui label
   viene restituita dal metodo getRxDataLabel().

3) per trasmettere dati occorre emettere un Evento di tipo UdpPkt con label uguale
   a quella restituita dal metodo getTxDataLabel(). In questo modo sostanzialmente
   si dice a Upd (il thread locale) di trasmettere un datagram verso un determinato
   destinatario individuato univocamente da (ip/udpPort)

NOTA: poiche` udp e` connectionless, occorre gestire autonomamente le eventuali
      ritrasmissioni (per esempio utilizzando uno stop and wait)
*/
//------------------------------------------------------------------------------

public class Udp extends Thread {
    private DatagramSocket sock;
    private boolean exit;
    public Udp(int udpPort)
    {
        exit=false;
        try { sock = new DatagramSocket(udpPort);}
        catch (SocketException se) {LOG.ziblog(LOG.LogLev.ERR, "SocketException(%s)", se.getMessage());}
        reader=new Reader();
        register2Label(getTxDataLabel());
    }
    public void Start()
    {
        super.Start();
        reader.Start();
    }
    public void Stop()
    {
        reader.Stop();
        super.Stop();
    }
    public void Join()
    {
        reader.Join();
        super.Stop();
    }
    public String getTxDataLabel()
    {
        return "dgramTxDataEvent"+sock.getLocalAddress().getHostAddress()+(new Integer(sock.getLocalPort()).toString());
    }
    public String getRxDataLabel()
    { 
        return "dgramRxDataEvent"+sock.getLocalAddress().getHostAddress()+(new Integer(sock.getLocalPort()).toString());
    }
    public void run()
    {
        while(!exit) {
            Event ev = pullOut();
            if(ev!=null) {
                LOG.ziblog(LOG.LogLev.INF, "received event with label %s", ev.label());
                if(ev instanceof StopThreadEvent) exit=true;
                else if(ev instanceof UdpPkt) {
                    byte[] tx = new byte[((UdpPkt)ev).len()];
                    ByteBuffer txbb=((UdpPkt)ev).buf();
                    txbb.rewind();
                    txbb.get(tx);
                    InetAddress srcIpAddr;
                    try {
                        srcIpAddr = InetAddress.getByName(((UdpPkt)ev).ipAddr());
                    } catch(UnknownHostException ex) {
                        LOG.ziblog(LOG.LogLev.ERR, "cannot get srcIpAddr - UnknownHostException(%s)", ex.getMessage());
                        continue;
                    }
                    DatagramPacket pkt = new DatagramPacket(tx, tx.length, srcIpAddr, ((UdpPkt)ev).udpPort());
                    try{sock.send(pkt);}
                    catch(IOException ex){LOG.ziblog(LOG.LogLev.ERR, "send error - IOException(%s)", ex.getMessage());}
                } else LOG.ziblog(LOG.LogLev.ERR, "unhandled event (%s)", ev.label());
            }
        }
    }
    class Reader extends Thread {
        private boolean exit;
        public void Stop()
        {
            sock.close();
            exit=true;
        }
        public void Start()
        {
            exit=false;
            super.Start();
        }
        public void run()
        {
            byte[] rxbyte = new byte[1024];//we choose to set 1 kb max each datagram ... if not enough u can modify this....
            DatagramPacket rxPkt = new DatagramPacket(rxbyte, rxbyte.length);
            String rxDataLabel=getRxDataLabel();
            while(!exit) {
                try {
                    sock.receive(rxPkt);//blocks here until something comes!
                    UdpPkt rx = new UdpPkt(rxDataLabel, 
                                           rxPkt.getPort(), 
                                           rxPkt.getAddress().getHostAddress(), 
                                           (ByteBuffer.allocate(rxPkt.getLength())).put(rxPkt.getData(), 0, rxPkt.getLength()));
                    rx.emitEvent();
                } catch(IOException ex){
                    if(!exit) LOG.ziblog(LOG.LogLev.ERR, "receive error - IOException(%s)", ex.getMessage());
                    else LOG.ziblog(LOG.LogLev.INF, "socked closed because of Stop()");
                }
            }
        }
    } 
    Reader reader;
}
