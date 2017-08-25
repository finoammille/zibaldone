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
    register2Label(getTxDataLabel());//self-registration to transmission request event emitted by class-users
                                     //autoregistrazione sugli eventi di richiesta di trasmissione inviatigli dagli utilizzatori
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
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,22)//eventFd is not available with this version so we have to do polling
                                               //non c'e` eventFd x cui occorre aspettare il polling...
    reader.Join();
#endif
    Thread::Stop();
}

void Udp::Join()
{
    //N.B.: non e` possibile che reader stia girando ma TcpConnHandler sia terminato.
    reader.Join();
    Thread::Stop();
}

Udp::Reader::Reader()
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,22)
    if((efd=eventfd(0, O_NONBLOCK))==-1) ziblog(LOG::ERR, "efd error (%s)", strerror(errno));
#endif
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,22)

Udp::Reader::~Reader() {close(efd);}

void Udp::Reader::run()
{
    std::stringstream sap;
    sap<<_sockId;
    std::string rxDataLabel = "dgramRxDataEvent"+sap.str();//REM!! it must = Udp::getRxDataLabel()
                                                           //REM!! deve essere = a quella ritornata da Udp::getRxDataLabel()
    const int maxSize = 1024;//we choose to set 1 kb max each datagram ... if not enough u can modify this....
                             //1 kb max payload a pacchetto... se non basta, modificare qui!
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
                                          //dati presenti sul socket
                int len = 0;
                socklen_t address_len = sizeof(_addr);
                if((len = recvfrom(_sockId, rxbyte, maxSize, 0, (sockaddr*)&_addr, &address_len)) > 0) {
                    std::string senderIpAddr=inet_ntoa(_addr.sin_addr);
                    int senderUdpPort = ntohs(_addr.sin_port);
                    UdpPkt rx(rxDataLabel, senderUdpPort, senderIpAddr, rxbyte, len);
                    rx.emitEvent();
                }
            } else if(FD_ISSET(efd, &rdfs)) {//stop event
                                             //evento di stop
                unsigned char exitFlag[8];
                if(read(efd, exitFlag, 8)==-1) ziblog(LOG::ERR, "read from efd error (%s)", strerror(errno));
                if(exitFlag[7]!=1) ziblog(LOG::ERR, "unexpected exitFlag value (%d)", exitFlag[7]);
                exit=true;
            } else ziblog(LOG::ERR, "select lied to me!");
        }
    }
}

#else//KERNEL OLDER THAN 2.6.22

void Udp::Reader::run()
{
    std::stringstream sap;
    sap<<_sockId;
    std::string rxDataLabel = "dgramRxDataEvent"+sap.str();//REM!! it must = Udp::getRxDataLabel()
                                                           //REM!! deve essere = a quella ritornata da Udp::getRxDataLabel()
    const int maxSize = 1024;//we choose to set 1 kb max each datagram ... if not enough u can modify this....
                             //1 kb max payload a pacchetto... se non basta, modificare qui!
    unsigned char rxbyte[maxSize];
    fd_set rdfs;//file descriptor set
	int nfds=_sockId+1;
	timeval tv;
	while(!exit) {
        FD_ZERO(&rdfs);
        FD_SET(_sockId, &rdfs);
		tv.tv_sec = 1;//1 sec polling to catch any Stop()
                      //1 sec di polling x eventuale Stop()
		tv.tv_usec = 0;
        int ret = select(nfds, &rdfs, NULL, NULL, &tv);
		if(ret) {
			if (ret<0) ziblog(LOG::ERR, "select error (%s)", strerror(errno));
			else {
				if(FD_ISSET(_sockId, &rdfs)) {//available data on socket
                                              //dati presenti sul socket
					int len = 0;
					socklen_t address_len = sizeof(_addr);
					if((len = recvfrom(_sockId, rxbyte, maxSize, 0, (sockaddr*)&_addr, &address_len)) > 0) {
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

#endif

void Udp::Reader::Start()
{
    exit=false;
    Thread::Start();
}

void Udp::Reader::Stop()
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,22)
    unsigned char exitFlag[]={0,0,0,0,0,0,0,1};
    if(write(efd, exitFlag, 8)==-1) ziblog(LOG::ERR, "reader stop error (%s)", strerror(errno));
#else
	exit=true;
#endif
}
//-------------------------------------------------------------------------------------------
}//namespace Z
