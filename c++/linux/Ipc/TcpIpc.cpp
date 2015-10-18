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

#include "TcpIpc.h"
#include "Events.h"

namespace Z
{
//-------------------------------------------------------------------------------------------
TcpConnHandler::TcpConnHandler(int sockId):_sockId(sockId), reader(sockId)
{
    exit = false;
    std::stringstream sap;
    sap<<_sockId;
    _sap = sap.str();
    register2Label(getTxDataLabel());//self-registration to transmission request event emitted by class-users
}

TcpConnHandler::~TcpConnHandler() {close(_sockId);}

void TcpConnHandler::run()
{
    while(!exit){
        Event* Ev = pullOut();
        if(Ev){
            ziblog(LOG::INF, "received event with label %s", Ev->label().c_str());
            if(dynamic_cast<StopThreadEvent *>(Ev)) exit = true;
            else if(dynamic_cast<RawByteBufferData *>(Ev)) {
                RawByteBufferData* txDataEvent = (RawByteBufferData*)Ev;
                write(_sockId, txDataEvent->buf(), txDataEvent->len());
            } else ziblog(LOG::ERR, "unexpected event");
            delete Ev;
        }
    }
}

void TcpConnHandler::Start()
{
    Thread::Start();
    reader.Start();
}

void TcpConnHandler::Stop()
{
    reader.Stop();
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,22)//eventFd is not available with this version so we have to do polling
    reader.Join();
#endif
	Thread::Stop();
}

void TcpConnHandler::Join()
{
    if(alive()) {
        reader.Join();
        Thread::Stop();
    } else reader.Stop();
}

TcpConnHandler::Reader::Reader(int sockId) : exit(false), _sockId(sockId)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,22)
    if((efd=eventfd(0, O_NONBLOCK))==-1) ziblog(LOG::ERR, "efd error (%s)", strerror(errno));
#endif
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,22)

TcpConnHandler::Reader::~Reader() {close(efd);}

void TcpConnHandler::Reader::run()
{
    std::stringstream sap;
    sap<<_sockId;
    std::string rxDataLabel = "rxDataEvent"+sap.str();
    const int maxSize = 1024;
    unsigned char rxbyte[maxSize];
    fd_set rdfs;//file descriptor set
    int nfds=(efd>_sockId ? efd : _sockId)+1;
    while(!exit) {
        FD_ZERO(&rdfs);
        FD_SET(_sockId, &rdfs);
        FD_SET(efd, &rdfs);
        if(select(nfds, &rdfs, NULL, NULL, NULL)==-1) ziblog(LOG::ERR, "select error (%s)", strerror(errno));
        else {
            if(FD_ISSET(_sockId, &rdfs)) {//available data on socket
                int len = 0;
                if((len = read(_sockId, rxbyte, maxSize)) > 0) {
                    RawByteBufferData rx(rxDataLabel, rxbyte, len);
                    rx.emitEvent();
                } else if(len == 0) break; /* the peer has closed the socket. We have to exit thread loop.
                                            * We do not have to close socket here because it's up to the
                                            * destructor ~TcpConnHandler
                                            */
            } else if(FD_ISSET(efd, &rdfs)) {//stop event
                unsigned char exitFlag[8];
                if(read(efd, exitFlag, 8)==-1) ziblog(LOG::ERR, "read from efd error (%s)", strerror(errno));
                if(exitFlag[7]!=1) ziblog(LOG::ERR, "unexpected exitFlag value (%d)", exitFlag[7]);
                exit=true;
            } else ziblog(LOG::ERR, "select lied to me!");
        }
    }
}

#else//KERNEL OLDER THAN 2.6.22

void TcpConnHandler::Reader::run()
{
    std::stringstream sap;
    sap<<_sockId;
    std::string rxDataLabel = "rxDataEvent"+sap.str();
    const int maxSize = 1024;
    unsigned char rxbyte[maxSize];
    fd_set rdfs;//file descriptor set
	int nfds=_sockId+1;
	timeval tv;
    while(!exit) {
        FD_ZERO(&rdfs);
        FD_SET(_sockId, &rdfs);
		tv.tv_sec = 1;//1 sec polling to catch any Stop()
		tv.tv_usec = 0;
        int ret = select(nfds, &rdfs, NULL, NULL, &tv);
		if(ret) {
			if (ret<0) ziblog(LOG::ERR, "select error (%s)", strerror(errno));
			else {
				if(FD_ISSET(_sockId, &rdfs)) {//available data on socket
					int len = 0;
					if((len = read(_sockId, rxbyte, maxSize)) > 0) {
						RawByteBufferData rx(rxDataLabel, rxbyte, len);
						rx.emitEvent();
					} else if(len == 0) break; /* the peer has closed the socket. We have to exit thread loop.
                                                * We do not have to close socket here because it's up to the
                                                * destructor ~TcpConnHandler
                                                */
				} /* N.B.: select returns if there are available data on socket and sets _sockId in rdfs, or
				   * if timeout (1 sec in our case). So. if Stop() is called then exit is set to true and
                   * thread exits gently
                   */
			}
		}
    }
}

