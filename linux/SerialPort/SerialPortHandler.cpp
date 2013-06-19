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
SerialPortHandler::SerialPortHandler(const std::string& portName,
                                     int baudRate,
                                     SerialPort::Parity parity,
                                     int dataBits,
                                     int stopBits,
                                     SerialPort::FlowControl flwctrl,                     
                                     bool toggleDtr,
                                     bool toggleRts):
_sp(portName, baudRate, parity, dataBits, stopBits, flwctrl, toggleDtr, toggleRts),
_portName(portName), rxDataEventId("rxDataEvent"+portName), txDataEventId("txDataEvent"+portName),
serialPortErrorEventId("serialPortErrorEvent"+portName)
{
    exit = false;
    /*
     * autoregistrazione sull'evento di richiesta di scrittura sulla seriale.
     * Chi vuol inviare dati sulla porta seriale "_portName" deve
     * semplicemente inviare un evento  (Event) con eventId=txDataEventId
     */
    register2Event(txDataEventId);
}

std::string SerialPortHandler::portName(){return _portName;}

void SerialPortHandler::run()
{
    std::vector<unsigned char> rxData;
    while(!exit){
        try {
            rxData = _sp.Read();
            if(!rxData.empty()){
                unsigned char* buffer = new unsigned char[rxData.size()];
                for(size_t i=0; i<rxData.size(); i++)buffer[i]=rxData[i];
                Event rx(rxDataEventId, buffer, rxData.size());
                rx.emitEvent();
                delete buffer;
            }
            Event* Ev = pullOut(10);//max 1/100 secondo di attesa
            if(Ev){
                std::string eventId = Ev->eventId();
                ziblog(LOG::INFO, "received event %s", eventId.c_str());
                if(eventId == StopThreadEventId) exit = true; 
                else if(eventId == txDataEventId){
                    _sp.Write(Ev->buf(), Ev->len());
                    delete Ev;
                } 
            }
        } catch(SerialPortException spEx){
            unsigned int ErrorMessageLen = spEx.ErrorMessage().size();
            unsigned char* buffer = new unsigned char[ErrorMessageLen];
            for(size_t i=0; i<ErrorMessageLen; i++) buffer[i]=(unsigned char)spEx.ErrorMessage()[i];
            Event serialPortErrorEvent(serialPortErrorEventId, buffer, ErrorMessageLen);
            serialPortErrorEvent.emitEvent();//nota: l'evento viene emesso in modo che l'utilizzatore della seriale sappia che questa non funziona. Poi devo comunque uscire (exit = true) per evitare di continuare a emettere lo stesso errore ripetutamente (la seriale non funziona! E' scollegata o e' rotta!)
            exit = true;
        }
    }
}
//-------------------------------------------------------------------------------------------
}//namespace Z
