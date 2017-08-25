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

#if defined(_MSC_VER)
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "zibaldone.lib")
#endif

#include "Log.h"
using namespace Z;

#ifdef linux
void testPfLocalIpc();				            //test socket PfLocal (unix only)
#endif
void testSerialPort(std::string,                //test serial port
                    std::string,
                    std::string,
                    std::string,
                    std::string,
                    std::string);
void testTcpIpc();	                            //test TCP IPC
void testUdpIpc();	                            //test UDP IPC
void testAutonomousThread();                    //test AutonomousThread
void testThreads();					            //test Threads
void testOneLabel4MoreEvents();		            //test OneLabel4MoreEvents
void testTimers();					            //test timer
void testTools();					            //test Tools
void testStopAndWaitOverUdp();		            //test StopAndWaitOverUdp
void testSameThreadAddressSpaceEventReceiver(); //test SameThreadAddressSpaceEventReceiver

int main(int argc, char *argv[])
{
    LOG::Level LogLv=LOG::DBG;//default
    bool showLogOnConsole = false;//default
    enum {tcpIpcTst,
          udpIpcTst,
          pfLocalIpcTst,
          ThreadTst,
          AutThreadTst,
          TimerTst,
          OneLabel4MoreEventsTest,
          ToolsTst, SerialPortTst,
          StopAndWaitOverUdpTst,
          SameThreadAddressSpaceEventReceiverTst,
          unspecified} test=unspecified;//available tests
    std::vector<std::string> testParams;
    std::string tmp;
    for(int i=1; i<argc; i++) {
        tmp = argv[i];
        if(tmp == "--testTcp") test=tcpIpcTst;
        else if(tmp == "--testUdp") test=udpIpcTst;
        else if(tmp == "--testPfLocal") test=pfLocalIpcTst;
        else if(tmp == "--testThreads") test=ThreadTst;
        else if(tmp == "--testAutonomousThread") test=AutThreadTst;
        else if(tmp == "--testTimers") test=TimerTst;
        else if(tmp == "--testOneLabel4MoreEvents") test=OneLabel4MoreEventsTest;
        else if(tmp == "--testZibTools") test=ToolsTst;
        else if(tmp == "--testStopAndWaitOverUdp") test=StopAndWaitOverUdpTst;
        else if(tmp == "--testSameThreadAddressSpaceEventReceiver") test=SameThreadAddressSpaceEventReceiverTst;
        else if(tmp == "--testSerialPort") test=SerialPortTst;
        else if(tmp == "--logLevel=debug") LogLv=LOG::DBG;
        else if(tmp == "--logLevel=info") LogLv=LOG::INF;
        else if(tmp == "--logLevel=warning") LogLv=LOG::WRN;
        else if(tmp == "--logLevel=error") LogLv=LOG::ERR;
        else if(tmp == "--showLog=true") showLogOnConsole=true;
        else if(tmp == "--showLog=false") showLogOnConsole=false;
        else testParams.push_back(tmp);
    }
    LOG::set("./", "zibTests", LogLv, showLogOnConsole);
    switch (test) {
        case tcpIpcTst:
            std::cout<<"testTcpIpc()\n";
            testTcpIpc();
            break;
        case udpIpcTst:
            std::cout<<"testUdpIpc()\n";
            testUdpIpc();
            break;
#ifdef linux
        case pfLocalIpcTst:
            std::cout<<"testPfLocalIpc()\n";
            testPfLocalIpc();
            break;
#endif
        case OneLabel4MoreEventsTest:
            std::cout<<"testOneLabel4MoreEvents()\n";
            testOneLabel4MoreEvents();
            break;
        case AutThreadTst:
            std::cout<<"testAutonomousThread()\n";
            testAutonomousThread();
            break;
        case ThreadTst:
            std::cout<<"testThreads()\n";
            testThreads();
            break;
        case TimerTst:
            std::cout<<"testTimers()\n";
            testTimers();
            break;
        case ToolsTst:
            std::cout<<"testTools()\n";
            testTools();
            break;
        case StopAndWaitOverUdpTst:
            std::cout<<"testStopAndWaitOverUdp()\n";
            testStopAndWaitOverUdp();
            break;
        case SameThreadAddressSpaceEventReceiverTst:
            std::cout<<"testSameThreadAddressSpaceEventReceiver()\n";
            testSameThreadAddressSpaceEventReceiver();
            break;
        case SerialPortTst: {
            if(testParams.size()<6) std::cout<<"insufficient parameters.\n";
            else if(testParams.size()>6) std::cout<<"too many parameters....\n";
            else {
                std::cout<<"testSerialPort("<<testParams[0]<<","//         /dev/ttyS0  (port)
                                            <<testParams[1]<<","//         115200      (speed)
                                            <<testParams[2]<<","//         8           (data bits)
                                            <<testParams[3]<<","//         none        (parity - none, even, odd, space, mark - )
                                            <<testParams[4]<<","//         1           (stop bits)
                                            <<testParams[5]<<","//         xonxoff     (flow control - none, hardware, xonxoff - )
                                            <<")\n";
               testSerialPort(testParams[0],
                              testParams[1],
                              testParams[2],
                              testParams[3],
                              testParams[4],
                              testParams[5]);
            }
        } break;

        default:
            if (tmp.length()<2) tmp="???";
            std::cerr<<"test "<<tmp.substr(2)<<" not available...\n";
            break;
    }
    LOG::disable();
    return 0;
}
