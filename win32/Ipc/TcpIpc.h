/*
 *
 * zibaldone - a C++/Java library for Thread, Timers and other Stuff
 *
 * Copyright (C) 2012  Antonio Buccino
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 2.
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

#ifndef _TCPIPC_H
#define _TCPIPC_H

#include <winsock2.h>
#include <ws2tcpip.h>
#include "Log.h"
#include "Thread.h"
#include "Events.h"

namespace Z
{
//-------------------------------------------------------------------------------------------
class TcpConnHandler : public Thread {
    friend class TcpServer;
    friend class TcpClient;
    SOCKET _sockId;
    std::string _sap;//serve per taggare univocamente gli eventi relativi ad uno specifico socket
    bool exit;
    void run();
    class Reader : public Thread {
        bool exit;
        SOCKET _sockId;
        void run();//ciclo del thread per la lettura
    public:
        Reader(SOCKET);
        void Start();
        void Stop();
    } reader;
    TcpConnHandler(SOCKET sockId);//TcpConnHandler non deve essere istanziabile, ma solo ottenibile effettuando una connessione.
public:
    ~TcpConnHandler();
    void Start();
    void Stop();
    void Join();
    std::string getTxDataLabel(){return "txDataEvent"+_sap;}//id dell'evento di richiesta trasmissione dati sul socket (evento ricevuto e gestito da TcpConnHandler)
    std::string getRxDataLabel(){return "rxDataEvent"+_sap;}//id dell'evento di notifica ricezione dati sul socket (evento emesso da TcpConnHandler)
};

class TcpServer {
    SOCKET _sockId;
public:
    TcpServer(int port);
    TcpConnHandler* Accept();
};

class TcpClient {
    SOCKET _sockId;
    struct addrinfo *ai;
public:
    TcpClient(const std::string& remoteAddr, int port);
    TcpConnHandler* Connect();
};

//-------------------------------------------------------------------------------------------
}//namespace Z
#endif /* _TCPIPC_H */
