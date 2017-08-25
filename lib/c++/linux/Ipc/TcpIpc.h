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

#include <linux/version.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,22)
#include <sys/eventfd.h>
#else
#include <sys/time.h>
#endif

#include "Thread.h"
#include "Log.h"

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
//------------------------------------------------------------------------------
class TcpConnHandler : public Thread {
    friend class TcpServer;
    friend class TcpClient;
    int _sockId;
    std::string _sap;//serve per taggare univocamente gli eventi relativi ad uno specifico socket
    bool exit;
    void run();//ciclo del thread che gestisce la scrittura
    class Reader : public Thread {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,22)
        int efd; //Event file descriptor
#endif
        bool exit;
        int _sockId;
        void run();//ciclo del thread per la lettura
    public:
        Reader(int);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,22)
		~Reader();
#endif
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
     * the well known big 3 rule of C++ says:
     * "if your class needs to define the destructor or the copy constructor or the
     * copy assigment operator then it should probably explicitly define all three"
     * In our case since a Thread cannot be assigned or copied, we declare the
     * copy constructor and the assignment operator as private. This way we can
     * prevent and easily detect any inadverted abuse
     * ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
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
     * the well known big 3 rule of C++ says:
     * "if your class needs to define the destructor or the copy constructor or the
     * copy assigment operator then it should probably explicitly define all three"
     * In our case since a Thread cannot be assigned or copied, we declare the
     * copy constructor and the assignment operator as private. This way we can
     * prevent and easily detect any inadverted abuse
     * ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
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
