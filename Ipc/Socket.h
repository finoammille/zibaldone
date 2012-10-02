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

#ifndef _SOCKET_H
#define	_SOCKET_H

#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>
#include <sstream>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#include "Thread.h"
#include "Log.h"

#define MAX_IPC_NET_CONNECTION  	1
#define MAX_IPC_LOCAL_CONNECTION  	50

class Socket {
protected:
    enum IpcType {LocalIpc, NetworkIpc} _ipcType;
    int _sockId;
    Socket(IpcType type);
};

class ConnHandler : public Thread {
    friend class Server;
    friend class Client;
    int _sockId;
    std::string _sap;//serve per taggare univocamente gli eventi relativi ad uno specifico socket
    bool exit;
    void run();
    ConnHandler(int sockId);//ConnHandler non deve essere istanziabile, ma solo ottenibile effettuando una connessione.
public:
    ~ConnHandler(){close(_sockId);}
    std::string getSap(){return _sap;}
    //eventi emessi da ConnHandler
    struct RxDataEvent : public DataEvent {
        RxDataEvent(std::string sap, const unsigned char* buf, int len):DataEvent(DataEvent::Rx, sap, buf, len){}
        static std::string rxDataEventName(std::string sap){return (DataEvent::DataEventName[DataEvent::Rx] + sap);}
    };
    //eventi ricevuti da ConnHandler
    struct TxDataEvent : public DataEvent {
        TxDataEvent(std::string sap, const unsigned char* buf, int len):DataEvent(DataEvent::Tx, sap, buf, len){}
        static std::string txDataEventName(std::string sap){return (DataEvent::DataEventName[DataEvent::Tx] + sap);}
    };
};

class Server : protected Socket {
    int _maxConnections;
    int _activeConnections;
    void bindAndListen(sockaddr *addr, int addrlen);//funzione di utilizzo interno.
public:
    Server(int port, const int maxConnections = MAX_IPC_NET_CONNECTION);
    Server(std::string IpcSocketName , const int maxConnections = MAX_IPC_LOCAL_CONNECTION);
    ~Server(){if(_activeConnections < _maxConnections) close(_sockId);}
    ConnHandler* Accept();
};

class Client : protected Socket {
    sockaddr* _addr;
    int _addrlen;
public:
    Client(std::string remoteAddr, int port);
    Client(std::string IpcSocketName);
    ~Client();
    ConnHandler* Connect();
private:
    /*
     * secondo la "Law of The Big Three" del c++, se si definisce uno tra:
     * distruttore, costruttore di copia o operatore di assegnazione,
     * probabilmente occorrera' definire anche i due restanti (e non usare
     * quelli di default cioe`!). Tuttavia in questo caso un Thread non puo`
     * essere assegnato o copiato, per cui il rispetto della suddetta regola
     * avviene rendendoli privati in modo da prevenirne un utilizzo
     * involontario!
     */
    Client(const Client &);//non ha senso ritornare o passare come parametro un Socket Client per valore
    Client & operator = (const Client &);//non ha senso assegnare un Socket Client per valore
};

#endif	/* _SOCKET_H */
