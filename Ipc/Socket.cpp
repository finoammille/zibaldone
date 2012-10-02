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

Socket::Socket(IpcType type):_ipcType(type)
{
    int socketType = (_ipcType == LocalIpc ? PF_LOCAL : AF_INET);
    if((_sockId = socket(socketType, SOCK_STREAM, 0))<0) {
        ziblog("Socket Error (%d)!", _sockId);
        return;
    }
    if(_ipcType == NetworkIpc) {
        int optionValue = 1;
        if((setsockopt(_sockId, SOL_SOCKET, SO_REUSEADDR, &optionValue, sizeof(optionValue))<0) ||
           (setsockopt(_sockId, SOL_SOCKET, SO_KEEPALIVE, &optionValue, sizeof(optionValue))<0)) ziblog("Socket Error (%d)!", errno);
    }
}

ConnHandler::ConnHandler(int sockId):_sockId(sockId)
{
    exit = false;
    std::stringstream sap;
    sap<<_sockId;
    _sap = sap.str();
    int nonBlockingMode;
    if((nonBlockingMode = fcntl(_sockId, F_GETFL) < 0)) {
        ziblog("Setting Non Blocking mode on Socket (%d) has failed!", _sockId);
        return;
    }
    nonBlockingMode = (nonBlockingMode | O_NONBLOCK);
    fcntl(_sockId, F_SETFL, nonBlockingMode);
    register2Event(TxDataEvent::txDataEventName(_sap));//autoregistrazione sugli eventi di richiesta di trasmissione inviatigli dagli utilizzatori
}

void ConnHandler::run()
{
    const int maxSize = 1024;
    unsigned char rxbyte[maxSize];
    while(!exit){
        int len = 0;
        if((len = read(_sockId, rxbyte, maxSize)) > 0) {
            RxDataEvent rx(_sap, rxbyte, len);
            rx.emitEvent();
        } else if(len == 0) break; /* il peer ha chiuso il socket. Esco dal loop del thread. La close
                                    * del socket non devo farla qui perche' la fa gia' il distruttore
                                    * di ConnHandler
                                    */
        Event* Ev = pullOut(10);//max 1/100 di secondo di attesa
        if(Ev){
            std::string eventName = Ev->eventName();
            ziblog("received event %s", eventName.c_str());
            if(eventName == StopThreadEvent::StopThreadEventName) exit = true;
            if(eventName == TxDataEvent::txDataEventName(_sap)) {
                TxDataEvent* w = (TxDataEvent *)Ev;
                write(_sockId, w->buf(), w->len());
                delete w;
            }
        }
    }
}

Server::Server(int port, const int maxConnections):Socket(Socket::NetworkIpc), _maxConnections(maxConnections)
{
    _activeConnections = 0;
    sockaddr_in sockAddr;
    sockAddr.sin_family = AF_INET;
    sockAddr.sin_addr.s_addr = INADDR_ANY;
    sockAddr.sin_port = htons(port);    
    bindAndListen((sockaddr *)&sockAddr, sizeof(sockAddr));
}

Server::Server(std::string IpcSocketName, const int maxConnections):Socket(Socket::LocalIpc), _maxConnections(maxConnections)
{
    _activeConnections = 0;
    sockaddr_un sockAddr;
    sockAddr.sun_family = PF_UNIX;
    strcpy (sockAddr.sun_path, IpcSocketName.c_str());
    unlink(sockAddr.sun_path);
    bindAndListen((sockaddr *)&sockAddr, SUN_LEN(&sockAddr));
}

void Server::bindAndListen(sockaddr *addr, int addrlen)
{
    if(bind(_sockId,addr, addrlen)<0){
        ziblog("Socket Bind Error (%d)!", _sockId);
        return;
    }
    if(listen (_sockId, _maxConnections)<0) ziblog("Socket Listening Error");
}

ConnHandler* Server::Accept()
{
    for(;;) {
        int sockConnHandlerId;
        if((sockConnHandlerId = accept(_sockId, NULL, NULL)) < 0) {
            ziblog("Connection Error (%d)", errno);
            return NULL;
        }
        if(++_activeConnections = _maxConnections) {
            close(_sockId);//non accetto altre richieste di connessione.
        }
        return new ConnHandler(sockConnHandlerId);//istanzio il thread che gestisce la connessione
    }
}

Client::Client(std::string remoteAddr, int port):Socket(Socket::NetworkIpc)
{
    sockaddr_in* addr = new sockaddr_in();
    addr->sin_family = AF_INET;
    addr->sin_port = htons(port);
    _addrlen = sizeof(*addr);
    if(!inet_pton(AF_INET, remoteAddr.c_str(), &addr->sin_addr)) ziblog("inet_pton (%s) has failed!", remoteAddr.c_str());
    _addr = (sockaddr* )addr;
}

Client::Client(std::string IpcSocketName):Socket(Socket::LocalIpc)
{
    sockaddr_un* addr = new sockaddr_un();
    addr->sun_family = PF_UNIX;
    strcpy(addr->sun_path ,IpcSocketName.c_str());
    _addrlen = SUN_LEN(addr);
    _addr = (sockaddr* )addr;
}

Client::~Client()
{
    if(_ipcType == Socket::NetworkIpc)delete((sockaddr_in*)_addr);
    else delete((sockaddr_un*)_addr);//LocalIpc
}

ConnHandler* Client::Connect()
{
    if(connect(_sockId, _addr, _addrlen) < 0) {
        ziblog("connection to %s has failed (%d)", inet_ntoa(((struct sockaddr_in *)_addr)->sin_addr), errno);
        return NULL;
    }
    return new ConnHandler(_sockId);
}
