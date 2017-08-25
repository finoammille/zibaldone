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

#include "Thread.h"
#include "Events.h"
#include "Log.h"
#include <typeinfo>
using namespace Z;//namespace for zibaldone library

class reqEv : public Event {
    std::string msg;
public:
    reqEv(std::string info):Event("reqEv"), msg(info){}
    reqEv* clone()const{return new reqEv(*this);}
    std::string getMsg(){return msg;}
};

class respEv : public Event {
    std::string msg;
public:
    respEv(std::string info):Event("respEv"), msg(info){}
    respEv* clone()const{return new respEv(*this);}
    std::string getMsg(){return msg;}
};

class worker : public Thread {
    bool exit;
    void run()
    {
        while(!exit) {
            Event* Ev = pullOut();
            if(Ev){
                ziblog(LOG::INF, "received event with label %s", Ev->label().c_str());
                if(dynamic_cast<StopThreadEvent *>(Ev)) exit = true;
                else if(dynamic_cast<reqEv *>(Ev)) {
                    reqEv* ev = (reqEv*)Ev;
                    std::cout<<"...WORKER received info message:"<<ev->getMsg()<<"\n";
                    std::string resp="*** ok! *** received the request "+ev->getMsg();
                    (respEv(resp)).emitEvent();
                } else ziblog(LOG::ERR, "### unexpected event! ###");
                delete Ev;
            }
        }
    }
public:
    worker():exit(false){register2Label("reqEv");}
};

class delegator : public SameThreadAddressSpaceEventReceiver {
public:
    delegator(){register2Label("respEv");}
    void request(int msecWait)
    {
        std::cout<<"...purge rx queue....\n";
        Event* discarded;
        int count=0;
        while ((discarded=pullOut(0))!=NULL) {//instant peek and discard
            std::cout<<"### discarded event "<<discarded->label()<<" ###\n";
            delete discarded;
            discarded=NULL;
            count++;
        }
        if(count==0) std::cout<<"*** nothing discarded!!! ***\n";
        std::cout<<"... processing request("<<msecWait<<")\n";
        std::stringstream tmp;
        tmp<<msecWait;
        std::string infoMsg="request"+tmp.str();
        (reqEv(infoMsg)).emitEvent();
        Event* Ev=pullOut(msecWait);
        if(Ev){
            ziblog(LOG::INF, "received event with label %s", Ev->label().c_str());
            if(dynamic_cast<respEv *>(Ev)) {
                respEv* ev = (respEv*)Ev;
                std::cout<<"*** received RESPONSE message: *** "<<ev->getMsg()<<" *** for request("<<msecWait<<")\n";
            } else ziblog(LOG::ERR, "unexpected event!");
            delete Ev;
        } else std::cout<<"### timeout elapsed!! for request("<<msecWait<<") ###\n";
    }
    void requestWithoutResponse()
    {
        std::cout<<"... purge rx queue....\n";
        Event* discarded;
        int count=0;
        while ((discarded=pullOut(0))!=NULL) {
            std::cout<<"### discarded event "<<discarded->label()<<" ###\n";
            delete discarded;
            discarded=NULL;
            count++;
        }
        if(count==0) std::cout<<"*** nothing discarded!!! ***\n";
        std::cout<<"... processing requestWithoutResponse\n";
        (InfoMsg("ciao", "ciao")).emitEvent();
        Event* Ev=pullOut(1000);
        if(Ev){
            ziblog(LOG::INF, "received event with label %s", Ev->label().c_str());
            if(dynamic_cast<respEv *>(Ev)) {
                respEv* ev = (respEv*)Ev;
                std::cout<<"*** received RESPONSE message: ***"<<ev->getMsg()<<" *** and label="<<Ev->label()<<"\n";
            } else ziblog(LOG::ERR, "unexpected event!");
            delete Ev;
        } else std::cout<<"\n### timeout elapsed!! for requestWithoutResponse ###\n";
    }
};

void testSameThreadAddressSpaceEventReceiver()
{
    std::string cmd;
	char tmpBuf[1024];
    std::cout<<"r(msec) => call d.request(msec);\n";
    std::cout<<"reqWithoutResp => call d.requestWithoutResponse();\n";
    std::cout<<"e => exit\n\n";
    worker w;
    w.Start();
    delegator d;
    while(true) {
        if(std::cin.good()){
            std::cin.getline(tmpBuf, 1024);//prendo il comando da console input
            cmd = tmpBuf;
            if(!cmd.empty()) {
                if(cmd == "e") {
                    w.Stop();
                    return;
                } else if(cmd[0] == 'r') {
                    std::string command = cmd;
                    size_t i1 = command.find("(",0);
                    size_t i2 = command.find(")",0);
                    if(i1 != std::string::npos && i2 != std::string::npos) {
                        i1++;
                        int millisec;
                        std::istringstream ssMillisec(command.substr(i1, i2-i1));
                        ssMillisec >> millisec;
                        d.request(millisec);
                    }
                } else if(cmd == "reqWithoutResp") d.requestWithoutResponse();
                else {
                    std::cout<<"invalid choice!\n\n";
                    std::cout<<"r(msec) => call d.request(msec);\n";
                    std::cout<<"reqWithoutResp => call d.requestWithoutResponse();\n";
                    std::cout<<"e => exit\n\n";
                }
                cmd.clear();
            }
            std::cout<<">> ";
        }
    }
}
