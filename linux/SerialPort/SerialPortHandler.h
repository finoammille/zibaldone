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

#ifndef _SERIALPORTHANDLER_H
#define	_SERIALPORTHANDLER_H
#include "Thread.h"
#include "LinuxSerialPort.h"

/*
 * implementa un thread in ascolto sulla porta seriale che emette l'evento
 * "rxDataEvent" ogni volta che riceve un byte sulla porta. Il nome dell'evento 
 * (eventId) e' ottenibile invocando il metodo getRxDataEventName.
 * Chi desidera ricevere l'evento deve registrarsi.
 */

namespace Z
{
//-------------------------------------------------------------------------------------------
class SerialPortHandler : public Thread {
protected:
    /*
     * in questo modo un thread che usa la seriale (praticamente tutti i driver)
     * puo' direttamente derivare da SerialPortHandler facendo l'override del
     * run() e gestire in modo personalizzato la porta.
     */
    SerialPort _sp;
    std::string _portName;//serve per taggare univocamente gli eventi relativi ad una specifica porta
    const std::string rxDataEventId;
    const std::string txDataEventId;
    const std::string serialPortErrorEventId;
    bool exit;
    void run();//ciclo del thread in ascolto sulla porta
public:    
    SerialPortHandler(const std::string& portName,
        int baudRate,
        SerialPort::Parity parity,
        int dataBits,
        int stopBits,
        SerialPort::FlowControl flwctrl,
        bool toggleDtr = false,
        bool toggleRts = false);//Constructor. Nota: ToggleDtr e ToggleRts normalmente non servono e sono impostate di
                                //default a false. In alcuni rari casi (per esempio nello stacker controllato tramite
                                //chip FT232RL della FTDI che simula una porta seriale via USB) occorre invertirli entrambi.
    std::string portName();
    std::string getRxDataEventId()const{return rxDataEventId;}//eventId ricezione emesso da SerialPortHandler
    static std::string getRxDataEventId(const std::string& port) {return "rxDataEvent"+port;}//eventId ricezione emesso da SerialPortHandler
    std::string getTxDataEventId()const{return txDataEventId;}//eventId x richiesta di trasmissione, ricevuto da SerialPortHandler
    static std::string getTxDataEventId(const std::string& port) {return "txDataEvent"+port;}//eventId x richiesta di trasmissione, ricevuto da SerialPortHandler
    std::string getSerialPortErrorEventId()const{return serialPortErrorEventId;}//eventId x serial error, ricevuto da SerialPortHandler
    static std::string getSerialPortErrorEventId(const std::string& port) {return "serialPortErrorEvent"+port;}//eventId x serial error, ricevuto da SerialPortHandler
};
//-------------------------------------------------------------------------------------------
}//namespace Z
#endif	/* _SERIALPORTHANDLER_H */
