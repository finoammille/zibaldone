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

#include <sstream>
#include "SerialPortHandler.h"
#include "Events.h"

using namespace Z;
class SerialPortTest : public Thread {
private:
    const std::string portName;
    bool exit;
    void run()
    {
        std::string cmd;
	    char txBuf[256];
        std::cout<<"SerialPortTest>>\ntype ""exit"" to finish.\n";
        bool noError = true;
        while(noError && !exit){
            Event* Ev = pullOut(1000);//max 1 secondo di attesa
            if(Ev){
                ziblog(LOG::INF, "received event %s", Ev->label().c_str());
                if(dynamic_cast<StopThreadEvent *>(Ev)) exit = true;
                else if(dynamic_cast<RawByteBufferData *>(Ev)) {
                    RawByteBufferData* ev = (RawByteBufferData*)Ev;
                    for(int i=0; i<ev->len(); i++)std::cout<<"Received: "<<(ev->buf())[i]<<" from serial port "<<portName<<std::endl;
                } else if(dynamic_cast<zibErr *>(Ev)) {
                    zibErr* ev = (zibErr*)Ev;
                    ziblog(LOG::INF, "%s", ev->errorMsg().c_str());
                    noError = false;
                }
                delete Ev;
            }
            if(std::cin.good()){
                std::cin.getline(txBuf, 256);//prendo il comando da console input
                cmd = txBuf;
                if(cmd == "exit") return;
                else if (cmd.length()) {
                    RawByteBufferData tx(SerialPortHandler::getTxDataLabel(portName), (unsigned char*)txBuf, cmd.length());
                    tx.emitEvent();
                }
                cmd.clear();
            }
        }
    }
public:
    SerialPortTest(std::string portName):portName(portName)
    {
        exit = false;
        register2Label(SerialPortHandler::getRxDataLabel(portName));
        register2Label(SerialPortHandler::getSerialPortErrorLabel(portName));
    }
};

//------------------------------------------------------------------------------
//esempio: zibTest --testSerialPort /dev/ttyS0 115200 8 none 1 xonxoff
void testSerialPort(std::string port,
                    std::string baudRate,
                    std::string dataBits,
                    std::string parity,//none, even, odd, space, mark
                    std::string stopBits,
                    std::string flwCtrl)//none, hardware, xonxoff
{
    int BaudRate;
    int DataBits;
    int StopBits;
    SerialPort::Parity Parity;
    SerialPort::FlowControl FlwCtrl;

    std::istringstream ssBaudRate(baudRate);
    ssBaudRate >> BaudRate;
    std::istringstream ssDataBits(dataBits);
    ssDataBits >> DataBits;
    std::istringstream ssStopBits(stopBits);
    ssStopBits >> StopBits;

    if(parity=="none") Parity=SerialPort::ParityNone;
    else if(parity=="even") Parity=SerialPort::ParityEven;
    else if(parity=="odd") Parity=SerialPort::ParityOdd;
    else if(parity=="space") Parity=SerialPort::ParitySpace;
    else if(parity=="mark") Parity=SerialPort::ParityMark;
    else std::cerr<<"invalid parity parameter ("<<parity<<")\n";

    if(flwCtrl=="none") FlwCtrl=SerialPort::FlowControlNone;
    else if(flwCtrl=="hardware") FlwCtrl=SerialPort::FlowControlHardware;
    else if(flwCtrl=="xonxoff") FlwCtrl=SerialPort::FlowControlXonXoff;
    else std::cerr<<"invalid parity parameter ("<<flwCtrl<<")\n";

    std::cout<<"opening port:"<<port<<" - "<<"baudRate="<<BaudRate<<" - "
             <<"DataBits="<<DataBits<<" - "<<"Parity="<<(int)Parity<<" - "
             <<"StopBits="<<StopBits<<" - "<<"FlwCtrl="<<(int)FlwCtrl<<"\n";

    SerialPortHandler sp(port, BaudRate, Parity, DataBits, StopBits, FlwCtrl);
    SerialPortTest test(port);
    test.Start();
    sp.Start();
    test.Join();//join con user thr
    sp.Stop();
    test.Stop();
}
