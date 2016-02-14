/*
 *
 * zibaldone - a C++/Java library for Thread, Timers and other Stuff
 *
 * http://sourceforge.net/projects/zibaldone/
 *
 * version 3.2.0, February 14th, 2016
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
