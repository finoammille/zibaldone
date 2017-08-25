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

#include "Timer.h"
#include "Events.h"
#include <vector>
#include <algorithm>

using namespace Z;

class testTimerThread : public Thread {
public:
    void help()
    {
        std::cout<<"Timers Test!.... press Q to exit\n";
        std::cout<<"Available commands:\n";
        std::cout<<"n(timerId, duration_msec) => start new timer\n";
        std::cout<<"s(timerId) => stop timer\n";
        std::cout<<"l => list timer\n";
        std::cout<<"h => help: print this screen...\n";
    }

private:
    std::vector<Timer*> TimerList;
    void removeTimerFromList(std::string timerId)
    {
        std::vector<Timer *>::iterator it;
        for(it = TimerList.begin(); it != TimerList.end(); it++)
            if((*it)->getTimerId() == timerId){
                delete *it;
                TimerList.erase(it);
                break;
            }
    }
    void run()
    {
        std::string cmd;
        char buffer[256];
        help();
        for(;;) {
            Event* Ev = pullOut(1000);
            if(Ev) {
                ziblog(LOG::INF, "received event %s", Ev->label().c_str());
                if(dynamic_cast<StopThreadEvent *>(Ev)) {
                    delete Ev;
                    return;
                } else if(dynamic_cast<TimeoutEvent *>(Ev)) {
                    TimeoutEvent* ev = (TimeoutEvent*)Ev;
                    std::string timerId = ev->timerId();
                    delete Ev;
                    std::cout<<"timer "<<timerId<<" expired!\n";
                    removeTimerFromList(timerId);
                }
            }
            if(std::cin.good()) {
                std::cin.getline(buffer, 256);//prendo il comando da console input
                cmd = buffer;
                if(cmd == "q" || cmd == "Q") return;
                else if(cmd == "h" || cmd == "H") help();
                else if(cmd == "l" || cmd == "L") {
                    std::cout<<"timer list:\n";
                    for (size_t i=0; i<TimerList.size(); i++) std::cout<<TimerList[i]->getTimerId()<<std::endl;
                    std::cout<<std::endl<<std::flush;
                } else if(cmd[0] == 'n' || cmd[0] == 'N' || cmd[0] == 's' || cmd[0] == 'S') {
                    std::string command = cmd;
                    size_t i1 = command.find("(",0);
                    size_t i2 = command.find(",");
                    size_t i3 = command.find(")",0);
                    if(i1 != std::string::npos && i3 != std::string::npos) {
                        i1++;
                        if(cmd[0] == 's' || cmd[0] == 'S') {
                            std::string timerId;
                            std::istringstream ssTimerName(command.substr(i1, i3-i1));
                            ssTimerName >> timerId;
                            for(size_t i = 0; i != TimerList.size(); i++)
                                if(TimerList[i]->getTimerId() == timerId){
                                    TimerList[i]->Stop();
                                    removeTimerFromList(timerId);
                                    std::cout<<"stopped Timer "<<timerId<<std::endl<<std::flush;
                                    break;
                                }
                        } else {
                            int msecDuration;
                            std::string timerId;
                            std::istringstream ssTimerName(command.substr(i1, i2-i1));
                            ssTimerName >> timerId;
                            if(i2 == std::string::npos) {
                                std::cout<<"need duration to start timer!\n"<<std::flush;
                                continue;
                            }
                            i2++;
                            std::istringstream ssMsecDuration(command.substr(i2, i3-i2));
                            ssMsecDuration >> msecDuration;
                            Timer *t = NULL;
                            for(size_t i = 0; i != TimerList.size(); i++)
                                if(TimerList[i]->getTimerId() == timerId){
                                    t = TimerList[i];
                                    t->Stop();
                                    std::cout<<"Timer "<<timerId<<" already present has been restarted with new value..."<<std::endl<<std::flush;
                                    break;
                                }
                            if(!t) {
                                t = new Timer(timerId);
                                TimerList.push_back(t);
                            }
                            register2Label(t->getTimerId());
                            t->Start(msecDuration);
                            std::cout<<"started Timer "<<timerId<<std::endl<<std::flush;
                        }
                    }
                }
                cmd.clear();
            }
        }
    }
};

void testTimers()
{
    testTimerThread tTt;
    tTt.Start();
    tTt.Join();
    tTt.Stop();
}
