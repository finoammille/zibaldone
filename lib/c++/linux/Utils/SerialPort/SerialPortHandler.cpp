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

#include "SerialPortHandler.h"
#include "Events.h"

namespace Z
{
//-------------------------------------------------------------------------------------------
static void exceptionHandler(SerialPortException& spEx, std::string portName)//metodo statico (scope locale) per evitare codice ripetuto
{
    zibErr serialPortErrorEvent(SerialPortHandler::getSerialPortErrorLabel(portName), spEx.ErrorMessage());
    serialPortErrorEvent.emitEvent();//l'evento viene emesso in modo che l'utilizzatore della seriale sappia che questa porta non funziona.
}
//-------------------------------------------------------------------------------------------
SerialPortHandler::SerialPortHandler(const std::string& portName,
                                     int baudRate,
                                     SerialPort::Parity parity,
                                     int dataBits,
                                     int stopBits,
                                     SerialPort::FlowControl flwctrl,
                                     bool toggleDtr,
                                     bool toggleRts):
sp(portName, baudRate, parity, dataBits, stopBits, flwctrl, toggleDtr,
toggleRts), reader(sp)
{
    exit = false;
    /*
     * autoregistrazione sull'evento di richiesta di scrittura sulla seriale.
     * Chi vuol inviare dati sulla porta seriale "_portName" deve
     * semplicemente inviare un evento  (Event) con label=txDataLabel
     */
    register2Label(getTxDataLabel());
}

void SerialPortHandler::run()
{
    while(!exit){
        try {
            Event* Ev = pullOut();
            if(Ev){
                ziblog(LOG::INF, "received event with label %s", Ev->label().c_str());
                if(dynamic_cast<StopThreadEvent *>(Ev)) exit = true;
                else if(dynamic_cast<RawByteBufferData *>(Ev)) {
                    RawByteBufferData* txDataEvent = (RawByteBufferData*)Ev;
                    sp.Write(txDataEvent->buf(), txDataEvent->len());
                } else ziblog(LOG::ERR, "unexpected event");
                delete Ev;
            }
        } catch(SerialPortException spEx){
            exceptionHandler(spEx, sp.portName);
            exit = true;//devo comunque uscire per evitare di continuare a emettere lo stesso
                        //errore ripetutamente (la seriale non funziona! E' scollegata o e' rotta!)
        }
    }
}

void SerialPortHandler::Start()
{
    Thread::Start();
    reader.Start();
}

void SerialPortHandler::Stop()
{
    reader.Stop();
    Thread::Stop();
}

void SerialPortHandler::Join()
{
    //N.B.: non e` possibile che reader stia girando ma TcpConnHandler sia terminato.
    reader.Join();
    Thread::Stop();
}

SerialPortHandler::Reader::Reader(SerialPort& sp):exit(false), sp(sp)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,22)
    if((efd=eventfd(0, O_NONBLOCK))==-1) ziblog(LOG::ERR, "efd error");
#endif
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,22)

void SerialPortHandler::Reader::run()
{
    fd_set rdfs;//file descriptor set
    std::vector<unsigned char> rxData;
    int nfds=(efd>sp.fd ? efd : sp.fd)+1;
    while(!exit) {
        try {
            FD_ZERO(&rdfs);
            FD_SET(sp.fd, &rdfs);
            FD_SET(efd, &rdfs);
            if(select(nfds, &rdfs, NULL, NULL, NULL)==-1) ziblog(LOG::ERR, "select error");
            else {
                if(FD_ISSET(sp.fd, &rdfs)) {//data available on serial port
                                            //dati presenti sulla seriale
                    rxData = sp.Read();
                    if(!rxData.empty()){
                        unsigned char* buffer = new unsigned char[rxData.size()];
                        for(size_t i=0; i<rxData.size(); i++)buffer[i]=rxData[i];
                        RawByteBufferData rx(SerialPortHandler::getRxDataLabel(sp.portName), buffer, rxData.size());
                        rx.emitEvent();
                        delete buffer;
                    }
                } else if(FD_ISSET(efd, &rdfs)) {//stop event
                                                 //evento di stop
                    unsigned char exitFlag[8];
                    if(read(efd, exitFlag, 8)==-1) ziblog(LOG::ERR, "read from efd error (%s)", strerror(errno));
                    if(exitFlag[7]!=1) ziblog(LOG::ERR, "unexpected exitFlag value (%d)", exitFlag[7]);
                    exit=true;
                } else ziblog(LOG::ERR, "select lied to me!");
            }
        } catch(SerialPortException spEx){
            exceptionHandler(spEx, sp.portName);
            return;//devo comunque uscire per evitare di continuare a emettere lo stesso
                   //errore ripetutamente (la seriale non funziona! E' scollegata o e' rotta!)
        }
    }
}

#else//KERNEL OLDER THAN 2.6.22

void SerialPortHandler::Reader::run()
{
    fd_set rdfs;//file descriptor set
    std::vector<unsigned char> rxData;
    int nfds=sp.fd+1;
	timeval tv;
    while(!exit) {
        try {
            FD_ZERO(&rdfs);
            FD_SET(sp.fd, &rdfs);
			tv.tv_sec = 1;//1 sec polling to catch any Stop()
                          //1 sec di polling x eventuale Stop()
			tv.tv_usec = 0;
			int ret = select(nfds, &rdfs, NULL, NULL, &tv);
			if(ret) {
				if (ret<0) ziblog(LOG::ERR, "select error (%s)", strerror(errno));
				else {
					if(FD_ISSET(sp.fd, &rdfs)) {//available data on serial port
                                                //dati presenti sulla seriale
						rxData = sp.Read();
						if(!rxData.empty()){
							unsigned char* buffer = new unsigned char[rxData.size()];
							for(size_t i=0; i<rxData.size(); i++)buffer[i]=rxData[i];
							RawByteBufferData rx(SerialPortHandler::getRxDataLabel(sp.portName), buffer, rxData.size());
							rx.emitEvent();
							delete buffer;
						}
					} /* N.B.: select returns if there are available data on serial port and sets _sockId in rdfs,
				       * or if timeout (1 sec in our case). So. if Stop() is called then exit is set to true and
                       * thread exits gently
                       * :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
					   * N.B.: la select ritorna se c'e` qualcosa sul socket, e nel caso setta _sockId in rdfs, oppure
				       * per timeout (nel nostro caso impostato ad 1 secondo). Quindi se viene chiamato il metodo Stop()
					   * questo imposta exit=true che permette l'uscita pulita dal thread.
				       */
				}
			}
        } catch(SerialPortException spEx){
            exceptionHandler(spEx, sp.portName);
            return;//devo comunque uscire per evitare di continuare a emettere lo stesso
                   //errore ripetutamente (la seriale non funziona! E' scollegata o e' rotta!)
        }
    }
}

#endif

void SerialPortHandler::Reader::Start()
{
    exit=false;
    Thread::Start();
}

void SerialPortHandler::Reader::Stop()
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,22)
    unsigned char exitFlag[]={0,0,0,0,0,0,0,1};
    if(write(efd, exitFlag, 8)==-1) ziblog(LOG::ERR, "reader stop error (%s)", strerror(errno));
#else
	exit=true;
#endif
}
//-------------------------------------------------------------------------------------------
}//namespace Z
