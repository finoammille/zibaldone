/*
 *
 * zibaldone - a C++/Java library for Thread, Timers and other Stuff
 *
 * http://sourceforge.net/projects/zibaldone/
 *
 * version 3.1.2, August 29th, 2015
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

#include "SerialPort.h"

namespace Z
{
//-------------------------------------------------------------------------------------------
bool SerialPort::Open(const std::string& portName)
{
    fd = CreateFile(portName.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, 0);//Overlapped mode
	if(fd==INVALID_HANDLE_VALUE) {
        ziblog(LOG::ERR, "error (code = %ld) while trying to open port %s", GetLastError(), portName.c_str());
        throw SerialPortException("OPEN ERROR");
    }
	return (fd!=INVALID_HANDLE_VALUE);
}

void SerialPort::SetBaudRate(int rate)
{
    switch(rate) {
        case 110: rate = CBR_110; break;
        case 300: rate = CBR_300; break;
        case 600: rate = CBR_600; break;
        case 1200: rate = CBR_1200; break;
        case 2400: rate = CBR_2400; break;
        case 4800: rate = CBR_4800; break;
        case 9600: rate = CBR_9600; break;
        case 14400: rate = CBR_14400; break;
        case 19200: rate = CBR_19200; break;
        case 38400: rate = CBR_38400; break;
        case 57600: rate = CBR_57600; break;
        case 115200: rate = CBR_115200; break;
        case 128000: rate = CBR_128000; break;
        case 256000: rate = CBR_256000; break;
        default: ziblog(LOG::ERR, "SetBaudRate = %d failed", rate);
    }
    //adds new settings
	serialPortSettings.BaudRate=rate;
}

void SerialPort::SetParity(SerialPort::Parity parity)
{
    switch(parity) {
        case SerialPort::ParityNone: serialPortSettings.Parity=NOPARITY; break;
        case SerialPort::ParityEven: serialPortSettings.Parity=EVENPARITY; break;
        case SerialPort::ParityOdd: serialPortSettings.Parity=ODDPARITY; break;
        case SerialPort::ParityMark: serialPortSettings.Parity=MARKPARITY; break;
        case SerialPort::ParitySpace: serialPortSettings.Parity=SPACEPARITY; break;
        default: ziblog(LOG::ERR, "SetParityBits = %d failed (is it a wrong value?)", parity);
    }
}

void SerialPort::SetDataBits(int dataBits)
{
    if(dataBits<5 || dataBits>8) ziblog(LOG::WRN, "SetDataBits = %d is a wrong value", dataBits);
    serialPortSettings.ByteSize = dataBits;
}

void SerialPort::SetStopBits(int stopBits)
{
    if(stopBits == 1) serialPortSettings.StopBits = ONESTOPBIT;
    else if(stopBits == 2) serialPortSettings.StopBits = TWOSTOPBITS;
    else ziblog(LOG::ERR, "SetStopBits = %d failed (is it a wrong value?)", stopBits);
    //N.B.: We do not support 1.5 stop bits. The aim of 1.5 stop bits was to gain 4% more
    //data throughput when a link is too long for one stop bit-time but is too short to
    //require two stop bit-times. 1.5 stop bit-times is now rare enough to be a hazard to use.
}

void SerialPort::SetFlowControl(SerialPort::FlowControl flwctrl)
{
    serialPortSettings.fBinary = TRUE;
    serialPortSettings.fOutxCtsFlow = FALSE;
    serialPortSettings.fOutxDsrFlow = FALSE;
    serialPortSettings.fDsrSensitivity = FALSE;
    serialPortSettings.fParity = TRUE;
    serialPortSettings.fOutX = FALSE;
    serialPortSettings.fInX = FALSE;
    serialPortSettings.fNull = FALSE;
    serialPortSettings.fAbortOnError = FALSE;
    serialPortSettings.fDtrControl = DTR_CONTROL_DISABLE;
    serialPortSettings.fDsrSensitivity= FALSE;
    serialPortSettings.fRtsControl = RTS_CONTROL_DISABLE;
    if(flwctrl == SerialPort::FlowControlHardware) {
        serialPortSettings.fOutxCtsFlow = TRUE;
        serialPortSettings.fDtrControl = DTR_CONTROL_ENABLE;
        serialPortSettings.fRtsControl = RTS_CONTROL_HANDSHAKE;
    } else if(flwctrl == SerialPort::FlowControlXonXoff) {
        serialPortSettings.fTXContinueOnXoff = FALSE;
        serialPortSettings.fOutX = TRUE;
        serialPortSettings.fInX = TRUE;
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
	memset(&serialPortSettings, 0, sizeof(DCB));
    Open(portName);
    DWORD err;
    if(!ClearCommError(fd, &err, NULL)) {
        ziblog(LOG::ERR, "ClearCommError failed (code = %ld)", GetLastError());
        throw SerialPortException("ClearCommError Failed");
    }
    if(err) ziblog(LOG::INF, "cleared err mask = %lx", err);
    if(!PurgeComm(fd, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR)) {
        ziblog(LOG::ERR, "PurgeComm failed (code = %ld)", GetLastError());
        throw SerialPortException("PurgeComm Failed");
    }
	if(!GetCommState(fd, &serialPortSettings)) throw SerialPortException("GET CURRENT SETTINGS ERROR");
    SetBaudRate(baudRate);
    SetParity(parity);
    SetDataBits(dataBits);
    SetStopBits(stopBits);
    SetFlowControl(flwctrl);
	if(!SetCommState(fd, &serialPortSettings)) throw SerialPortException("GET CURRENT SETTINGS ERROR");
    Sleep(100);
    if(!SetCommMask(fd, EV_ERR | EV_RXCHAR)) throw SerialPortException("SET COMM MASK ERROR");
}

std::vector<unsigned char> SerialPort::Read()
{
    OVERLAPPED ov;
    memset(&ov, 0, sizeof(OVERLAPPED));
    ov.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    std::vector<unsigned char> buffer;
    unsigned char rxByte;
    DWORD readBytes;
    for(;;) {
        bool synchronousOperation=true;
		if(!ReadFile(fd, &rxByte, 1, &readBytes, &ov)) {
            synchronousOperation=false;
			if(GetLastError()==ERROR_IO_PENDING) {
				DWORD wRet=WaitForSingleObject(ov.hEvent, 50);
                if (wRet == WAIT_TIMEOUT) {
                    if(!buffer.empty()) {
                        CancelIo(fd);
                        break;
                    }
                } else if(wRet==WAIT_OBJECT_0) {
                    if(!GetOverlappedResult(fd, &ov, &readBytes, TRUE)) {
                        ziblog(LOG::ERR, "GetOverlappedResult error (%ld)", GetLastError());
                        throw SerialPortException("READ ERROR");
                    }
                } else {//WAIT_ABANDONED or WAIT_FAILED
                    ziblog(LOG::ERR, "WaitForSingleObject error (%ld)", GetLastError());
                    throw SerialPortException("READ ERROR");
                }
			} else {
				ziblog(LOG::ERR, "ReadFile error (%ld)", GetLastError());
				throw SerialPortException("READ ERROR");
			}
		}
        if(synchronousOperation && !readBytes) break;
		buffer.push_back(rxByte);
    }
    CloseHandle(ov.hEvent);
    return buffer;
}

int SerialPort::Write(const unsigned char* data, const int len)
{
    OVERLAPPED ov;
    memset(&ov, 0, sizeof(OVERLAPPED));
    ov.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    DWORD writtenBytes=0;
    if(!WriteFile(fd, data, len, &writtenBytes, &ov)){
        if(GetLastError()==ERROR_IO_PENDING) {
            DWORD ret=WaitForSingleObject(ov.hEvent, INFINITE);
            if(ret==WAIT_OBJECT_0) {
                if(!GetOverlappedResult(fd, &ov, &writtenBytes, TRUE)) {
                    ziblog(LOG::ERR, "GetOverlappedResult error (%ld)", GetLastError());
                    throw SerialPortException("WRITE ERROR");
                }
            } else ziblog(LOG::ERR, "WaitForSingleObject exit code = %ld",ret);
        } else {
            ziblog(LOG::ERR, "WriteFile error (%ld)", GetLastError());
            throw SerialPortException("Write ERROR");
        }
    }
    if((int)writtenBytes < len) ziblog(LOG::ERR, "%d bytes were written instead of %d", (int)writtenBytes, len);
    CloseHandle(ov.hEvent);
    return writtenBytes;
}

SerialPort::~SerialPort()
{
    if(fd != INVALID_HANDLE_VALUE) {
        DWORD err;
        ClearCommError(fd, &err, NULL);
        PurgeComm(fd, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);
        CloseHandle(fd);
        fd = INVALID_HANDLE_VALUE;
    }
}
//-------------------------------------------------------------------------------------------

}//namespace Z
