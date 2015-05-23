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

#include "PfLocalIpc.h"
#include "Events.h"

namespace Z
{
//-------------------------------------------------------------------------------------------
PfLocalConnHandler::PfLocalConnHandler(int sockId, const std::string& IpcSocketName):
                    _sockId(sockId), _IpcSocketName(IpcSocketName), reader(sockId)
{
    exit = false;
    std::stringstream sap;
    sap<<_sockId;
    _sap = sap.str();
    register2Label(getTxDataLabel());//autoregistrazione sugli eventi di richiesta di trasmissione inviatigli dagli utilizzatori
}

PfLocalConnHandler::~PfLocalConnHandler() 
{
    close(_sockId);
    if(!_IpcSocketName.empty()) unlink(_IpcSocketName.c_str());
}

void PfLocalConnHandler::run()
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

void PfLocalConnHandler::Start()
{
    Thread::Start();
    reader.Start();
}

void PfLocalConnHandler::Stop()
{
    reader.Stop();
    Thread::Stop();
}

void PfLocalConnHandler::Join()
{
    if(alive()) {
        reader.Join();
        Thread::Stop();
    } else reader.Stop();
}

PfLocalConnHandler::Reader::Reader(int sockId):exit(false), _sockId(sockId)
{
    if((efd=eventfd(0, O_NONBLOCK))==-1) ziblog(LOG::ERR, "efd error (%s)", strerror(errno));
}

PfLocalConnHandler::Reader::~Reader(){close(efd);}

void PfLocalConnHandler::Reader::run()                                                                                                                                           
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
                                            * di PfLocalConnHandler
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

void PfLocalConnHandler::Reader::Start()
{
    exit=false;
    Thread::Start();
}

void PfLocalConnHandler::Reader::Stop()
{
    unsigned char exitFlag[]={0,0,0,0,0,0,0,1};
    if(write(efd, exitFlag, 8)==-1) ziblog(LOG::ERR, "reader stop error (%s)", strerror(errno));
}

PfLocalServer::PfLocalServer(const std::string& IpcSocketName):_IpcSocketName(IpcSocketName)
{
    if((_sockId = socket(PF_LOCAL, SOCK_STREAM, 0))<0) {
        ziblog(LOG::ERR, "Socket Error (%s)!", strerror(errno));
        return;
    }   
    sockaddr_un sockAddr;
    sockAddr.sun_family = PF_UNIX;
    strcpy (sockAddr.sun_path, IpcSocketName.c_str());
    unlink(sockAddr.sun_path);//rimuove un eventuale refuso
    if(bind(_sockId, (sockaddr *)&sockAddr, SUN_LEN(&sockAddr))<0){
        ziblog(LOG::ERR, "Socket Bind Error (%s)!", strerror(errno));
        return;
    }
    if(listen(_sockId, 1) < 0) ziblog(LOG::ERR, "Socket Listening Error");
}

PfLocalServer::~PfLocalServer() {close(_sockId);}

PfLocalConnHandler* PfLocalServer::Accept()
{    
    int sockPfLocalConnHandlerId = accept(_sockId, NULL, NULL);//rimane bloccato qui sinche` non arriva una richiesta di connessione.
    if(sockPfLocalConnHandlerId < 0) {
        ziblog(LOG::ERR, "Connection Error (%s)", strerror(errno));
        return NULL;
    } else return new PfLocalConnHandler(sockPfLocalConnHandlerId, _IpcSocketName);//istanzio il thread che gestisce la connessione 
    
}

PfLocalClient::PfLocalClient(const std::string& IpcSocketName)
{
    if((_sockId = socket(PF_LOCAL, SOCK_STREAM, 0))<0) {
        ziblog(LOG::ERR, "Socket Error (%s)!", strerror(errno));
        return;
    }
    sockaddr_un* addr = new sockaddr_un();
    addr->sun_family = PF_UNIX;
    strcpy(addr->sun_path, IpcSocketName.c_str());
    _addrlen = SUN_LEN(addr);
    _addr = (sockaddr* )addr;
}

PfLocalClient::~PfLocalClient() {delete((sockaddr_un*)_addr); _addr=NULL;}

PfLocalConnHandler* PfLocalClient::Connect()
{
    if(connect(_sockId, _addr, _addrlen) < 0) {
        ziblog(LOG::ERR, "connection to %s has failed (%s)", inet_ntoa(((struct sockaddr_in *)_addr)->sin_addr), strerror(errno));
        return NULL;
    }
    return new PfLocalConnHandler(_sockId, ((sockaddr_un*)_addr)->sun_path);
}
//-------------------------------------------------------------------------------------------
}//namespace Z
