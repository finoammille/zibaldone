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

#ifndef _UDPIPC_H
#define _UDPIPC_H

#include <winsock2.h>
#include <ws2tcpip.h>
#include "Log.h"
#include "Thread.h"
#include "Events.h"

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
namespace Z
{
//------------------------------------------------------------------------------
/*
    UdpPkt Event

    event for tx/rx of an udp datagram

    it's basically a RawByteBufferData extended with a string for ip address and
    a int for udp port. The ip address is meant to be the sender ip address if
    the event label is = to getRxDataLabel() or the destination ip address if
    label = getTxDataLabel(). In other words UdpPkt is a RawByteBufferData with
    the addition of the "udp coordinates"

    ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

    evento UdpPkt

    evento per la tx/rx di un pacchetto udp.

    sostanzialmente e` un rawbytebufferdata esteso con l'aggiunta di una string
    che contiene l'indirizzo ip e la porta udp del destinatario o del mittente a
    seconda che la label dell'evento sia gettxdatalabel oppure getrxdatalabel.
    in altre parole e` un rawbytebufferdata con in piu` le "coordinate udp".

    nota: il tcp usa semplicemente rawbytebufferdata perche` una volta stabilita
          la connessione i dati viaggano su un collegamento connesso punto-punto,
          invece udp e` connectionless.
*/
class UdpPkt : public RawByteBufferData {
    const int udpPort;
    const std::string ipAddr;
    virtual Event* clone()const;
public:
    UdpPkt(const std::string& label, const int udpPort, const std::string& ipAddr, const unsigned char* buf, const int len);
    UdpPkt(const std::string& label, const int udpPort, const std::string& ipAddr, const std::vector<unsigned char>&);
    int UdpPort() const;
    std::string IpAddr() const;
};
//------------------------------------------------------------------------------
class Udp : public Thread {
    const int udpPort;
    SOCKET _sockId;
    sockaddr_in _addr;
    std::string _sap;//serve per taggare univocamente gli eventi relativi ad uno specifico socket
    bool exit;
    void run();//write loop
    class Reader : public Thread {
        friend class Udp;//per poter referenziare _sockId
        bool exit;
        SOCKET _sockId;
        sockaddr_in _addr;
        void run();//read loop
    public:
        void Start();
        void Stop();
    } reader;
public:
    Udp(int port);
    ~Udp();
    int UdpPort() const;
    void Start();
    void Stop();
    void Join();
    std::string getTxDataLabel(){return "dgramTxDataEvent"+_sap;}//id dell'evento di richiesta trasmissione datagram
    std::string getRxDataLabel(){return "dgramRxDataEvent"+_sap;}//id dell'evento di notifica ricezione datagram
};
//------------------------------------------------------------------------------
}//namespace Z
#endif /* _UDPIPC_H */
