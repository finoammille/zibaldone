/*
 *
 * zibaldone - a C++ library for Thread, Timers and other Stuff
 *
 * Copyright (C) 2012  Antonio Buccino
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
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

namespace Z 
{
//-------------------------------------------------------------------------------------------
static void exceptionHandler(SerialPortException& spEx, std::string portName)//metodo statico (scope locale) per evitare codice ripetuto
{
    unsigned int ErrorMessageLen = spEx.ErrorMessage().size();
    unsigned char* buffer = new unsigned char[ErrorMessageLen];
    for(size_t i=0; i<ErrorMessageLen; i++) buffer[i]=(unsigned char)spEx.ErrorMessage()[i];
    Event serialPortErrorEvent(SerialPortHandler::getSerialPortErrorEventId(portName), buffer, ErrorMessageLen);
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
     * semplicemente inviare un evento  (Event) con eventId=txDataEventId
     */
    register2Event(getTxDataEventId());
}

void SerialPortHandler::run()
{
    while(!exit){
        try {
            Event* Ev = pullOut();
            if(Ev){
                std::string eventId = Ev->eventId();
                ziblog(LOG::INF, "received event %s", eventId.c_str());
                if(eventId == StopThreadEventId) exit = true; 
                else if(eventId == getTxDataEventId()){
                    sp.Write(Ev->buf(), Ev->len());
                    delete Ev;
                } 
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

SerialPortHandler::Reader::Reader(SerialPort& sp):sp(sp){}

void SerialPortHandler::Reader::run()
{
    fd_set rdfs;//file descriptor set
    std::vector<unsigned char> rxData;
    FD_ZERO(&rdfs);
    FD_SET(sp.fd, &rdfs);
    for(;;) {
        try {
            if(select(sp.fd+1, &rdfs, NULL, NULL, NULL)) rxData = sp.Read();
            if(!rxData.empty()){
                unsigned char* buffer = new unsigned char[rxData.size()];
                for(size_t i=0; i<rxData.size(); i++)buffer[i]=rxData[i];
                Event rx(SerialPortHandler::getRxDataEventId(sp.portName), buffer, rxData.size());
                rx.emitEvent();
                delete buffer;
            } else ziblog(LOG::ERR, "select lied to me!");
        } catch(SerialPortException spEx){
            exceptionHandler(spEx, sp.portName);
            return;//devo comunque uscire per evitare di continuare a emettere lo stesso
                   //errore ripetutamente (la seriale non funziona! E' scollegata o e' rotta!)
        }
    }
}

void SerialPortHandler::Reader::Stop(){kill();}
//-------------------------------------------------------------------------------------------
}//namespace Z
