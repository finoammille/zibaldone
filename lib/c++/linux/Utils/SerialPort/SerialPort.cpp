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

#include "SerialPort.h"

namespace Z
{
//-------------------------------------------------------------------------------------------
bool SerialPort::Open(const std::string& portName)
{
    fd = open(portName.c_str(), O_RDWR | O_NOCTTY | O_NDELAY);
    if(fd < 0) {
        ziblog(LOG::ERR, "error (code = %d) while trying to open port %s", fd, portName.c_str());
        throw SerialPortException("OPEN ERROR");
    } else fcntl(fd, F_SETFL, 0);//clear all flags on descriptor, enable direct I/O
    return (fd);
}

void SerialPort::ToggleDtr()
{
    int status;
    ioctl(fd, TIOCMGET, &status);
    status &= ~TIOCM_DTR;
    ioctl(fd, TIOCMSET, &status);
}

void SerialPort::ToggleRts()
{
    int status;
    ioctl(fd, TIOCMGET, &status);
    status &= ~TIOCM_RTS;
    ioctl(fd, TIOCMSET, &status);
}

int SerialPort::Write(const unsigned char* data, const int len)
{
    int n = write(fd, data, len);
    if(n<0) {
        ziblog(LOG::ERR, "Error on write of %d bytes", len);
        throw SerialPortException("WRITE ERROR");
    }
    return n;
}

std::vector<unsigned char> SerialPort::Read()
{
    unsigned char rxByte;
    int rxByteNum;
    std::vector<unsigned char> buffer;
    ioctl(fd, FIONREAD, &rxByteNum);
    for(int i = 0; i<rxByteNum; i++){
        if((read(fd, &rxByte, 1))<0) {
            ziblog(LOG::WRN, "SERIAL ERROR");
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
        default: ziblog(LOG::ERR, "SetBaudRate = %d failed", rate);
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
#ifdef CMSPAR //arm board
            _serialPortSettings.c_cflag &= ~CMSPAR;//reset degli altri flag della parita'
#endif
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
#ifdef CMSPAR //arm board
            _serialPortSettings.c_cflag |= (PARENB | CMSPAR);
#endif
            _serialPortSettings.c_iflag |= INPCK;
            _serialPortSettings.c_iflag &= ~(IGNPAR);//don't ignore parity errors
            _serialPortSettings.c_iflag &= ~(PARMRK);//and don't mark them either
            break;
        case SerialPort::ParitySpace:
#ifdef CMSPAR //arm board
            _serialPortSettings.c_cflag |= (PARENB | CMSPAR | CMSPAR);
#endif
            _serialPortSettings.c_iflag |= INPCK;
            _serialPortSettings.c_iflag &= ~(IGNPAR);//don't ignore parity errors
            _serialPortSettings.c_iflag &= ~(PARMRK);//and don't mark them either
            break;
        default: ziblog(LOG::ERR, "SetParityBits = %d failed (is it a wrong value?)", parity);
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
        default: ziblog(LOG::ERR, "SetDataBits = %d failed (is it a wrong value?)", dataBits);
    }
}

void SerialPort::SetStopBits(int stopBits)
{
    if(stopBits == 1) _serialPortSettings.c_cflag &= ~CSTOPB;
    else if(stopBits == 2) _serialPortSettings.c_cflag |= CSTOPB;
    else ziblog(LOG::ERR, "SetStopBits = %d failed (is it a wrong value?)", stopBits);
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
    bool toggleRts):portName(portName)
{
    Open(portName);
    if(toggleDtr) ToggleDtr();
    if(toggleRts) ToggleRts();
    tcgetattr(fd, &_serialPortSettings);
    SetBaudRate(baudRate);
    SetParity(parity);
    SetDataBits(dataBits);
    SetStopBits(stopBits);
    SetLocal(true);
    SetFlowControl(flwctrl);
    SetRaw(true, 0, 10);
    tcflush(fd, TCIFLUSH);
    tcsetattr(fd,TCSANOW,&_serialPortSettings);
    usleep(10000);//10 msec sleep. Normally not necessary, but sometimes we might need this (i.e. for ftdi serial port on a usb hub)
                  //10 msec per dare tempo alla seriale. In genere non serve, ma incerti casi (x es. seriali ftdi su hub usb) e` necessario
}

SerialPort::~SerialPort(){close(fd);}
//-------------------------------------------------------------------------------------------
}//namespace Z
