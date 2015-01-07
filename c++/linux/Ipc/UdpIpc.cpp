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

#include "UdpIpc.h"

namespace Z
{
//-------------------------------------------------------------------------------------------
/*
    UdpPkt
*/
UdpPkt::UdpPkt(const std::string& label, const int udpPort, const std::string& ipAddr, const unsigned char* buf, const int len):
        RawByteBufferData(label, buf, len), udpPort(udpPort), ipAddr(ipAddr) {}

UdpPkt::UdpPkt(const std::string& label, const int udpPort, const std::string& ipAddr, const std::vector<unsigned char>& data):
        RawByteBufferData(label, data), udpPort(udpPort), ipAddr(ipAddr) {}

Event* UdpPkt::clone()const{return new UdpPkt(*this);}

int UdpPkt::UdpPort() const {return udpPort;}

std::string UdpPkt::IpAddr() const {return ipAddr;}
//-------------------------------------------------------------------------------------------
Udp::Udp(int port):udpPort(port)
{
    if((_sockId = socket(AF_INET, SOCK_DGRAM, 0))<0) {
        ziblog(LOG::ERR, "Socket Error (%s)!", strerror(errno));
        return;
    }
    _addr.sin_family = AF_INET;
    _addr.sin_port = htons(port);
    _addr.sin_addr.s_addr = INADDR_ANY;
    if(bind(_sockId, (sockaddr*)&_addr, sizeof(_addr))<0) ziblog(LOG::ERR, "Socket Bind Error (%s)!", strerror(errno));
    exit = false;
    std::stringstream sap;
    sap<<_sockId;
    _sap = sap.str();
    reader._sockId=_sockId;
    reader._addr=_addr;
    reader.exit=false;
    register2Label(getTxDataLabel());//autoregistrazione sugli eventi di richiesta di trasmissione inviatigli dagli utilizzatori
}

Udp::~Udp() {close(_sockId);}

int Udp::UdpPort() const {return udpPort;}

void Udp::run()
{
    while(!exit){
        Event* Ev = pullOut();
        if(Ev){
            ziblog(LOG::INF, "received event with label %s", Ev->label().c_str());
            if(dynamic_cast<StopThreadEvent *>(Ev)) exit = true;
            else if(dynamic_cast<UdpPkt *>(Ev)) {
                UdpPkt* txDataEvent = (UdpPkt*)Ev;
                std::string destIpAddr=txDataEvent->IpAddr();
                int destUdpPort=txDataEvent->UdpPort();
                sockaddr_in destSockAddr;
                destSockAddr.sin_family = AF_INET;
                destSockAddr.sin_port = htons(destUdpPort);
                if(destIpAddr=="localhost") destIpAddr="127.0.0.1";
                if(!inet_pton(AF_INET, destIpAddr.c_str(), &destSockAddr.sin_addr)) ziblog(LOG::ERR, "inet_pton (%s) has failed!", destIpAddr.c_str());
                if(sendto(_sockId, txDataEvent->buf(), txDataEvent->len(), 0, (sockaddr*)&destSockAddr, sizeof(destSockAddr)) < 0) ziblog(LOG::ERR, "send to failed (%s)", strerror(errno));
            } else ziblog(LOG::ERR, "unexpected event");
            delete Ev;
        }
    }
}

void Udp::Start()
{
    Thread::Start();
    reader.Start();
}

void Udp::Stop()
{
    reader.Stop();
    Thread::Stop();
}

void Udp::Join()
{
    if(alive()) {
        reader.Join();
        Thread::Stop();
    } else reader.Stop();
}

Udp::Reader::Reader()
{
    if((efd=eventfd(0, O_NONBLOCK))==-1) ziblog(LOG::ERR, "efd error (%s)", strerror(errno));
}

Udp::Reader::~Reader() {close(efd);}

void Udp::Reader::run()
{
    std::stringstream sap;
    sap<<_sockId;
    std::string rxDataLabel = "dgramRxDataEvent"+sap.str();//REM!! deve essere = a quella ritornata da Udp::getRxDataLabel()
    const int maxSize = 1024;//1 kb max payload a pacchetto...
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
                socklen_t address_len = sizeof(_addr);
                if((len = recvfrom(_sockId, rxbyte, maxSize, 0, (sockaddr*)&_addr, &address_len)) > 0) {
                    std::string senderIpAddr=inet_ntoa(_addr.sin_addr);
                    int senderUdpPort = ntohs(_addr.sin_port);
                    UdpPkt rx(rxDataLabel, senderUdpPort, senderIpAddr, rxbyte, len);
                    rx.emitEvent();
                }
            } else if(FD_ISSET(efd, &rdfs)) {//evento di stop
                unsigned char exitFlag[8];
                if(read(efd, exitFlag, 8)==-1) ziblog(LOG::ERR, "read from efd error (%s)", strerror(errno));
                if(exitFlag[7]!=1) ziblog(LOG::ERR, "unexpected exitFlag value (%d)", exitFlag[7]);
                exit=true;
            } else ziblog(LOG::ERR, "select lied to me!");
        }
    }
}

void Udp::Reader::Start()
{
    exit=false;
    Thread::Start();
}

void Udp::Reader::Stop()
{
    unsigned char exitFlag[]={0,0,0,0,0,0,0,1};
    if(write(efd, exitFlag, 8)==-1) ziblog(LOG::ERR, "reader stop error (%s)", strerror(errno));
}
//-------------------------------------------------------------------------------------------
}//namespace Z
