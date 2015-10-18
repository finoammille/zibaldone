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

namespace Z
{
//-------------------------------------------------------------------------------------------
TcpConnHandler::TcpConnHandler(SOCKET sockId):_sockId(sockId), reader(sockId)
{
    exit = false;
    std::stringstream sap;
    sap<<_sockId;
    _sap = sap.str();
    register2Label(getTxDataLabel());//self-registration to transmission request event emitted by class-users
}

void TcpConnHandler::run()
{
    while(!exit){
        Event* Ev = pullOut();
        if(Ev){
            ziblog(LOG::INF, "received event with label %s", Ev->label().c_str());
            if(dynamic_cast<StopThreadEvent *>(Ev)) exit = true;
            else if(dynamic_cast<RawByteBufferData *>(Ev)) {
                RawByteBufferData* txDataEvent = (RawByteBufferData*)Ev;
                int ret = send(_sockId, (const char*)txDataEvent->buf(), txDataEvent->len(), 0);
                if (ret == SOCKET_ERROR) {
                    ziblog(LOG::ERR, "send failed: %d", WSAGetLastError());
                    closesocket(_sockId);
                    WSACleanup();
                    delete Ev;
                    return;
                }
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
	reader.Join();
	Thread::Stop();
}

void TcpConnHandler::Join()
{
    if(alive()) {
        reader.Join();
        Thread::Stop();
    } else reader.Stop();
}

TcpConnHandler::Reader::Reader(SOCKET sockId) : exit(false), _sockId(sockId){}

void TcpConnHandler::Reader::run()
{
    std::stringstream sap;
    sap<<_sockId;
    std::string rxDataLabel = "rxDataEvent"+sap.str();
    const int maxSize = 1024;
    char rxbyte[maxSize];
    fd_set rdfs;//file descriptor set
	int ret=0;
    while(!exit) {
		FD_ZERO(&rdfs);
		FD_SET(_sockId, &rdfs);
		TIMEVAL tv={0};
		tv.tv_sec = 1;//1 sec
		tv.tv_usec = 0;
		ret=select(0, &rdfs, NULL, NULL, &tv);
		if(ret) {
			if(ret==SOCKET_ERROR) ziblog(LOG::ERR, "select error (%d)", WSAGetLastError());
			else {
				if(FD_ISSET(_sockId, &rdfs)) {//available data on socket
					int len = 0;
					if((len = recv(_sockId, rxbyte, maxSize, 0)) > 0) {
						RawByteBufferData rx(rxDataLabel, (unsigned char*)rxbyte, len);
						rx.emitEvent();
					} else if(len == 0) break;/* the peer has closed the socket. We have to exit thread loop.
                                               * We do not have to close socket here because it's up to the
                                               * destructor ~TcpConnHandler
                                               */
				} /* note: select returns if there are available data on socket and in that case sets _sockId in rdfs,
				   * or because of timeout (in our case set to 1 sec). Therefore if the Stop() method has been called
                   * then exit=true that allows a fair thread exit
				   */
			}
		}
    }
}

void TcpConnHandler::Reader::Start()
{
    exit=false;
    Thread::Start();
}

void TcpConnHandler::Reader::Stop() {exit=true;}

TcpConnHandler::~TcpConnHandler()
{
    shutdown(_sockId, SD_SEND);
    closesocket(_sockId);
    WSACleanup();
}

TcpServer::TcpServer(int port)
{
    _sockId = INVALID_SOCKET;
    WSADATA wsaData;
    //Initialize Winsock
    int ret = WSAStartup(MAKEWORD(2,2), &wsaData);
    if(ret != 0) {
        ziblog(LOG::ERR, "WSAStartup failed: %d", ret);
        return;
    }
    std::stringstream ssPort;
    ssPort << port;
    struct addrinfo *result = NULL, hints;
    ZeroMemory(&hints, sizeof (hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;
    //Resolve the local address and port to be used by the server
    ret = getaddrinfo(NULL, ssPort.str().c_str(), &hints, &result);
    if (ret != 0) {
        ziblog(LOG::ERR, "getaddrinfo failed: %d", ret);
        WSACleanup();
        return;
    }
    _sockId = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (_sockId == INVALID_SOCKET) {
        ziblog(LOG::ERR, "Error at socket(): %d", WSAGetLastError());
        freeaddrinfo(result);
        WSACleanup();
        return;
    }
    //Setup the TCP listening socket
    ret = bind(_sockId, result->ai_addr, (int)result->ai_addrlen);
    if (ret == SOCKET_ERROR) {
        ziblog(LOG::ERR, "bind failed with error: %d", WSAGetLastError());
        freeaddrinfo(result);
        closesocket(_sockId);
        WSACleanup();
        return ;
    }
    freeaddrinfo(result);
    //Listen
    if (listen(_sockId, SOMAXCONN ) == SOCKET_ERROR ) {
        ziblog(LOG::ERR, "Listen failed with error: %d", WSAGetLastError());
        closesocket(_sockId);
        WSACleanup();
        return;
    }
}

TcpConnHandler* TcpServer::Accept()
{
    SOCKET sockTcpConnHandlerId = accept(_sockId, NULL, NULL);//blocks here until a connection request comes
    if (sockTcpConnHandlerId == INVALID_SOCKET) {
        ziblog(LOG::ERR, "accept failed: %d", WSAGetLastError());
        closesocket(_sockId);
        WSACleanup();
        return NULL;
    } else {
        closesocket(_sockId);//No longer need server socket
        return new TcpConnHandler(sockTcpConnHandlerId);//instantiate the Thread who handles the connection
    }
}

TcpClient::TcpClient(const std::string& remoteAddr, int port)
{
    _sockId = INVALID_SOCKET;
    WSADATA wsaData;
    //Initialize Winsock
    int ret = WSAStartup(MAKEWORD(2,2), &wsaData);
    if(ret != 0) {
        ziblog(LOG::ERR, "WSAStartup failed: %d", ret);
        return;
    }
    _sockId = INVALID_SOCKET;
    ai = NULL;
    std::stringstream ssPort;
    ssPort << port;
    struct addrinfo *result = NULL, hints;
    ZeroMemory(&hints, sizeof (hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    //Resolve the server address and port
    ret = getaddrinfo(remoteAddr.c_str(), ssPort.str().c_str(), &hints, &result);
    if (ret != 0) {
        ziblog(LOG::ERR, "getaddrinfo failed: %d", ret);
        WSACleanup();
        return;
    }
    ai = result;
    // Create a SOCKET for connecting to server
    _sockId = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
    if (_sockId == INVALID_SOCKET) {
        ziblog(LOG::ERR, "Error at socket(): %d", WSAGetLastError());
        freeaddrinfo(result);
        WSACleanup();
        return;
    }
}

TcpConnHandler* TcpClient::Connect()
{
    // Connect to server.
    int ret = connect(_sockId, ai->ai_addr, (int)ai->ai_addrlen);
    if (ret == SOCKET_ERROR) {
        closesocket(_sockId);
        _sockId = INVALID_SOCKET;
    }
    freeaddrinfo(ai);
    if (_sockId == INVALID_SOCKET) {
        ziblog(LOG::ERR, "Unable to connect to server!");
        WSACleanup();
        return NULL;
    } else return new TcpConnHandler(_sockId);
}
//-------------------------------------------------------------------------------------------
}//namespace Z
