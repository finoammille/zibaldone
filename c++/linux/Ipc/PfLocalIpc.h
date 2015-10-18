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

#ifndef _PFLOCALIPC_H
#define	_PFLOCALIPC_H

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

1) instantiate a PfLocalServer object specifying the listening port "P"
2) instantiate a PfLocalClient object specifying IP and port P of
   listening server
3) to connect client and server, you have to call Accept() on server side
   and then you have to call Connect() on client side. Both methods return
   (respectively on server side and client side) a pointer to an object of
   type PfLocalConnHandler
4) to receive data from the connection you have to register on event of
   type RawByteBufferData whose label is returned by
   PfLocalConnHandler::getRxDataLabel()
5) to transmit data over the connection you have to emit an Event of type
   RawByteBufferData with label set to the label returned by the
   ConnHandler::getTxDataLabel() method
*/

namespace Z
{
//-------------------------------------------------------------------------------------------
class PfLocalConnHandler : public Thread {
    friend class PfLocalServer;
    friend class PfLocalClient;
    int _sockId;
    std::string _IpcSocketName;
    std::string _sap;
    bool exit;
    void run();
    class Reader : public Thread {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,22)
        int efd; //Event file descriptor
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
    PfLocalConnHandler(int sockId, const std::string& IpcSocketName="");
public:
    ~PfLocalConnHandler();
    void Start();
    void Stop();
    void Join();
    std::string getTxDataLabel(){return "txDataEvent"+_sap;}
    std::string getRxDataLabel(){return "rxDataEvent"+_sap;}
};

class PfLocalServer {
    int _sockId;
    const std::string _IpcSocketName;
public:
    PfLocalServer(const std::string& IpcSocketName);
    ~PfLocalServer();
    PfLocalConnHandler* Accept();
private:
    /*
     * the well known big 3 rule of C++ says:
     * "if your class needs to define the destructor or the copy constructor or the copy
     * assigment operator then it should probably explicitly define all three"
     * In our case since a Thread cannot be assigned or copied, we declare the
     * copy constructor and the assignment operator as private. This way we can
     * prevent and easily detect any inadverted abuse
     */
    PfLocalServer(const PfLocalServer &);
    PfLocalServer & operator = (const PfLocalServer &);
};

class PfLocalClient {
    int _sockId;
    sockaddr* _addr;
    int _addrlen;
public:
    PfLocalClient(const std::string& IpcSocketName);
    ~PfLocalClient();
    PfLocalConnHandler* Connect();
private:
    /*
     * the well known big 3 rule of C++ says:
     * "if your class needs to define the destructor or the copy constructor or the copy
     * assigment operator then it should probably explicitly define all three"
     * In our case since a Thread cannot be assigned or copied, we declare the
     * copy constructor and the assignment operator as private. This way we can
     * prevent and easily detect any inadverted abuse
     */
    PfLocalClient(const PfLocalClient &);
    PfLocalClient & operator = (const PfLocalClient &);
};
//-------------------------------------------------------------------------------------------
}//namespace Z
#endif	/* _PFLOCALIPC_H */
