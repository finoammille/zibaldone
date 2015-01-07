/*
 *
 * zibaldone - a C++/Java library for Thread, Timers and other Stuff
 *
 * Copyright (C) 2012  Antonio Buccino
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
    register2Label(getTxDataLabel());//autoregistrazione sugli eventi di richiesta di trasmissione inviatigli dagli utilizzatori
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
    if((efd=eventfd(0, O_NONBLOCK))==-1) ziblog(LOG::ERR, "efd error (%s)", strerror(errno));
}

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
            if(FD_ISSET(_sockId, &rdfs)) {//dati presenti sul socket
                int len = 0;
                if((len = read(_sockId, rxbyte, maxSize)) > 0) {
                    RawByteBufferData rx(rxDataLabel, rxbyte, len);
                    rx.emitEvent();
                } else if(len == 0) break; /* il peer ha chiuso il socket. Esco dal loop del thread. La close
                                            * del socket non devo farla qui perche' la fa gia' il distruttore
                                            * di TcpConnHandler
                                            */
            } else if(FD_ISSET(efd, &rdfs)) {//evento di stop
                unsigned char exitFlag[8];
                if(read(efd, exitFlag, 8)==-1) ziblog(LOG::ERR, "read from efd error (%s)", strerror(errno));
                if(exitFlag[7]!=1) ziblog(LOG::ERR, "unexpected exitFlag value (%d)", exitFlag[7]);
                exit=true;
            } else ziblog(LOG::ERR, "select lied to me!");
        }
    }
}

void TcpConnHandler::Reader::Start()
{
    exit=false;
    Thread::Start();
}

void TcpConnHandler::Reader::Stop()
{
    unsigned char exitFlag[]={0,0,0,0,0,0,0,1};
    if(write(efd, exitFlag, 8)==-1) ziblog(LOG::ERR, "reader stop error (%s)", strerror(errno));
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
    int sockTcpConnHandlerId = accept(_sockId, NULL, NULL);//rimane bloccato qui sinche` non arriva una richiesta di connessione.
    if(sockTcpConnHandlerId < 0) {
        ziblog(LOG::ERR, "Connection Error (%s)", strerror(errno));
        return NULL;
    } else return new TcpConnHandler(sockTcpConnHandlerId);//istanzio il thread che gestisce la connessione
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
