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

#ifndef _SERIALPORTHANDLER_H
#define	_SERIALPORTHANDLER_H
#include "Thread.h"
#include "SerialPort.h"
#include <sys/eventfd.h>

/*
 * implementa un thread che gestisce la porta seriale.
 *
 * 1) SerialPortHandler emette un evento di tipo RawByteBufferData con label = "rxDataEventxxx"
 *    dove "xxx" identifica univocamente la porta seriale gestita da SerialPortHandler (x es. 
 *    /dev/ttyS0) ogni volta che riceve un burst di byte sulla porta. Il nome dell'evento (label) 
 *    e' ottenibile invocando il metodo getRxDataLabel. Chi desidera ricevere l'evento deve registrarsi.
 *
 * 2) SerialPortHandler e` registrato per ricevere eventi di tipo RawByteBufferData con label = 
 *    "txDataEventxxx" dove "xxx" identifica univocamente la porta seriale gestita da SerialPortHandler 
 *    (x es. Quindi ogni volta che si vuole trasmettere dei byte sulla porta basta emettere un evento di 
 *    tipo RawByteBufferData avente come label la stringa identificativa ottenibile invocando il metodo 
 *    getTxDataLabel.
 *
 * 3) SerialPortHandler emette un evento zibErr con label = "serialPortErrorEventxxx" dove "xxx" 
 *    identifica univocamente la porta seriale gestita da SerialPortHandler (x es. /dev/ttyS0) in caso di 
 *    errore Il nome dell'evento (label) e' ottenibile invocando il metodo getSerialPortErrorLabel
 */

namespace Z
{
//-------------------------------------------------------------------------------------------
class SerialPortHandler : public Thread {
    SerialPort sp;
    bool exit;
    void run();//ciclo del thread che gestisce le richieste di trasmissione sulla porta (writer)
    class Reader : public Thread {
        int efd; //Event file descriptor
        bool exit;
        SerialPort& sp;
        void run();//ciclo del thread in ascolto sulla porta 
    public:
        Reader(SerialPort&);
        void Start();
        void Stop();
    } reader;
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
    void Start();
    void Stop();
    void Join();
    std::string getRxDataLabel(){return "rxDataEvent"+sp.portName;}//label ricezione emesso da SerialPortHandler
    static std::string getRxDataLabel(const std::string& port) {return "rxDataEvent"+port;}//label ricezione emesso da SerialPortHandler
    std::string getTxDataLabel(){return "txDataEvent"+sp.portName;}//label x richiesta di trasmissione, ricevuto da SerialPortHandler 
    static std::string getTxDataLabel(const std::string& port) {return "txDataEvent"+port;}//label x richiesta di trasmissione, ricevuto da SerialPortHandler
    std::string getSerialPortErrorLabel(){return "serialPortErrorEvent"+sp.portName;}//label x serial error, ricevuto da SerialPortHandler
    static std::string getSerialPortErrorLabel(const std::string& port) {return "serialPortErrorEvent"+port;}//label x serial error, ricevuto da SerialPortHandler
};
//-------------------------------------------------------------------------------------------
}//namespace Z
#endif	/* _SERIALPORTHANDLER_H */
