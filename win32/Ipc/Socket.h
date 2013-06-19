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
#define _SOCKET_H

#include <winsock2.h>
#include <ws2tcpip.h>
#include "Log.h"
#include "Thread.h"

namespace Z
{
//-------------------------------------------------------------------------------------------
class Socket {
protected:
    SOCKET _sockId;
    Socket();
};

class ConnHandler : public Thread {
    friend class Server;
    friend class Client;
    SOCKET _sockId;
    std::string _sap;//serve per taggare univocamente gli eventi relativi ad uno specifico socket
    bool exit;
    void run();
    class Reader : public Thread {
        const SOCKET& _sockId;
        void run();//ciclo del thread per la lettura
    public:
        Reader(const SOCKET&);
        void Stop();
    } reader;
    ConnHandler(SOCKET _sockId);//ConnHandler non deve essere istanziabile, ma solo ottenibile effettuando una connessione.
public:
    ~ConnHandler();
    void Start();
    void Stop();
    void Join();
    std::string getTxDataEventId(){return "txDataEvent"+_sap;}//id dell'evento di richiesta trasmissione dati sul socket (evento ricevuto e gestito da ConnHandler)
    std::string getRxDataEventId(){return "rxDataEvent"+_sap;}//id dell'evento di notifica ricezione dati sul socket (evento emesso da ConnHandler)
};

class Server : protected Socket {
public:
    Server(int port);
    ConnHandler* Accept();
};

class Client : protected Socket {
    struct addrinfo *ai;
public:
    Client(const std::string& remoteAddr, int port);
    ConnHandler* Connect();
};

//-------------------------------------------------------------------------------------------
}//namespace Z
#endif /* _SOCKET_H */
