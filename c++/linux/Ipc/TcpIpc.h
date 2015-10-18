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
#define	_TPCIPC_H

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
#include "Log.h"

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
//------------------------------------------------------------------------------
class TcpConnHandler : public Thread {
    friend class TcpServer;
    friend class TcpClient;
    int _sockId;
    std::string _sap;
    bool exit;
    void run();
    class Reader : public Thread {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,22)
        int efd;
#endif
        bool exit;
        int _sockId;
        void run();
    public:
        Reader(int);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,22)
		~Reader();
#endif
        void Start();
        void Stop();
    } reader;
    TcpConnHandler(int sockId);
public:
    ~TcpConnHandler();
    void Start();
    void Stop();
    void Join();
    std::string getTxDataLabel(){return "txDataEvent"+_sap;}
    std::string getRxDataLabel(){return "rxDataEvent"+_sap;}
};

class TcpServer {
    int _sockId;
public:
    TcpServer(const int port);
    ~TcpServer();
    TcpConnHandler* Accept();
private:
    /*
     * the well known big 3 rule of C++ says:
     * "if your class needs to define the destructor or the copy constructor or the copy
     * assigment operator then it should probably explicitly define all three"
     * In our case since a Thread cannot be assigned or copied, we declare the
     * copy constructor and the assignment operator as private. This way we can
     * prevent and easily detect any inadverted abuse
     */
    TcpServer(const TcpServer &);
    TcpServer & operator = (const TcpServer &);
};

class TcpClient {
    int _sockId;
    sockaddr* _addr;
    int _addrlen;
public:
    TcpClient(std::string& remoteAddr, int port);
    ~TcpClient();
    TcpConnHandler* Connect();
private:
    /*
     * the well known big 3 rule of C++ says:
     * "if your class needs to define the destructor or the copy constructor or the copy
     * assigment operator then it should probably explicitly define all three"
     * In our case since a Thread cannot be assigned or copied, we declare the
     * copy constructor and the assignment operator as private. This way we can
     * prevent and easily detect any inadverted abuse
     */
    TcpClient(const TcpClient &);
    TcpClient & operator = (const TcpClient &);
};
//------------------------------------------------------------------------------
}//namespace Z
#endif	/* _TPCIPC_H */