#endif

void TcpConnHandler::Reader::Start()
{
    exit=false;
    Thread::Start();
}

void TcpConnHandler::Reader::Stop()
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,22)
    unsigned char exitFlag[]={0,0,0,0,0,0,0,1};
    if(write(efd, exitFlag, 8)==-1) ziblog(LOG::ERR, "reader stop error (%s)", strerror(errno));
#else
	exit=true;
#endif
}

TcpServer::TcpServer(int port)
{
    if((_sockId = socket(AF_INET, SOCK_STREAM, 0))<0) {
        ziblog(LOG::ERR, "Socket Error (%s)!", strerror(errno));
        return;
    }
    int optionValue = 1;
    if((setsockopt(_sockId, SOL_SOCKET, SO_REUSEADDR, &optionValue, sizeof(optionValue))<0) ||
       (setsockopt(_sockId, SOL_SOCKET, SO_KEEPALIVE, &optionValue, sizeof(optionValue))<0)) ziblog(LOG::ERR, "Socket Error (%s)!", strerror(errno));
    sockaddr_in sockAddr;
    sockAddr.sin_family = AF_INET;
    sockAddr.sin_addr.s_addr = INADDR_ANY;
    sockAddr.sin_port = htons(port);
    if(bind(_sockId, (sockaddr *)&sockAddr, sizeof(sockAddr))<0){
        ziblog(LOG::ERR, "Socket Bind Error (%s)!", strerror(errno));
        return;
    }
    if(listen(_sockId, 1) < 0) ziblog(LOG::ERR, "Socket Listening Error (%s)", strerror(errno));
}

TcpServer::~TcpServer() {close(_sockId);}

TcpConnHandler* TcpServer::Accept()
{
    int sockTcpConnHandlerId = accept(_sockId, NULL, NULL);
    if(sockTcpConnHandlerId < 0) {
        ziblog(LOG::ERR, "Connection Error (%s)", strerror(errno));
        return NULL;
    } else return new TcpConnHandler(sockTcpConnHandlerId);
}

TcpClient::TcpClient(std::string& remoteAddr, int port)
{
    if((_sockId = socket(AF_INET, SOCK_STREAM, 0))<0) {
        ziblog(LOG::ERR, "Socket Error (%s)!", strerror(errno));
        return;
    }
    int optionValue = 1;
    if((setsockopt(_sockId, SOL_SOCKET, SO_REUSEADDR, &optionValue, sizeof(optionValue))<0) ||
       (setsockopt(_sockId, SOL_SOCKET, SO_KEEPALIVE, &optionValue, sizeof(optionValue))<0)) ziblog(LOG::ERR, "Socket Error (%s)!", strerror(errno));
    if(remoteAddr=="localhost") remoteAddr="127.0.0.1";
    sockaddr_in* addr = new sockaddr_in();
    addr->sin_family = AF_INET;
    addr->sin_port = htons(port);
    _addrlen = sizeof(*addr);
    if(!inet_pton(AF_INET, remoteAddr.c_str(), &addr->sin_addr)) ziblog(LOG::ERR, "inet_pton (%s) has failed!", remoteAddr.c_str());
    _addr = (sockaddr* )addr;
}

TcpClient::~TcpClient(){delete((sockaddr_in*)_addr); _addr=NULL;}

TcpConnHandler* TcpClient::Connect()
{
    if(connect(_sockId, _addr, _addrlen) < 0) {
        ziblog(LOG::ERR, "connection to %s has failed (%s)", inet_ntoa(((struct sockaddr_in *)_addr)->sin_addr), strerror(errno));
        return NULL;
    }
    return new TcpConnHandler(_sockId);
}
//-------------------------------------------------------------------------------------------
}//namespace Z
