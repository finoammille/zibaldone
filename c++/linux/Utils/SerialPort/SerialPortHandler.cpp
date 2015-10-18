/*
 *
 * zibaldone - a C++ library for Thread, Timers and other Stuff
 * http://sourceforge.net/projects/zibaldone/
 *
 * Copyright (C) 2012  ilant (ilant@users.sourceforge.net)
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

#include "SerialPortHandler.h"
#include "Events.h"

namespace Z
{
//-------------------------------------------------------------------------------------------
static void exceptionHandler(SerialPortException& spEx, std::string portName)
{
    zibErr serialPortErrorEvent(SerialPortHandler::getSerialPortErrorLabel(portName), spEx.ErrorMessage());
    serialPortErrorEvent.emitEvent();
}
//-------------------------------------------------------------------------------------------
SerialPortHandler::SerialPortHandler(const std::string& portName,
                                     int baudRate,
                                     SerialPort::Parity parity,
                                     int dataBits,
                                     int stopBits,
                                     SerialPort::FlowControl flwctrl,
                                     bool toggleDtr,
                                     bool toggleRts):
sp(portName, baudRate, parity, dataBits, stopBits, flwctrl, toggleDtr,
toggleRts), reader(sp)
{
    exit = false;
    register2Label(getTxDataLabel());
}

void SerialPortHandler::run()
{
    while(!exit){
        try {
            Event* Ev = pullOut();
            if(Ev){
                ziblog(LOG::INF, "received event with label %s", Ev->label().c_str());
                if(dynamic_cast<StopThreadEvent *>(Ev)) exit = true;
                else if(dynamic_cast<RawByteBufferData *>(Ev)) {
                    RawByteBufferData* txDataEvent = (RawByteBufferData*)Ev;
                    sp.Write(txDataEvent->buf(), txDataEvent->len());
                } else ziblog(LOG::ERR, "unexpected event");
                delete Ev;
            }
        } catch(SerialPortException spEx){
            exceptionHandler(spEx, sp.portName);
            exit = true;
        }
    }
}

void SerialPortHandler::Start()
{
    Thread::Start();
    reader.Start();
}

void SerialPortHandler::Stop()
{
    Thread::Stop();
    reader.Stop();
}

void SerialPortHandler::Join()
{
    if(alive()) {
        reader.Join();
        Thread::Stop();
    } else reader.Stop();
}

SerialPortHandler::Reader::Reader(SerialPort& sp):exit(false), sp(sp)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,22)
    if((efd=eventfd(0, O_NONBLOCK))==-1) ziblog(LOG::ERR, "efd error");
#endif
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,22)

void SerialPortHandler::Reader::run()
{
    fd_set rdfs;//file descriptor set
    std::vector<unsigned char> rxData;
    int nfds=(efd>sp.fd ? efd : sp.fd)+1;
    while(!exit) {
        try {
            FD_ZERO(&rdfs);
            FD_SET(sp.fd, &rdfs);
            FD_SET(efd, &rdfs);
            if(select(nfds, &rdfs, NULL, NULL, NULL)==-1) ziblog(LOG::ERR, "select error");
            else {
                if(FD_ISSET(sp.fd, &rdfs)) {//data available on serial port
                    rxData = sp.Read();
                    if(!rxData.empty()){
                        unsigned char* buffer = new unsigned char[rxData.size()];
                        for(size_t i=0; i<rxData.size(); i++)buffer[i]=rxData[i];
                        RawByteBufferData rx(SerialPortHandler::getRxDataLabel(sp.portName), buffer, rxData.size());
                        rx.emitEvent();
                        delete buffer;
                    }
                } else if(FD_ISSET(efd, &rdfs)) {//stop event
                    unsigned char exitFlag[8];
                    if(read(efd, exitFlag, 8)==-1) ziblog(LOG::ERR, "read from efd error (%s)", strerror(errno));
                    if(exitFlag[7]!=1) ziblog(LOG::ERR, "unexpected exitFlag value (%d)", exitFlag[7]);
                    exit=true;
                } else ziblog(LOG::ERR, "select lied to me!");
            }
        } catch(SerialPortException spEx){
            exceptionHandler(spEx, sp.portName);
            return;
        }
    }
}

#else//KERNEL OLDER THAN 2.6.22

void SerialPortHandler::Reader::run()
{
    fd_set rdfs;//file descriptor set
    std::vector<unsigned char> rxData;
    int nfds=sp.fd+1;
	timeval tv;
    while(!exit) {
        try {
            FD_ZERO(&rdfs);
            FD_SET(sp.fd, &rdfs);
			tv.tv_sec = 1;//1 sec polling to catch any Stop()
			tv.tv_usec = 0;
			int ret = select(nfds, &rdfs, NULL, NULL, &tv);
			if(ret) {
				if (ret<0) ziblog(LOG::ERR, "select error (%s)", strerror(errno));
				else {
					if(FD_ISSET(sp.fd, &rdfs)) {//available data on serial port
						rxData = sp.Read();
						if(!rxData.empty()){
							unsigned char* buffer = new unsigned char[rxData.size()];
							for(size_t i=0; i<rxData.size(); i++)buffer[i]=rxData[i];
							RawByteBufferData rx(SerialPortHandler::getRxDataLabel(sp.portName), buffer, rxData.size());
							rx.emitEvent();
							delete buffer;
						}
					} /* N.B.: select returns if there are available data on serial port and sets _sockId in rdfs,
				       * or if timeout (1 sec in our case). So. if Stop() is called then exit is set to true and
                       * thread exits gently
                       */
				}
			}

        } catch(SerialPortException spEx){
            exceptionHandler(spEx, sp.portName);
            return;
        }
    }
}

#endif

void SerialPortHandler::Reader::Start()
{
    exit=false;
    Thread::Start();
}

void SerialPortHandler::Reader::Stop()
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
