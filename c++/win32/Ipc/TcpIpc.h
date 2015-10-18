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
