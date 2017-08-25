/*
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

#include "StopAndWaitOverUdp.h"
#include "Events.h"

using namespace Z;

class StopAndWaitOverUdpTest : public Thread {
    void help()
    {
        std::cout<<"\nsend <ip> <port> <data>\t=>\tsend datagram with payload=data to ip,port\n";
        std::cout<<"\t\t\t\texample: send 172.19.32.200 8989 hello\n";
        std::cout<<"help\t\t\t=>\tthis screen\n";
        std::cout<<"exit\t\t\t=>\tquit\n";
    }
    const int port;
    StopAndWaitOverUdp* srv;
    bool exit;
public:
    StopAndWaitOverUdpTest(int port):port(port), exit(false) {srv=new StopAndWaitOverUdp(port);}
    ~StopAndWaitOverUdpTest() {delete srv; srv=NULL;}
    void GO()
    {
        register2Label(srv->getRxDataLabel());
        register2Label(sawUdpPktError::sawUdpPktErrorLabel());
        srv->Start();
        Start();//avvio di questo thread
    }
    void run()
    {
        std::string cmd;
	    char txBuf[1024];
        help();
        while(!exit){
            Event* Ev = pullOut(1000);//max 1 secondo di attesa
            if(Ev){
                ziblog(LOG::INF, "received event with label %s", Ev->label().c_str());
                if(dynamic_cast<StopThreadEvent *>(Ev)) exit = true;
                else if(dynamic_cast<UdpPkt *>(Ev)) {
                    UdpPkt* ev = (UdpPkt*)Ev;
                    std::cout<<"received:\n";
                    for(int i=0; i<ev->len(); i++) std::cout<<(ev->buf())[i];
                    std::cout<<"\nfrom "<<ev->IpAddr()<<":"<<ev->UdpPort()<<std::endl;
                } else if (dynamic_cast<sawUdpPktError *>(Ev)) {
                    sawUdpPktError* ev = (sawUdpPktError*)Ev;
                    std::cout<<"received TX ERROR event\n";
                    std::cout<<"\nfor tx to "<<ev->destIpAddr<<":"<<ev->udpPort<<std::endl;
                }
                delete Ev;
            }
            if(std::cin.good()){
                std::cin.getline(txBuf, 1024);//prendo il comando da console input
                cmd = txBuf;
                if(!cmd.empty()) {
                    if(cmd == "exit") {
                        srv->Stop();
                        exit=true;
                    } else {
                        unsigned int prevPos=0;
                        size_t currPos=cmd.find(' ');
                        std::vector<std::string> cmdParams;
                        int paramCount=0;
                        while(currPos!=std::string::npos && paramCount<3) {
                            cmdParams.push_back(cmd.substr(prevPos, currPos-prevPos));
                            paramCount++;
                            prevPos=currPos+1;
                            currPos=cmd.find(' ', prevPos);
                        }
                        cmdParams.push_back(cmd.substr(prevPos));
                        if(cmdParams.size()<4) {
                            std::cout<<"\ninvalid command\n";
                            help();
                        } else {
                            if(cmdParams[0]!="send") std::cout<<"unknown command "<<cmdParams[0]<<std::endl;
                            else {
                                std::string destIpAddr=cmdParams[1];
                                int destUdpPort;
                                std::istringstream ssDestUdpPort(cmdParams[2]);
                                ssDestUdpPort >> destUdpPort;
                                std::cout<<"sending data to "<<destIpAddr<<":"<<destUdpPort<<std::endl;
                                std::vector<unsigned char> data(cmdParams[3].begin(), cmdParams[3].end());
                                UdpPkt tx(srv->getTxDataLabel(), destUdpPort, destIpAddr, data);
                                tx.emitEvent();
                            }
                        }
                    }
                    cmd.clear();
                }
                std::cout<<"\nStopAndWaitOverUdpTest>> ";
            }
        }
    }
};

void testStopAndWaitOverUdp()
{
    int port;
    std::cout<<"insert port number ";
    std::cin>>port;
    StopAndWaitOverUdpTest udp(port);
    udp.GO();
    udp.Join();
}
