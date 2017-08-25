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

#include "PfLocalIpc.h"
#include "Events.h"

namespace Z
{
//-------------------------------------------------------------------------------------------
PfLocalConnHandler::PfLocalConnHandler(int sockId, const std::string& IpcSocketName):
                    _sockId(sockId), _IpcSocketName(IpcSocketName), reader(sockId)
{
    exit = false;
    std::stringstream sap;
    sap<<_sockId;
    _sap = sap.str();
    register2Label(getTxDataLabel());//self-registration to transmission request event emitted by class-users
                                     //autoregistrazione sugli eventi di richiesta di trasmissione inviatigli dagli utilizzatori
}

PfLocalConnHandler::~PfLocalConnHandler()
{
    close(_sockId);
    if(!_IpcSocketName.empty()) unlink(_IpcSocketName.c_str());
}

void PfLocalConnHandler::run()
{
    while(!exit){
        Event* Ev = pullOut();
        if(Ev){
            ziblog(LOG::INF, "received event with label %s", Ev->label().c_str());
            if(dynamic_cast<StopThreadEvent *>(Ev)) exit = true;
            else if(dynamic_cast<RawByteBufferData *>(Ev)) {
                RawByteBufferData* txDataEvent = (RawByteBufferData*)Ev;
                write(_sockId, txDataEvent->buf(), txDataEvent->len());
            } else ziblog(LOG::ERR, "unexpected event");
            delete Ev;
        }
    }
}

void PfLocalConnHandler::Start()
{
    Thread::Start();
    reader.Start();
}

void PfLocalConnHandler::Stop()
{
    reader.Stop();
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,22)//eventFd is not available with this version so we have to do polling
                                               //non c'e` eventFd x cui occorre aspettare il polling...
    reader.Join();
#endif
    Thread::Stop();
}

void PfLocalConnHandler::Join()
{
    //N.B.: non e` possibile che reader stia girando ma TcpConnHandler sia terminato.
    reader.Join();
    Thread::Stop();
}

PfLocalConnHandler::Reader::Reader(int sockId):exit(false), _sockId(sockId)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,22)
    if((efd=eventfd(0, O_NONBLOCK))==-1) ziblog(LOG::ERR, "efd error (%s)", strerror(errno));
#endif
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,22)

PfLocalConnHandler::Reader::~Reader(){close(efd);}

void PfLocalConnHandler::Reader::run()
{
    std::stringstream sap;
    sap<<_sockId;
    std::string rxDataLabel = "rxDataEvent"+sap.str();
    const int maxSize = 1024;
    unsigned char rxbyte[maxSize];
    fd_set rdfs;//file descriptor set
    int nfds=(efd>_sockId ? efd : _sockId)+1;
    while(!exit) {
        FD_ZERO(&rdfs);
        FD_SET(_sockId, &rdfs);
        FD_SET(efd, &rdfs);
        if(select(nfds, &rdfs, NULL, NULL, NULL)==-1) ziblog(LOG::ERR, "select error (%s)", strerror(errno));
        else {
            if(FD_ISSET(_sockId, &rdfs)) {//available data on socket
                                          //dati presenti sul socket
                int len = 0;
                if((len = read(_sockId, rxbyte, maxSize)) > 0) {
                    RawByteBufferData rx(rxDataLabel, rxbyte, len);
                    rx.emitEvent();
                } else if(len == 0) break; /* the peer has closed the socket. We have to exit thread loop.
                                            * We do not have to close socket here because it's up to the
                                            * destructor ~PfLocalConnHandler
                                            * :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
                                            * il peer ha chiuso il socket. Esco dal loop del thread. La close
                                            * del socket non devo farla qui perche' la fa gia' il distruttore
                                            * di PfLocalConnHandler
                                            */
            } else if(FD_ISSET(efd, &rdfs)) {//stop event
                                             //evento di stop
                unsigned char exitFlag[8];
                if(read(efd, exitFlag, 8)==-1) ziblog(LOG::ERR, "read from efd error (%s)", strerror(errno));
                if(exitFlag[7]!=1) ziblog(LOG::ERR, "unexpected exitFlag value (%d)", exitFlag[7]);
                exit=true;
            } else ziblog(LOG::ERR, "select lied to me!");
        }
    }
}

#else//KERNEL OLDER THAN 2.6.22

