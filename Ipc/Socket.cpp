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

namespace Z
{
//-------------------------------------------------------------------------------------------
Socket::Socket(IpcType type):_ipcType(type)
{
    int socketType = (_ipcType == LocalIpc ? PF_LOCAL : AF_INET);
    if((_sockId = socket(socketType, SOCK_STREAM, 0))<0) {
        ziblog(LOG::ERROR, "Socket Error (%d)!", _sockId);
        return;
    }
    if(_ipcType == NetworkIpc) {
        int optionValue = 1;
        if((setsockopt(_sockId, SOL_SOCKET, SO_REUSEADDR, &optionValue, sizeof(optionValue))<0) ||
           (setsockopt(_sockId, SOL_SOCKET, SO_KEEPALIVE, &optionValue, sizeof(optionValue))<0)) ziblog(LOG::ERROR, "Socket Error (%d)!", errno);
    }
}

ConnHandler::ConnHandler(int sockId):_sockId(sockId)
{
    exit = false;
    std::stringstream sap;
    sap<<_sockId;
    _sap = sap.str();
    txDataEventId="txDataEvent"+_sap;
    rxDataEventId="rxDataEvent"+_sap;
    register2Event(txDataEventId);//autoregistrazione sugli eventi di richiesta di trasmissione inviatigli dagli utilizzatori
}

void ConnHandler::run()
{
    const int maxSize = 1024;
    unsigned char rxbyte[maxSize];
    int nonBlockingMode;//occorre ora cambiare la modalita' di accesso al file descriptor del socket e renderla non bloccante,
                        //altrimenti il thread resta inchiodato sulla read senza poter fare altro sinche` non arriva qualcosa...
    if((nonBlockingMode = fcntl(_sockId, F_GETFL) < 0)) {
        ziblog(LOG::ERROR, "Setting Non Blocking mode on Socket (%d) has failed!", _sockId);
        return;
    }
    nonBlockingMode = (nonBlockingMode | O_NONBLOCK);
    fcntl(_sockId, F_SETFL, nonBlockingMode);
    while(!exit){
        int len = 0;
        if((len = read(_sockId, rxbyte, maxSize)) > 0) {
            Event rx(rxDataEventId, rxbyte, len);
            rx.emitEvent();
        } else if(len == 0) break; /* il peer ha chiuso il socket. Esco dal loop del thread. La close
                                    * del socket non devo farla qui perche' la fa gia' il distruttore
                                    * di ConnHandler
                                    */
        Event* Ev = pullOut(10);//max 1/100 di secondo di attesa
        if(Ev){
            std::string eventId = Ev->eventId();
            ziblog(LOG::INFO, "received event %s", eventId.c_str());
            if(eventId == StopThreadEventId) exit = true;
            if(eventId == txDataEventId) {
                write(_sockId, Ev->buf(), Ev->len());
                delete Ev;
            }
        }
    }
}

Server::Server(int port):Socket(Socket::NetworkIpc)
{
    sockaddr_in sockAddr;
    sockAddr.sin_family = AF_INET;
    sockAddr.sin_addr.s_addr = INADDR_ANY;
    sockAddr.sin_port = htons(port);    
    bindAndListen((sockaddr *)&sockAddr, sizeof(sockAddr));
}

Server::Server(const std::string& IpcSocketName):Socket(Socket::LocalIpc)
{
    sockaddr_un sockAddr;
    sockAddr.sun_family = PF_UNIX;
    strcpy (sockAddr.sun_path, IpcSocketName.c_str());
    unlink(sockAddr.sun_path);
    bindAndListen((sockaddr *)&sockAddr, SUN_LEN(&sockAddr));
}

void Server::bindAndListen(sockaddr *addr, int addrlen)
{
    if(bind(_sockId, addr, addrlen)<0){
        ziblog(LOG::ERROR, "Socket Bind Error (%d)!", _sockId);
        return;
    }
    if(listen(_sockId, 1) < 0) ziblog(LOG::ERROR, "Socket Listening Error");
}

ConnHandler* Server::Accept()
{
    int sockConnHandlerId = accept(_sockId, NULL, NULL);//rimane bloccato qui sinche` non arriva una richiesta di connessione.
    if(sockConnHandlerId < 0) {
        ziblog(LOG::ERROR, "Connection Error (%d)", errno);
        return NULL;
    } else return new ConnHandler(sockConnHandlerId);//istanzio il thread che gestisce la connessione 
}

Client::Client(const std::string& remoteAddr, int port):Socket(Socket::NetworkIpc)
{
    sockaddr_in* addr = new sockaddr_in();
    addr->sin_family = AF_INET;
    addr->sin_port = htons(port);
    _addrlen = sizeof(*addr);
    if(!inet_pton(AF_INET, remoteAddr.c_str(), &addr->sin_addr)) ziblog(LOG::WARNING, "inet_pton (%s) has failed!", remoteAddr.c_str());
    _addr = (sockaddr* )addr;
}

Client::Client(const std::string& IpcSocketName):Socket(Socket::LocalIpc)
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
        ziblog(LOG::ERROR, "connection to %s has failed (%d)", inet_ntoa(((struct sockaddr_in *)_addr)->sin_addr), errno);
        return NULL;
    }
    return new ConnHandler(_sockId);
}
//-------------------------------------------------------------------------------------------
}//namespace Z
