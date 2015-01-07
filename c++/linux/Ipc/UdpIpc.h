/*
 *
 * zibaldone - a C++/Java library for Thread, Timers and other Stuff
 *
 * Copyright (C) 2012  Antonio Buccino
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 2.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef _UDPIPC_H
#define	_UDPIPC_H

#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>
#include <sstream>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/eventfd.h>

#include "Thread.h"
#include "Events.h"
#include "Log.h"

//------------------------------------------------------------------------------
/*
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
    const int udpPort;//porta su cui riceve il thread che gestisce l'end point Udp
    int _sockId;
    sockaddr_in _addr;
    std::string _sap;
    bool exit;
    void run();//write loop
    class Reader : public Thread {
        friend class Udp;//per poter referenziare _sockId e _addr
        int efd; //Event file descriptor
        bool exit;
        int _sockId;
        sockaddr_in _addr;
        void run();//read loop
    public:
        Reader();
        ~Reader();
        void Start();
        void Stop();
    } reader;
public:
    Udp(int udpPort);
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
#endif	/* _UDPIPC_H */
