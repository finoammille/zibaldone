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
