/*
 *
 * zibaldone - a C++ library for Thread, Timers and other Stuff
 * http://sourceforge.net/projects/zibaldone/
 *
 * Copyright (C) 2012  ilant (ilant@users.sourceforge.net)
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
    std::string _sap;
    bool exit;
    void run();//write loop
    class Reader : public Thread {
        friend class Udp;
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
    std::string getTxDataLabel(){return "dgramTxDataEvent"+_sap;}
    std::string getRxDataLabel(){return "dgramRxDataEvent"+_sap;}
};
//------------------------------------------------------------------------------
}//namespace Z
#endif /* _UDPIPC_H */
