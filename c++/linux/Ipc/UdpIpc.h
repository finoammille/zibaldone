/*
 *
 * zibaldone - a C++/Java library for Thread, Timers and other Stuff
 *
 * http://sourceforge.net/projects/zibaldone/
 *
 * version 3.1.2, August 29th, 2015
 *
 * Copyright (C) 2012  ilant (ilant@users.sourceforge.net)
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
 * ilant ilant@users.sourceforge.net
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

#include <linux/version.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,22)
#include <sys/eventfd.h>
#else
#include <sys/time.h>
#endif

#include "Thread.h"
#include "Events.h"
#include "Log.h"

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
    int _sockId;
    sockaddr_in _addr;
    std::string _sap;
    bool exit;
    void run();//write loop
    class Reader : public Thread {
        friend class Udp;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,22)
        int efd; //Event file descriptor
#endif
        bool exit;
        int _sockId;
        sockaddr_in _addr;
        void run();//read loop
    public:
        Reader();
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,22)
        ~Reader();
#endif
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
    std::string getTxDataLabel(){return "dgramTxDataEvent"+_sap;}
    std::string getRxDataLabel(){return "dgramRxDataEvent"+_sap;}
};
//------------------------------------------------------------------------------
}//namespace Z
#endif	/* _UDPIPC_H */
