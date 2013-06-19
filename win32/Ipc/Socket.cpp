/*
 *
 * zibaldone - a C++ library for Thread, Timers and other Stuff
 *
 * Copyright (C) 2012  Antonio Buccino
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
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

#include "Socket.h"
#include <sstream>

namespace Z
{
//-------------------------------------------------------------------------------------------
Socket::Socket()
{
    _sockId = INVALID_SOCKET;
    WSADATA wsaData;
    //Initialize Winsock
    int ret = WSAStartup(MAKEWORD(2,2), &wsaData);
    if(ret != 0) {
        ziblog(LOG::ERR, "WSAStartup failed: %d", ret);
        return;
    }
}

ConnHandler::ConnHandler(SOCKET sockId):_sockId(sockId), reader(_sockId)
{
    exit = false;
    std::stringstream sap;
    sap<<_sockId;
    _sap = sap.str();
    register2Event(getTxDataEventId());//autoregistrazione sugli eventi di richiesta di trasmissione inviatigli dagli utilizzatori
}

void ConnHandler::run()
{
    while(!exit){
        Event* Ev = pullOut();
        if(Ev){
            std::string eventId = Ev->eventId();
            ziblog(LOG::INF, "received event %s", eventId.c_str());
            if(eventId == StopThreadEventId) exit = true;
            if(eventId == getTxDataEventId()) {
                int ret = send(_sockId, (const char*)Ev->buf(), Ev->len(), 0);
                delete Ev;
                if (ret == SOCKET_ERROR) {
                    ziblog(LOG::ERR, "send failed: %d", WSAGetLastError());
                    closesocket(_sockId);
                    WSACleanup();
                    return;
                }
            }
        }
    }
}

void ConnHandler::Start()
{
    Thread::Start();
    reader.Start();
}

void ConnHandler::Stop()
{
    Thread::Stop();
    reader.Stop();
}

void ConnHandler::Join()
{
    if(alive()) {
        reader.Join();
        Thread::Stop();
    } else reader.Stop();
}

ConnHandler::Reader::Reader(const SOCKET& sockId):_sockId(sockId){}

void ConnHandler::Reader::run()
{
    std::stringstream sap;
    sap<<_sockId;
    std::string rxDataEventId = "rxDataEvent"+sap.str();
    const int maxSize = 1024;
    char rxbyte[maxSize];
    for(;;) {
        int len = 0;
        if((len = recv(_sockId, rxbyte, maxSize, 0)) > 0) {
            Event rx(rxDataEventId, (unsigned char*)rxbyte, len);
            rx.emitEvent();
        } else if(len == 0) break; /* il peer ha chiuso il socket. Esco dal loop del thread. La close
                                    * del socket non devo farla qui perche' la fa gia' il distruttore
                                    * di ConnHandler
                                    */
    }
}

void ConnHandler::Reader::Stop(){kill();}

ConnHandler::~ConnHandler()
{
    shutdown(_sockId, SD_SEND);
    closesocket(_sockId);
    WSACleanup();
}

Server::Server(int port)
{
    std::stringstream ssPort;
    ssPort << port;
    struct addrinfo *result = NULL, hints;
    ZeroMemory(&hints, sizeof (hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;
    //Resolve the local address and port to be used by the server
    int ret = getaddrinfo(NULL, ssPort.str().c_str(), &hints, &result);
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

ConnHandler* Server::Accept()
{
    SOCKET sockConnHandlerId = accept(_sockId, NULL, NULL);//rimane bloccato qui sinche` non arriva una richiesta di connessione.
    if (sockConnHandlerId == INVALID_SOCKET) {
        ziblog(LOG::ERR, "accept failed: %d", WSAGetLastError());
        closesocket(_sockId);
        WSACleanup();
        return NULL;
    } else {
        closesocket(_sockId);//No longer need server socket
        return new ConnHandler(sockConnHandlerId);//istanzio il thread che gestisce la connessione
    }
}

Client::Client(const std::string& remoteAddr, int port)
{
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
    int ret = getaddrinfo(remoteAddr.c_str(), ssPort.str().c_str(), &hints, &result);
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

ConnHandler* Client::Connect()
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
    } else return new ConnHandler(_sockId);
}
//-------------------------------------------------------------------------------------------
}//namespace Z
