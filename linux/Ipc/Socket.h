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

/*
Utilizzo:

1) istanziare un oggetto Server specificando la porta "P" di ascolto
2) istanziare un oggetto Client specificando l'IP e la porta "P" 
   su cui e` in ascolto il server
3) per connettere client con server, chiamare Accept() sul server, 
   e successivamente chiamare Connect() sul client. Entrambi i metodi restituiscono
   (rispettivamente su server e client) un puntatore ad un oggetto ConnHandler
4) per ricevere dati da una connessione occorre registrarsi sull'evento il cui id viene
   restituito dal metodo getRxDataEventId() di ConnHandler
5) per trasmettere dati occorre emettere un Evento con eventId uguale a quello
   restituito dal metodo getTxDataEventId() di ConnnHandler.
*/

namespace Z
{
//-------------------------------------------------------------------------------------------
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
    std::string txDataEventId;//id dell'evento di richiesta trasmissione dati sul socket (evento ricevuto e gestito da ConnHandler)
    std::string rxDataEventId;//id dell'evento di notifica ricezione dati sul socket (evento emesso da ConnHandler)
public:
    ~ConnHandler(){close(_sockId);}
    std::string getTxDataEventId()const{return txDataEventId;}
    std::string getRxDataEventId()const{return rxDataEventId;}
};

class Server : protected Socket {
    void bindAndListen(sockaddr *addr, int addrlen);//funzione di utilizzo interno.
public:
    Server(const int port);
    Server(const std::string& IpcSocketName);
    ~Server(){close(_sockId);}
    ConnHandler* Accept();
};

class Client : protected Socket {
    sockaddr* _addr;
    int _addrlen;
public:
    Client(const std::string& remoteAddr, int port);
    Client(const std::string& IpcSocketName);
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
//-------------------------------------------------------------------------------------------
}//namespace Z
#endif	/* _SOCKET_H */
