/*
 *
 * zibaldone - a C++/Java library for Thread, Timers and other Stuff
 *
 * http://sourceforge.net/projects/zibaldone/
 *
 * version 3.2.0, February 14th, 2016
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

#ifndef _SERIALPORTHANDLER_H
#define	_SERIALPORTHANDLER_H
#include "Thread.h"
#include "SerialPort.h"

/*
 * SerialPortHandler implements a thread who handles a serial port
 *
 * 1) SerialPortHandler emits an event of type RawByteBufferData with label = "rxDataEventxxx",
 *    where "xxx" uniquely identifies the serial port managed by SerialPortHandler (e.g.
 *    \dev\ttyS0), every time it receives a byte-burst data from the port. The event name (label)
 *    can be obtained by calling getRxDataLabel method. As usual, who wants to receive the event,
 *    has to register to the event.
 *
 * 2) SerialPortHandler registers to receive events of type RawByteBufferData with label =
 *    "txDataEventxxx", where "xxx" uniquely identifies the serial port managed by
 *    SerialPortHandler (e.g. \dev\ttyS0). The event name (label) can be obtained by calling
 *    getTxDataLabel method: every time someone wants to transmit something over the serial port, he
 *    has to emit a RawByteBufferData event with this label.
 *
 * 3) SerialPortHandler emits a zibErr event with label = "serialPortErrorEventxxx", where "xxx"
 *    uniquely identifies the serial port managed by SerialPortHandler (e.g. \dev\ttyS0), in case
 *    of error. The event label can be obtained by means of the method getSerialPortErrorLabel
 *
 * ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
 *
 * SerialPortHandler implementa un thread che gestisce la porta seriale.
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
    void run();//write thread loop
               //ciclo del thread che gestisce le richieste di trasmissione sulla porta (writer)
    class Reader : public Thread {
		bool exit;
        SerialPort& sp;
        void run();//read thread loop
                   //ciclo del thread in ascolto sulla porta
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
    std::string getRxDataLabel(){return "rxDataEvent"+sp.portName;}//label for rx events emitted by SerialPortHandler
                                                                   //label ricezione emesso da SerialPortHandler
	static std::string getRxDataLabel(const std::string& port) {return "rxDataEvent"+port;}//label ricezione emesso da SerialPortHandler
                                                                                           //label ricezione emesso da SerialPortHandler
    std::string getTxDataLabel(){return "txDataEvent"+sp.portName;}//label for tx requests events
                                                                   //label x richiesta di trasmissione, ricevuto da SerialPortHandler
    static std::string getTxDataLabel(const std::string& port) {return "txDataEvent"+port;}//label for tx request events
                                                                                           //label x richiesta di trasmissione, ricevuto da SerialPortHandler
    std::string getSerialPortErrorLabel(){return "serialPortErrorEvent"+sp.portName;}//label for serial error
                                                                                     //label x serial error, ricevuto da SerialPortHandler
    static std::string getSerialPortErrorLabel(const std::string& port) {return "serialPortErrorEvent"+port;}//label for serial error
                                                                                                             //label x serial error, ricevuto da SerialPortHandler
};
//-------------------------------------------------------------------------------------------
}//namespace Z
#endif	/* _SERIALPORTHANDLER_H */
