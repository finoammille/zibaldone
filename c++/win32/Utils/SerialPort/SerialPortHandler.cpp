/*
 *
 * zibaldone - a C++/Java library for Thread, Timers and other Stuff
 *
 * http://sourceforge.net/projects/zibaldone/
 *
 * version 3.1.2, August 29th, 2015
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

SerialPortHandler::Reader::Reader(SerialPort& sp):exit(false), sp(sp){}

void SerialPortHandler::Reader::run()
{
    std::vector<unsigned char> rxData;
    DWORD eMask;
    while(!exit) {
        OVERLAPPED ov;
        memset(&ov, 0, sizeof(OVERLAPPED));
        ov.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        if(!WaitCommEvent(sp.fd, &eMask, &ov)) {
            DWORD err=GetLastError();
            if(err == ERROR_IO_PENDING) {
                DWORD ret=WaitForSingleObject(ov.hEvent, INFINITE);
                if(ret==WAIT_OBJECT_0) {
                    DWORD dummy;
                    if(!GetOverlappedResult(sp.fd, &ov, &dummy, TRUE)) {
                        ziblog(LOG::ERR, "GetOverlappedResult error (%ld)", GetLastError());
                        throw SerialPortException("READ ERROR");
                    }
                } else ziblog(LOG::ERR, "WaitForSingleObject exit code = %ld",ret);
            } else if(err == ERROR_OPERATION_ABORTED) {
                ziblog(LOG::WRN, "Has Serial port been closed?");
                CloseHandle(ov.hEvent);
                return;
            } else {
                ziblog(LOG::ERR, "WaitCommEvent error (%ld)", GetLastError());
                throw SerialPortException("READ ERROR");
            }
        }
        CloseHandle(ov.hEvent);
        if(eMask==0) return;
        if(eMask & EV_ERR) ziblog(LOG::ERR, "A line-status error occurred");
        try {
            rxData=sp.Read();
            if(!rxData.empty()){
                unsigned char* buffer = new unsigned char[rxData.size()];
                for(size_t i=0; i<rxData.size(); i++)buffer[i]=rxData[i];
				RawByteBufferData rx(SerialPortHandler::getRxDataLabel(sp.portName), buffer, rxData.size());
                rx.emitEvent();
                delete buffer;
            } else ziblog(LOG::ERR, "WaitCommEvent lied to me!");
        } catch (SerialPortException spEx){
            exceptionHandler(spEx, sp.portName);
            return;
        }
    }
}

void SerialPortHandler::Reader::Start()
{
	exit=false;
	Thread::Start();
}

void SerialPortHandler::Reader::Stop()
{
	exit=true;
    SetCommMask(sp.fd, 0x0000);
}
//-------------------------------------------------------------------------------------------
}//namespace Z
