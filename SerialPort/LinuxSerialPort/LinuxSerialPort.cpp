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

#include "LinuxSerialPort.h"

namespace Z 
{
//-------------------------------------------------------------------------------------------
bool SerialPort::Open(const std::string& portName)
{
    _fd = open(portName.c_str(), O_RDWR | O_NOCTTY | O_NDELAY);
    if(_fd < 0) ziblog(LOG::ERROR, "error (code = %d) while trying to open port %s", _fd, portName.c_str());
    else fcntl(_fd, F_SETFL, 0);
    return (_fd);
}

void SerialPort::ToggleDtr()
{
    int status;
    ioctl(_fd, TIOCMGET, &status);
    status &= ~TIOCM_DTR;
    ioctl(_fd, TIOCMSET, &status);
}

void SerialPort::ToggleRts()
{
    int status;
    ioctl(_fd, TIOCMGET, &status);
    status &= ~TIOCM_RTS;
    ioctl(_fd, TIOCMSET, &status);
}

int SerialPort::Write(const unsigned char* data, const int len)
{
    int n = write(_fd, data, len);
    if(n<0) {
        ziblog(LOG::ERROR, "Error on write of %d bytes", len);
        throw SerialPortException("WRITE ERROR");
    }
    return n;
}

std::vector<unsigned char> SerialPort::Read()
{
    unsigned char rxByte;
    int rxByteNum;
    std::vector<unsigned char> buffer;
    ioctl(_fd, FIONREAD, &rxByteNum);
    for(int i = 0; i<rxByteNum; i++){
        if((read(_fd, &rxByte, 1))<0) {
            ziblog(LOG::WARNING, "SERIAL ERROR");
            throw SerialPortException("READ ERROR");
        }
        buffer.push_back(rxByte);
    }
    return buffer;
}

void SerialPort::SetBaudRate(int rate)
{
    switch(rate) {
        case 0: rate = B0; break;
        case 50: rate = B50; break;
        case 75: rate = B75; break;
        case 110: rate = B110; break;
        case 134: rate = B134; break;
        case 150: rate = B150; break;
        case 200: rate = B200; break;
        case 300: rate = B300; break;
        case 600: rate = B600; break;
        case 1200: rate = B1200; break;
        case 1800: rate = B1800; break;
        case 2400: rate = B2400; break;
        case 4800: rate = B4800; break;
        case 9600: rate = B9600; break;
        case 19200: rate = B19200; break;
        case 38400: rate = B38400; break;
        case 57600: rate = B57600; break;
        case 115200: rate = B115200; break;
        default: ziblog(LOG::ERROR, "SetBaudRate = %d failed", rate);
    }
    //adds new settings
    _serialPortSettings.c_cflag &= ~(CBAUD | CBAUDEX);
    _serialPortSettings.c_cflag |= rate;
    cfsetispeed(&_serialPortSettings, rate);
    cfsetospeed(&_serialPortSettings, rate);
}

void SerialPort::SetParity(SerialPort::Parity parity)
{
    _serialPortSettings.c_cflag &= ~PARENB;
    _serialPortSettings.c_cflag &= ~CSTOPB;
    switch(parity) {
        case SerialPort::ParityNone:
            _serialPortSettings.c_cflag &= ~PARENB;//disable parity generation and check
            _serialPortSettings.c_cflag &= ~PARODD;//reset degli altri flag della parita'
            _serialPortSettings.c_cflag &= ~CMSPAR;//reset degli altri flag della parita'
            _serialPortSettings.c_iflag &= ~(INPCK);//disable input parity checking
            break;
        case SerialPort::ParityEven:
            _serialPortSettings.c_cflag |= PARENB;//enable parity generation and check
            _serialPortSettings.c_cflag &= ~PARODD;//reset parity odd flag
            _serialPortSettings.c_iflag |= INPCK;//enable input parity checking
            _serialPortSettings.c_iflag &= ~(IGNPAR);//don't ignore parity errors
            _serialPortSettings.c_iflag &= ~(PARMRK);//and don't mark them either
            break;
        case SerialPort::ParityOdd:
            _serialPortSettings.c_cflag |= PARENB;//enable parity generation and check
            _serialPortSettings.c_cflag |= PARODD;//set parity odd flag
            _serialPortSettings.c_iflag |= INPCK;//enable input parity checking
            _serialPortSettings.c_iflag &= ~(IGNPAR);//don't ignore parity errors
            _serialPortSettings.c_iflag &= ~(PARMRK);//and don't mark them either
            break;
        case SerialPort::ParityMark:
            _serialPortSettings.c_cflag |= (PARENB | CMSPAR);
            _serialPortSettings.c_iflag |= INPCK;
            _serialPortSettings.c_iflag &= ~(IGNPAR);//don't ignore parity errors
            _serialPortSettings.c_iflag &= ~(PARMRK);//and don't mark them either
            break;
        case SerialPort::ParitySpace:
            _serialPortSettings.c_cflag |= (PARENB | CMSPAR | CMSPAR);
            _serialPortSettings.c_iflag |= INPCK;
            _serialPortSettings.c_iflag &= ~(IGNPAR);//don't ignore parity errors
            _serialPortSettings.c_iflag &= ~(PARMRK);//and don't mark them either
            break;
        default: ziblog(LOG::ERROR, "SetParityBits = %d failed (is it a wrong value?)", parity);
    }
}

void SerialPort::SetDataBits(int dataBits)
{
    switch(dataBits){
        case 5:
            _serialPortSettings.c_cflag = (_serialPortSettings.c_cflag & ~CSIZE) | CS5;
            break;
        case 6: 
            _serialPortSettings.c_cflag = (_serialPortSettings.c_cflag & ~CSIZE) | CS6;
            break;
        case 7: 
            _serialPortSettings.c_cflag = (_serialPortSettings.c_cflag & ~CSIZE) | CS7;
            _serialPortSettings.c_iflag |= ISTRIP;
            break;
        case 8:
            _serialPortSettings.c_cflag = (_serialPortSettings.c_cflag & ~CSIZE) | CS8;
            _serialPortSettings.c_iflag &= ~ISTRIP;
            break;
        default: ziblog(LOG::ERROR, "SetDataBits = %d failed (is it a wrong value?)", dataBits);
    }
}

void SerialPort::SetStopBits(int stopBits)
{
    if(stopBits == 1) _serialPortSettings.c_cflag &= ~CSTOPB;
    else if(stopBits == 2) _serialPortSettings.c_cflag |= CSTOPB;
    else ziblog(LOG::ERROR, "SetStopBits = %d failed (is it a wrong value?)", stopBits);
}

void SerialPort::SetLocal(bool isLocal)
{
    if(isLocal) {
        _serialPortSettings.c_cflag |= CLOCAL;//indica che non bisogna controllare le line (come il carrier detect) del modem.
        _serialPortSettings.c_cflag &= ~HUPCL;
    } else {
        _serialPortSettings.c_cflag &= ~CLOCAL;
        _serialPortSettings.c_cflag |= HUPCL;
    }
}

void SerialPort::SetFlowControl(SerialPort::FlowControl flwctrl)
{
    switch(flwctrl) {
        case SerialPort::FlowControlNone:
            _serialPortSettings.c_cflag &= ~CRTSCTS;
            _serialPortSettings.c_iflag &= ~IXON;
            _serialPortSettings.c_iflag &= ~IXOFF;
            _serialPortSettings.c_iflag &= ~IXANY;
            break;
        case SerialPort::FlowControlHardware:
            _serialPortSettings.c_cflag |= CRTSCTS;
            _serialPortSettings.c_iflag &= ~IXON;
            _serialPortSettings.c_iflag &= ~IXOFF;
            _serialPortSettings.c_iflag &= ~IXANY;
            break;
        case SerialPort::FlowControlXonXoff:
            _serialPortSettings.c_cflag &= ~CRTSCTS;
            _serialPortSettings.c_iflag |= IXON;
            _serialPortSettings.c_iflag |= IXOFF;
            _serialPortSettings.c_iflag |= IXANY;
            break;
    }
}

void SerialPort::SetRaw(bool isRaw, cc_t rawlen, cc_t rawtimeout)
{
    if(isRaw) {
        _serialPortSettings.c_iflag |= IGNBRK;
        _serialPortSettings.c_iflag &= ~(IGNCR | INLCR | ICRNL | IUCLC );
        _serialPortSettings.c_oflag &= ~OPOST;
        _serialPortSettings.c_lflag &= ~(ICANON | ISIG | ECHO | ECHOE | IEXTEN | XCASE);
        _serialPortSettings.c_cc[VMIN] = rawlen;
        _serialPortSettings.c_cc[VTIME] = rawtimeout;
    } else {
        _serialPortSettings.c_iflag |= IGNBRK | ICRNL;
        _serialPortSettings.c_iflag &= ~(IGNCR | INLCR | IUCLC );
        _serialPortSettings.c_oflag |= OPOST | ONLCR;
        _serialPortSettings.c_lflag |= (ICANON | ISIG | ECHOE | ECHOCTL | IEXTEN);
        _serialPortSettings.c_lflag &= ~ECHO;
        _serialPortSettings.c_cc[VEOL] = 0x0;
        _serialPortSettings.c_cc[VEOL2] = 0x0;
        _serialPortSettings.c_cc[VEOF] = 0x04;
    }
}

SerialPort::SerialPort(
    const std::string& portName,
    int baudRate,
    SerialPort::Parity parity,
    int dataBits,
    int stopBits,
    SerialPort::FlowControl flwctrl,
    bool toggleDtr,
    bool toggleRts)
{
    if(!Open(portName)) throw SerialPortException("OPEN ERROR");
    if(toggleDtr) ToggleDtr();
    if(toggleRts) ToggleRts();
    tcgetattr(_fd, &_serialPortSettings);
    SetBaudRate(baudRate);
    SetParity(parity);
    SetDataBits(dataBits);
    SetStopBits(stopBits);
    SetLocal(true);
    SetFlowControl(flwctrl);
    SetRaw(true, 0, 10);
    tcsetattr(_fd,TCSANOW,&_serialPortSettings);
    usleep(10000);//10 msec per dare tempo alla seriale. In genere non serve, ma incerti casi (x es. seriali ftdi su hub usb) e` necessario
}

SerialPort::~SerialPort(){close(_fd);}
//-------------------------------------------------------------------------------------------
}//namespace Z