void PfLocalConnHandler::Reader::run()
{
    std::stringstream sap;
    sap<<_sockId;
    std::string rxDataLabel = "rxDataEvent"+sap.str();
    const int maxSize = 1024;
    unsigned char rxbyte[maxSize];
    fd_set rdfs;//file descriptor set
    int nfds=_sockId+1;
	timeval tv;
    while(!exit) {
        FD_ZERO(&rdfs);
        FD_SET(_sockId, &rdfs);
		tv.tv_sec = 1;//1 sec polling to catch any Stop()
                      //1 sec di polling x eventuale Stop()
		tv.tv_usec = 0;
		int ret=select(nfds, &rdfs, NULL, NULL, NULL);
		if(ret) {
			if(ret<0) ziblog(LOG::ERR, "select error (%s)", strerror(errno));
			else {
				if(FD_ISSET(_sockId, &rdfs)) {//available data on socket
                                              //dati presenti sul socket
					int len = 0;
					if((len = read(_sockId, rxbyte, maxSize)) > 0) {
						RawByteBufferData rx(rxDataLabel, rxbyte, len);
						rx.emitEvent();
					} else if(len == 0) break; /* the peer has closed the socket. We have to exit thread loop.
                                                * We do not have to close socket here because it's up to the
                                                * destructor ~PfLocalConnHandler
                                                * :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
                                                * il peer ha chiuso il socket. Esco dal loop del thread. La close
												* del socket non devo farla qui perche' la fa gia' il distruttore
												* di PfLocalConnHandler
												*/
				} /* N.B.: select returns if there are available data on socket and sets _sockId in rdfs, or
				   * if timeout (1 sec in our case). So. if Stop() is called then exit is set to true and
                   * thread exits gently
                   * :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
                   * nota: la select ritorna se c'e` qualcosa sul socket, e nel caso setta _sockId in rdfs, oppure
				   * per timeout (nel nostro caso impostato ad 1 secondo). Quindi se viene chiamato il metodo Stop()
				   * questo imposta exit=true che permette l'uscita pulita dal thread.
				   */
			}
		}
    }
}

#endif

void PfLocalConnHandler::Reader::Start()
{
    exit=false;
    Thread::Start();
}

void PfLocalConnHandler::Reader::Stop()
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,22)
    unsigned char exitFlag[]={0,0,0,0,0,0,0,1};
    if(write(efd, exitFlag, 8)==-1) ziblog(LOG::ERR, "reader stop error (%s)", strerror(errno));
#else
	exit=true;
#endif
}

PfLocalServer::PfLocalServer(const std::string& IpcSocketName):_IpcSocketName(IpcSocketName)
{
    if((_sockId = socket(PF_LOCAL, SOCK_STREAM, 0))<0) {
        ziblog(LOG::ERR, "Socket Error (%s)!", strerror(errno));
        return;
    }
    sockaddr_un sockAddr;
    sockAddr.sun_family = PF_UNIX;
    strcpy (sockAddr.sun_path, IpcSocketName.c_str());
    unlink(sockAddr.sun_path);//delete of any refuse
                              //rimuove un eventuale refuso
    if(bind(_sockId, (sockaddr *)&sockAddr, SUN_LEN(&sockAddr))<0){
        ziblog(LOG::ERR, "Socket Bind Error (%s)!", strerror(errno));
        return;
    }
    if(listen(_sockId, 1) < 0) ziblog(LOG::ERR, "Socket Listening Error");
}

PfLocalServer::~PfLocalServer() {close(_sockId);}

PfLocalConnHandler* PfLocalServer::Accept()
{
    int sockPfLocalConnHandlerId = accept(_sockId, NULL, NULL);//rimane bloccato qui sinche` non arriva una richiesta di connessione.
    if(sockPfLocalConnHandlerId < 0) {
        ziblog(LOG::ERR, "Connection Error (%s)", strerror(errno));
        return NULL;
    } else return new PfLocalConnHandler(sockPfLocalConnHandlerId, _IpcSocketName);//istanzio il thread che gestisce la connessione

}

PfLocalClient::PfLocalClient(const std::string& IpcSocketName)
{
    if((_sockId = socket(PF_LOCAL, SOCK_STREAM, 0))<0) {
        ziblog(LOG::ERR, "Socket Error (%s)!", strerror(errno));
        return;
    }
    sockaddr_un* addr = new sockaddr_un();
    addr->sun_family = PF_UNIX;
    strcpy(addr->sun_path, IpcSocketName.c_str());
    _addrlen = SUN_LEN(addr);
    _addr = (sockaddr* )addr;
}

PfLocalClient::~PfLocalClient() {delete((sockaddr_un*)_addr); _addr=NULL;}

PfLocalConnHandler* PfLocalClient::Connect()
{
    if(connect(_sockId, _addr, _addrlen) < 0) {
        ziblog(LOG::ERR, "connection to %s has failed (%s)", inet_ntoa(((struct sockaddr_in *)_addr)->sin_addr), strerror(errno));
        return NULL;
    }
    return new PfLocalConnHandler(_sockId, ((sockaddr_un*)_addr)->sun_path);
}
//-------------------------------------------------------------------------------------------
}//namespace Z
