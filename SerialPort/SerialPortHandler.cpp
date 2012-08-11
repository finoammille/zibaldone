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

SerialPortHandler::SerialPortHandler(std::string portName,
                                     int baudRate,
                                     SerialPort::Parity parity,
                                     int dataBits,
                                     int stopBits,
                                     SerialPort::FlowControl flwctrl,                     
                                     bool toggleDtr,
                                     bool toggleRts):
_sp(portName, baudRate, parity, dataBits, stopBits, flwctrl, toggleDtr, toggleRts),
_portName(portName)
{
    exit = false;
    /*
     * autoregistrazione sull'evento di richiesta di scrittura sulla seriale.
     * Chi vuol inviare dati sulla porta seriale "_portName" deve
     * semplicemente inviare un evento  "DataEvent" di tipo "Tx" con
     * sap = _portName
     */
    register2Event(TxDataEvent::txDataEventName(_portName));
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
                RxDataEvent rx(_portName, buffer, rxData.size());
                rx.emitEvent();
                delete buffer;
            }
            Event* Ev = pullOut(10);//max 1/100 secondo di attesa
            if(Ev){
                std::string eventName = Ev->eventName();
                ziblog("received event %s", eventName.c_str());
                if(eventName == StopThreadEvent::StopThreadEventName) exit = true; 
                else if(eventName==(TxDataEvent::txDataEventName(_portName))){
                    TxDataEvent* w = (TxDataEvent *)Ev;
                    _sp.Write(w->buf(), w->len());
                    delete w;
                } 
            }
        } catch(SerialPortException spEx){
            SerialPortErrorEvent serialPortErrorEvent(_portName, spEx.ErrorMessage());
            serialPortErrorEvent.emitEvent();//nota: l'evento viene emesso in modo che l'utilizzatore della seriale sappia che questa non funziona. Poi devo comunque uscire (exit = true) per evitare di continuare a emettere lo stesso errore ripetutamente (la seriale non funziona! E' scollegata o e' rotta!)
            exit = true;
        }
    }
}
