/*
 *
 * zibaldone - a C++/Java library for Thread, Timers and other Stuff
 *
 * http://sourceforge.net/projects/zibaldone/
 *
 * version 3.2.0, February 14th, 2016
 *
 * Copyright (C) 2012  bucc4neer (bucc4neer@users.sourceforge.net)
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
 * bucc4neer bucc4neer@users.sourceforge.net
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
                                     //autoregistrazione sugli eventi di richiesta di trasmissione inviatigli dagli utilizzatori
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
    //N.B.: non e` possibile che reader stia girando ma TcpConnHandler sia terminato.
    reader.Join();
    Thread::Stop();
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
                                              //dati presenti sul socket
					int len = 0;
					if((len = recv(_sockId, rxbyte, maxSize, 0)) > 0) {
						RawByteBufferData rx(rxDataLabel, (unsigned char*)rxbyte, len);
						rx.emitEvent();
					} else if(len == 0) break;  /* the peer has closed the socket. We have to exit thread loop.
                                                 * We do not have to close socket here because it's up to the
                                                 * destructor ~TcpConnHandler
                                                 * :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
                                                 * il peer ha chiuso il socket. Esco dal loop del thread. La close
                                                 * del socket non devo farla qui perche' la fa gia' il distruttore
                                                 * di TcpConnHandler
                                                 */
				} /* N.B.: select returns if there are available data on socket and sets _sockId in rdfs, or
				   * if timeout (1 sec in our case). So. if Stop() is called then exit is set to true and
                   * thread exits gently
                   * :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
                   * nota: la select ritorna se c'e` qualcosa sul socket, e nel caso setta _sockId in rdfs, oppure
				   * per timeout (nel nostro caso impostato ad 1 secondo). Quindi se viene chiamato il metodo Stop()
				   * questo imposta exit=true che permette l'uscita pulita dal thread.
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
    SOCKET sockTcpConnHandlerId = accept(_sockId, NULL, NULL);//rimane bloccato qui sinche` non arriva una richiesta di connessione.
    if (sockTcpConnHandlerId == INVALID_SOCKET) {
        ziblog(LOG::ERR, "accept failed: %d", WSAGetLastError());
        closesocket(_sockId);
        WSACleanup();
        return NULL;
    } else {
        closesocket(_sockId);//No longer need server socket
        return new TcpConnHandler(sockTcpConnHandlerId);//istanzio il thread che gestisce la connessione
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
