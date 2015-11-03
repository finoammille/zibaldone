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

#ifndef _TCPIPC_H
#define _TCPIPC_H

#include <winsock2.h>
#include <ws2tcpip.h>
#include "Log.h"
#include "Thread.h"
#include "Events.h"

/*
Use:

1) instantiate a TcpServer object specifying the listening port "P"
2) instantiate a TcpClient object specifying IP and port P of
   listening server
3) to connect TcpClient with TcpServer, you have to call Accept() on server
   side (TcpServer) and then you have to call Connect() on client side
   (TcpClient). Both methods return (respectively on server side and
   client side) a pointer to an object of type TcpConnHandler
4) to receive data from the connection you have to register on event of
   type RawByteBufferData whose label is returned by
   TcpConnHandler::getRxDataLabel()
5) to transmit data over the connection you have to emit an Event of type
   RawByteBufferData with label set to the label returned by the
   ConnHandler::getTxDataLabel() method
*/

namespace Z
{
//-------------------------------------------------------------------------------------------
class TcpConnHandler : public Thread {
    friend class TcpServer;
    friend class TcpClient;
    SOCKET _sockId;
    std::string _sap;
    bool exit;
    void run();
    class Reader : public Thread {
        bool exit;
        SOCKET _sockId;
        void run();
    public:
        Reader(SOCKET);
        void Start();
        void Stop();
    } reader;
    TcpConnHandler(SOCKET sockId);
public:
    ~TcpConnHandler();
    void Start();
    void Stop();
    void Join();
    std::string getTxDataLabel(){return "txDataEvent"+_sap;}
    std::string getRxDataLabel(){return "rxDataEvent"+_sap;}
};

class TcpServer {
    SOCKET _sockId;
public:
    TcpServer(int port);
    TcpConnHandler* Accept();
};

class TcpClient {
    SOCKET _sockId;
    struct addrinfo *ai;
public:
    TcpClient(const std::string& remoteAddr, int port);
    TcpConnHandler* Connect();
};

//-------------------------------------------------------------------------------------------
}//namespace Z
#endif /* _TCPIPC_H */
