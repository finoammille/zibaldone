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

#ifndef _SERIALPORT_H
#define	_SERIALPORT_H

#include <windows.h>
#include "Log.h"

namespace Z
{
//-------------------------------------------------------------------------------------------
class SerialPort
{
    friend class SerialPortHandler;
    HANDLE fd;//File descriptor for the port
    const std::string portName;
public:
    enum Parity {ParityNone, ParityEven, ParityOdd , ParitySpace, ParityMark};
    enum FlowControl{FlowControlNone, FlowControlHardware, FlowControlXonXoff};
private:
    DCB serialPortSettings;
    bool Open(const std::string&);
    void SetBaudRate(int);
    void SetParity(Parity);
    void SetDataBits(int);
    void SetStopBits(int);
    void SetFlowControl(FlowControl);
public:
    SerialPort(const std::string& portName,
        int baudRate,
        SerialPort::Parity parity,
        int dataBits,
        int stopBits,
        SerialPort::FlowControl,
        bool toggleDtr = false,
        bool toggleRts = false);
    ~SerialPort();//Destructor

    int Write(const unsigned char* data, const int len);
    std::vector<unsigned char> Read();
};

class SerialPortException {
    std::string _errorMessage;
public:
    SerialPortException(std::string errorMessage):_errorMessage(errorMessage){}
    std::string ErrorMessage() {return _errorMessage;}
};
//-------------------------------------------------------------------------------------------
}//namespace Z

#endif	/* _SERIALPORT_H */
