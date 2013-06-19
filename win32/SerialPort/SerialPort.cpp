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

#include "SerialPort.h"

namespace Z
{
//-------------------------------------------------------------------------------------------
bool SerialPort::Open(const std::string& portName)
{
#if defined(_MSC_VER)
    std::wstring wportName = std::wstring(portName.begin(), portName.end());
    fd = CreateFile(wportName.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, 0);//Overlapped mode
#else
    fd = CreateFile(portName.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, 0);//Overlapped mode
#endif
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
    //NOTA: toggleDtr e toggleRts sono state inserite per gestire correttamente  le seriali
    //FTDI con linux. Per windows la soluzione migliore e` ignorare settaggi diversi da
    //quelli comunemente utilizzati che sono DTR_CONTROL_ENABLE e RTS_CONTROL_HANDSHAKE.
    //Nel caso servisse (dubito) aumentero` la "granularita`" dei settaggi anche in windows.
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
    Sleep(100);//100 msec per dare tempo alla seriale (... 10 volte il tempo che serve a linux ...)
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
				DWORD wRet=WaitForSingleObject(ov.hEvent, 50);//gestisco burst di byte a quanti di durata max = 50 msec.
                if (wRet == WAIT_TIMEOUT) {
                    if(!buffer.empty()) {
                        CancelIo(fd);
                        break;//burst finito.
                    }
                } else if(wRet==WAIT_OBJECT_0) {
                    if(!GetOverlappedResult(fd, &ov, &readBytes, TRUE)) {
                        ziblog(LOG::ERR, "GetOverlappedResult error (%ld)", GetLastError());
                        throw SerialPortException("READ ERROR");
                    }
                } else {//WAIT_ABANDONED oppure WAIT_FAILED
                    ziblog(LOG::ERR, "WaitForSingleObject error (%ld)", GetLastError());
                    throw SerialPortException("READ ERROR");
                }
			} else {
				ziblog(LOG::ERR, "ReadFile error (%ld)", GetLastError());
				throw SerialPortException("READ ERROR");
			}
		}
        if(synchronousOperation && !readBytes) break;//controllo se siamo in un'operazione sincrona perchè esiste la (remota) possibilità che
                                                     //che i 50 msec di timeout scadano prima della signal su ov.hEvent (IO ancora pending).
                                                     //In questo scenario, se il buffer è ancora vuoto (primo byte del burst) readBytes e`
                                                     //correttamente = 0, ma essendo il buffer ancora vuoto, la cancelIo non viene chiamata e
                                                     //uscirei (tra l'altro con una imminente signal su ov.hEvent) restituendo un buffer vuoto
                                                     //a fronte di una precedente segnalazione della WaitCommEvents (con conseguente emissione
                                                     //del warning "WaitCommEvent lied to me!"
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
