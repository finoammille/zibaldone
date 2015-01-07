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
#define	_TPCIPC_H

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
#include <sys/eventfd.h>

#include "Thread.h"
#include "Log.h"

/*
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

NOTA: Il TcpServer rimane in ascolto su una singola porta, una volta instaurata
      la connessione, il TcpServer non e` piu` in ascolto. Per implementare un TcpServer che
      gestisce piu` connessioni occorre implementare una classe che generi un nuovo TcpServer
      ad ogni richiesta di connessione su cui passa la richiesta spostandola su una nuova
      porta, per poi tornare in ascolto sulla porta originale per una nuova richiesta
      da gestire!

*/

namespace Z
{
//------------------------------------------------------------------------------
class TcpConnHandler : public Thread {
    friend class TcpServer;
    friend class TcpClient;
    int _sockId;
    std::string _sap;//serve per taggare univocamente gli eventi relativi ad uno specifico socket
    bool exit;
    void run();//ciclo del thread che gestisce la scrittura
    class Reader : public Thread {
        int efd; //Event file descriptor
        bool exit;
        int _sockId;
        void run();//ciclo del thread per la lettura
    public:
        Reader(int);
		~Reader();
        void Start();
        void Stop();
    } reader;
    TcpConnHandler(int sockId);//TcpConnHandler non deve essere istanziabile, ma solo ottenibile effettuando una connessione.
public:
    ~TcpConnHandler();
    void Start();
    void Stop();
    void Join();
    std::string getTxDataLabel(){return "txDataEvent"+_sap;}//id dell'evento di richiesta trasmissione dati sul socket (evento ricevuto e gestito da TcpConnHandler)
    std::string getRxDataLabel(){return "rxDataEvent"+_sap;}//id dell'evento di notifica ricezione dati sul socket (evento emesso da TcpConnHandler)
};

class TcpServer {
    int _sockId;
public:
    TcpServer(const int port);
    ~TcpServer();
    TcpConnHandler* Accept();
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
    TcpServer(const TcpServer &);//non ha senso ritornare o passare come parametro un TcpServer per valore
    TcpServer & operator = (const TcpServer &);//non ha senso assegnare un TcpServer per valore
};

class TcpClient {
    int _sockId;
    sockaddr* _addr;
    int _addrlen;
public:
    TcpClient(std::string& remoteAddr, int port);
    ~TcpClient();
    TcpConnHandler* Connect();
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
    TcpClient(const TcpClient &);//non ha senso ritornare o passare come parametro un TcpClient per valore
    TcpClient & operator = (const TcpClient &);//non ha senso assegnare un TcpClient per valore
};
//------------------------------------------------------------------------------
}//namespace Z
#endif	/* _TPCIPC_H */
