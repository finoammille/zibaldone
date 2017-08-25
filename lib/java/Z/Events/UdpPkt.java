/*
 *
 * zibaldone - a C++/Java library for Thread, Timers and other Stuff
 *
 * http://sourceforge.net/projects/zibaldone/
 *
 * version 3.1.2, August 29th, 2015
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

import java.nio.ByteBuffer;
import java.util.*;

//---------------------------------------------------------------------------------------------------------------------
/*
    UdpPkt Event

    event for tx/rx of an udp datagram

    it's basically a RawByteBufferData extended with a string for ip address and a int for udp port. The ip address is 
    meant to be the sender ip address if the event label is = to getRxDataLabel() or the destination ip address if 
    label = getTxDataLabel(). In other words UdpPkt is a RawByteBufferData with the addition of the "udp coordinates"

    :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

    evento UdpPkt

    evento per la tx/rx di un pacchetto udp.

    sostanzialmente e` un rawbytebufferdata esteso con l'aggiunta di una string che contiene l'indirizzo ip e la porta 
    udp del destinatario o del mittente a seconda che la label dell'evento sia gettxdatalabel oppure getrxdatalabel. 
    In altre parole e` un rawbytebufferdata con in piu` le "coordinate udp".

    nota: il tcp usa semplicemente rawbytebufferdata perche` una volta stabilita la connessione i dati viaggano su un 
          collegamento connesso punto-punto, invece udp e` connectionless.
*/
public class UdpPkt extends RawByteBufferData {
    private int udpPort;
    private String ipAddr;
    public int udpPort() {return udpPort;}
    public String ipAddr() {return new String(ipAddr);}
    public UdpPkt(String label, int udpPort, String ipAddr, ByteBuffer buf)
    {
        super(label, buf);
        this.udpPort=udpPort;
        this.ipAddr=ipAddr;
    }
    public UdpPkt(String label, int udpPort, String ipAddr, ArrayList<Byte> buf)
    {
        super(label, buf);
        this.udpPort=udpPort;
        this.ipAddr=ipAddr;
    }
}
//---------------------------------------------------------------------------------------------------------------------
