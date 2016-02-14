/*
 *
 * zibaldone - a C++/Java library for Thread, Timers and other Stuff
 *
 * http://sourceforge.net/projects/zibaldone/
 *
 * version 3.2.0, February 14th, 2016
 *
 * Copyright (C) 2012  ilant (ilant@users.sourceforge.net)
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
 * ilant ilant@users.sourceforge.net
 *
 */

#include "UdpIpc.h"

namespace Z
{
//------------------------------------------------------------------------------
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
//------------------------------------------------------------------------------
Udp::Udp(int port):udpPort(port)
{
    _sockId = INVALID_SOCKET;
    WSADATA wsaData;
    //Initialize Winsock
    int ret = WSAStartup(MAKEWORD(2,2), &wsaData);
    if(ret != 0) {
        ziblog(LOG::ERR, "WSAStartup failed: %d", ret);
        return;
    }
    _sockId = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (_sockId == INVALID_SOCKET) {
        ziblog(LOG::ERR, "Error at socket(): %d", WSAGetLastError());
        WSACleanup();
        return;
    }
    _addr.sin_family = AF_INET;
    _addr.sin_addr.s_addr = INADDR_ANY;
    _addr.sin_port = htons(port);
    if(bind(_sockId ,(struct sockaddr *)&_addr , sizeof(_addr)) == SOCKET_ERROR) {
        ziblog(LOG::ERR, "Error at socket(): %d", WSAGetLastError());
        WSACleanup();
        return;
    }
    exit=false;
    std::stringstream sap;
    sap<<_sockId;
    _sap = sap.str();
    reader._sockId=_sockId;
    reader._addr=_addr;
    reader.exit=false;
    register2Label(getTxDataLabel());//self-registration to transmission request event emitted by class-users
                                     //autoregistrazione sugli eventi di richiesta di trasmissione inviatigli dagli utilizzatori
}

Udp::~Udp()
{
    shutdown(_sockId, SD_BOTH);
    closesocket(_sockId);
    WSACleanup();
}

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
                destSockAddr.sin_addr.s_addr = inet_addr(destIpAddr.c_str());
                if(sendto(_sockId, (char*)(txDataEvent->buf()), txDataEvent->len(), 0, (sockaddr*)&destSockAddr, sizeof(destSockAddr)) < 0) ziblog(LOG::ERR, "send to failed (%d)", WSAGetLastError());
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
	reader.Join();
	Thread::Stop();
}

void Udp::Join()
{
    //N.B.: non e` possibile che reader stia girando ma TcpConnHandler sia terminato.
    reader.Join();
    Thread::Stop();
}

void Udp::Reader::run()
{
    std::stringstream sap;
    sap<<_sockId;
    std::string rxDataLabel = "dgramRxDataEvent"+sap.str();//REM!! deve essere = a quella ritornata da Udp::getRxDataLabel()
                                                           //REM!! deve essere = a quella ritornata da Udp::getRxDataLabel()
    const int maxSize = 1024;//we choose to set 1 kb max each datagram ... if not enough u can modify this....
                             //1 kb max payload a pacchetto... se non basta, modificare qui!
    unsigned char rxbyte[maxSize];
    fd_set rdfs;//file descriptor set
	TIMEVAL tv={0};
	int ret=0;
    while(!exit) {
		FD_ZERO(&rdfs);
		FD_SET(_sockId, &rdfs);
		tv.tv_sec = 1;//1 sec
		tv.tv_usec = 0;
		ret=select(0, &rdfs, NULL, NULL, &tv);
		if(ret) {
			if(ret==SOCKET_ERROR) ziblog(LOG::ERR, "select error (%d)", WSAGetLastError());
			else {
				if(FD_ISSET(_sockId, &rdfs)) {//available data on socket
                                              //dati presenti sul socket
					int len = 0;
					socklen_t address_len = sizeof(_addr);
					if((len = recvfrom(_sockId, (char*)rxbyte, maxSize, 0, (sockaddr*)&_addr, &address_len)) > 0) {
						std::string senderIpAddr=inet_ntoa(_addr.sin_addr);
						int senderUdpPort = ntohs(_addr.sin_port);
						UdpPkt rx(rxDataLabel, senderUdpPort, senderIpAddr, rxbyte, len);
						rx.emitEvent();
					}
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

void Udp::Reader::Start()
{
    exit=false;
    Thread::Start();
}

void Udp::Reader::Stop() {exit=true;}
//------------------------------------------------------------------------------
}//namespace Z
