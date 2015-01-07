/*
 *
 * zibaldone - a C++/Java library for Thread, Timers and other Stuff
 *
 * Copyright (C) 2012  Antonio Buccino
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
static void exceptionHandler(SerialPortException& spEx, std::string portName)//metodo statico (scope locale) per evitare codice ripetuto
{
    zibErr serialPortErrorEvent(SerialPortHandler::getSerialPortErrorLabel(portName), spEx.ErrorMessage());
    serialPortErrorEvent.emitEvent();//l'evento viene emesso in modo che l'utilizzatore della seriale sappia che questa porta non funziona. 
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
    /*
     * autoregistrazione sull'evento di richiesta di scrittura sulla seriale.
     * Chi vuol inviare dati sulla porta seriale "_portName" deve
     * semplicemente inviare un evento  (Event) con label=txDataLabel
     */
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
            exit = true;//devo comunque uscire per evitare di continuare a emettere lo stesso 
                        //errore ripetutamente (la seriale non funziona! E' scollegata o e' rotta!)
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
    if((efd=eventfd(0, O_NONBLOCK))==-1) ziblog(LOG::ERR, "efd error");
}

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
                if(FD_ISSET(sp.fd, &rdfs)) {//dati presenti sulla seriale
                    rxData = sp.Read();
                    if(!rxData.empty()){
                        unsigned char* buffer = new unsigned char[rxData.size()];
                        for(size_t i=0; i<rxData.size(); i++)buffer[i]=rxData[i];
                        RawByteBufferData rx(SerialPortHandler::getRxDataLabel(sp.portName), buffer, rxData.size());
                        rx.emitEvent();
                        delete buffer;
                    }
                } else if(FD_ISSET(efd, &rdfs)) {//evento di stop
                    int exitFlag=0;
                    if(read(efd, &exitFlag, sizeof(int))==-1) ziblog(LOG::ERR, "read from efd error");
                    if(exitFlag!=1) ziblog(LOG::ERR, "unexpected exitFlag value (%d)", exitFlag);
                    exit=true;
                } else ziblog(LOG::ERR, "select lied to me!");
            }
        } catch(SerialPortException spEx){
            exceptionHandler(spEx, sp.portName);
            return;//devo comunque uscire per evitare di continuare a emettere lo stesso
                   //errore ripetutamente (la seriale non funziona! E' scollegata o e' rotta!)
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
    int exitFlag=1;
    if(write(efd, &exitFlag, sizeof(int))==-1) ziblog(LOG::ERR, "reader stop error");
}
//-------------------------------------------------------------------------------------------
}//namespace Z
