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

#ifndef _SERIALPORTHANDLER_H
#define	_SERIALPORTHANDLER_H
#include "Thread.h"
#include "SerialPort.h"

#include <linux/version.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,22)
#include <sys/eventfd.h>
#else
#include <sys/time.h>
#endif

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
 */

namespace Z
{
//-------------------------------------------------------------------------------------------
class SerialPortHandler : public Thread {
    SerialPort sp;
    bool exit;
    void run();//write thread loop
    class Reader : public Thread {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,22)
        int efd; //Event file descriptor
#endif
        bool exit;
        SerialPort& sp;
        void run();//read thread loop
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
        bool toggleRts = false);
    void Start();
    void Stop();
    void Join();
    std::string getRxDataLabel(){return "rxDataEvent"+sp.portName;}//label for rx events emitted by SerialPortHandler
    static std::string getRxDataLabel(const std::string& port) {return "rxDataEvent"+port;}//label for rx events emitted by SerialPortHandler
    std::string getTxDataLabel(){return "txDataEvent"+sp.portName;}//label for tx requests events
    static std::string getTxDataLabel(const std::string& port) {return "txDataEvent"+port;}//label for tx request events
    std::string getSerialPortErrorLabel(){return "serialPortErrorEvent"+sp.portName;}//label for serial error
    static std::string getSerialPortErrorLabel(const std::string& port) {return "serialPortErrorEvent"+port;}//label for serial error
};
//-------------------------------------------------------------------------------------------
}//namespace Z
#endif	/* _SERIALPORTHANDLER_H */
