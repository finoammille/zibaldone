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
#if !defined(_MSC_VER)
#include <unistd.h>
#endif

using namespace Z;//namespace for zibaldone library

class backgroundWorker : public AutonomousThread {
    void singleLoopCycle()
    {
        std::cout<<"HELLO!!! I'm an autonomous thread!!!\n";
        std::cout<<"I'm emitting an InfoMsg event with label AutEv!...\n";
        InfoMsg tx("AutEv", "Hello Boss!");
        tx.emitEvent();

#ifdef linux
        usleep(1000000);//1 sec
#else //winzozz
    Sleep(1000);//1 sec
#endif
    }
};

class Boss : public Thread {
    bool exit;
    void help()
    {
        std::cout<<"\n*** Autonomous Thread Test ***\n";
        std::cout<<"type \"go\" to start autonomous thread\n";
        std::cout<<"type \"stop\" to stop autonomous thread\n";
        std::cout<<"type \"exit\" to finish test and quit\n";
        std::cout<<"type \"help\" to view this!\n";
    }
    void run()//thread loop function
    {
        std::string cmd;
	    char txBuf[256];
        backgroundWorker AT;//autonomous Thread
        help();
        while(!exit){
            Event* Ev = pullOut(100);//check thread queue for new events. We don't block on queue but we wait at least 1 sec, then we poll for user input
            if(Ev){
                //if here, there was an event!
                ziblog(LOG::INF, "received event with label %s", Ev->label().c_str());
                if(dynamic_cast<StopThreadEvent *>(Ev)) exit = true;
                else if(dynamic_cast<InfoMsg *>(Ev)) {
                    InfoMsg* ev = (InfoMsg*)Ev;
                    std::cout<<"\n\n\nRECEIVED EVENT "<<typeid(*ev).name()<<"\n";
                    std::cout<<"payload event msg ="<<ev->msg()<<"\n";
                } else ziblog(LOG::ERR, "unexpected event!");
                delete Ev;//we have extracted the event from queue and now it's our resposibility to delete it after using it
            }
            if(std::cin.good()){//check for user input
                std::cin.getline(txBuf, 256);//get input
                cmd = txBuf;
                if(cmd == "exit") {
                    AT.Stop();//a prescindere...!
                    exit=true;
                }
                else if(cmd == "help") help();
                else if(cmd == "go" || cmd=="1") AT.Start();
                else if(cmd == "stop" || cmd=="0") AT.Stop();
                else std::cout<<"unknown command.\n";
                cmd.clear();
            }
        }
    }
public:
    Boss()
    {
        exit=false;
        register2Label("AutEv");
    }
};

void testAutonomousThread()
{
    Boss B;
    B.Start();
    B.Join();
}
