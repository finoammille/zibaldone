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

#ifndef _LINUXSERIALPORT_H
#define	_LINUXSERIALPORT_H
#include <iostream>
#include <vector>
#include <fcntl.h>//File control definitions
#include <sys/ioctl.h>
#include <unistd.h>//UNIX standard function definitions
#include <errno.h>//Error number definitions
#include <termios.h>//POSIX terminal control definitions

#include "Log.h"

class SerialPort
{
private:
    int _fd;//File descriptor for the port

public:
    enum Parity {ParityNone, ParityEven, ParityOdd , ParitySpace, ParityMark};
    enum FlowControl{FlowControlNone, FlowControlHardware, FlowControlXonXoff};

private:
    struct termios _serialPortSettings;
    bool Open(std::string);
    void SetBaudRate(int);//NOTA: tutti i metodi che modificano i parametri della seriale (SetBaudRate, SetFlowControl, ...) ovviamente
                          //possono essere usati solamente dopo la open. Non metto controlli in tal senso perche' tali metodi sono privati
                          //all'interno di SerialPort, e li uso solo io (che so come usarli!)
    void SetFlowControl(FlowControl);
    void SetDataBits(int);
    void SetStopBits(int);
    void SetParity(Parity);
    void SetLocal(bool);
    void SetRaw(bool, cc_t, cc_t);
    void ToggleDtr();
    void ToggleRts();

public:
    SerialPort(std::string portName,
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

#endif	/* _LINUXSERIALPORT_H */
