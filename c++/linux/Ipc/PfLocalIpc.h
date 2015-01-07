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

#ifndef _PFLOCALIPC_H
#define	_PFLOCALIPC_H

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

1) istanziare un oggetto PfLocalServer specificando la porta "P" di ascolto
2) istanziare un oggetto PfLocalClient specificando l'IP e la porta "P" su 
   cui e` in ascolto il server
3) per connettere client con server, chiamare Accept() sul server e 
   successivamente chiamare Connect() sul client. Entrambi i metodi 
   restituiscono (rispettivamente su server e client) un puntatore ad 
   un oggetto PfLocalConnHandler
4) per ricevere dati da una connessione occorre registrarsi sull'evento 
   di tipo RawByteBufferData il cui id viene restituito dal metodo 
   getRxDataLabel() di PfLocalConnHandler
5) per trasmettere dati occorre emettere un Evento di tipo RawByteBufferData 
   con label uguale a quello restituito dal metodo getTxDataLabel() di 
   ConnnHandler.

NOTA: Il PfLocalServer rimane in ascolto su una singola porta/IpcSocketName, una volta instaurata
      la connessione, il server non e` piu` in ascolto. Per implementare un server che
      gestisce piu` connessioni occorre implementare una classe che generi un nuovo server
      ad ogni richiesta di connessione su cui passa la richiesta spostandola su una nuova
      porta, per poi tornare in ascolto sulla porta originale per una nuova richiesta
      da gestire!

*/

namespace Z
{
//-------------------------------------------------------------------------------------------
class PfLocalConnHandler : public Thread {
    friend class PfLocalServer;
    friend class PfLocalClient;
    int _sockId;
    std::string _IpcSocketName;
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
    PfLocalConnHandler(int sockId, const std::string& IpcSocketName="");//PfLocalConnHandler non deve essere istanziabile, ma solo ottenibile effettuando una connessione.
public:
    ~PfLocalConnHandler();
    void Start();
    void Stop();
    void Join();
    std::string getTxDataLabel(){return "txDataEvent"+_sap;}//id dell'evento di richiesta trasmissione dati sul socket (evento ricevuto e gestito da PfLocalConnHandler)
    std::string getRxDataLabel(){return "rxDataEvent"+_sap;}//id dell'evento di notifica ricezione dati sul socket (evento emesso da PfLocalConnHandler)
};

class PfLocalServer {
    int _sockId;
    const std::string _IpcSocketName;
public:
    PfLocalServer(const std::string& IpcSocketName);
    ~PfLocalServer();
    PfLocalConnHandler* Accept();
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
    PfLocalServer(const PfLocalServer &);//non ha senso ritornare o passare come parametro un Socket PfLocalServer per valore
    PfLocalServer & operator = (const PfLocalServer &);//non ha senso assegnare un Socket PfLocalServer per valore
};

class PfLocalClient {
    int _sockId;
    sockaddr* _addr;
    int _addrlen;
public:
    PfLocalClient(const std::string& IpcSocketName);
    ~PfLocalClient();
    PfLocalConnHandler* Connect();
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
    PfLocalClient(const PfLocalClient &);//non ha senso ritornare o passare come parametro un Socket PfLocalClient per valore
    PfLocalClient & operator = (const PfLocalClient &);//non ha senso assegnare un Socket PfLocalClient per valore
};
//-------------------------------------------------------------------------------------------
}//namespace Z
#endif	/* _PFLOCALIPC_H */
