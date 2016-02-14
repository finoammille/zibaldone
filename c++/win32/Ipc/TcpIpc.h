/*
 *
 * zibaldone - a C++/Java library for Thread, Timers and other Stuff
 *
 * http://sourceforge.net/projects/zibaldone/
 *
 * version 3.2.0, February 14th, 2016
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

#ifndef _TCPIPC_H
#define _TCPIPC_H

#include <winsock2.h>
#include <ws2tcpip.h>
#include "Log.h"
#include "Thread.h"
#include "Events.h"

/*
Use:

1) instantiate a TcpServer object specifying the listening port "P"
2) instantiate a TcpClient object specifying IP and port P of
   listening server
3) to connect TcpClient with TcpServer, you have to call Accept() on server
   side (TcpServer) and then you have to call Connect() on client side
   (TcpClient). Both methods return (respectively on server side and
   client side) a pointer to an object of type TcpConnHandler
4) to receive data from the connection you have to register on event of
   type RawByteBufferData whose label is returned by
   TcpConnHandler::getRxDataLabel()
5) to transmit data over the connection you have to emit an Event of type
   RawByteBufferData with label set to the label returned by the
   ConnHandler::getTxDataLabel() method

::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

Utilizzo:

1) istanziare un oggetto TcpServer specificando la porta "P" di ascolto
2) istanziare un oggetto TcpClient specificando l'IP e la porta "P" su
   cui e` in ascolto il TcpServer
3) per connettere TcpClient con TcpServer, chiamare Accept() sul TcpServer e
   successivamente chiamare Connect() sul TcpClient. Entrambi i metodi
   restituiscono (rispettivamente su TcpServer e TcpClient) un puntatore ad
   un oggetto TcpConnHandler
4) per ricevere dati da una connessione occorre registrarsi sull'evento
   di tipo RawByteBufferData il cui id viene restituito dal metodo
   getRxDataLabel() di TcpConnHandler
5) per trasmettere dati occorre emettere un Evento di tipo RawByteBufferData
   con label uguale a quello restituito dal metodo getTxDataLabel() di
   ConnnHandler.

TODO: Il TcpServer rimane in ascolto su una singola porta, una volta instaurata
      la connessione, il TcpServer non e` piu` in ascolto. Per implementare un TcpServer che
      gestisce piu` connessioni occorre implementare una classe che generi un nuovo TcpServer
      ad ogni richiesta di connessione su cui passa la richiesta spostandola su una nuova
      porta, per poi tornare in ascolto sulla porta originale per una nuova richiesta
      da gestire!

*/

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
