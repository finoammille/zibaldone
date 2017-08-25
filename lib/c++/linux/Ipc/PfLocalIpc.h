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

1) instantiate a PfLocalServer object specifying the listening port "P"
2) instantiate a PfLocalClient object specifying IP and port P of
   listening server
3) to connect client and server, you have to call Accept() on server side
   and then you have to call Connect() on client side. Both methods return
   (respectively on server side and client side) a pointer to an object of
   type PfLocalConnHandler
4) to receive data from the connection you have to register on event of
   type RawByteBufferData whose label is returned by
   PfLocalConnHandler::getRxDataLabel()
5) to transmit data over the connection you have to emit an Event of type
   RawByteBufferData with label set to the label returned by the
   ConnHandler::getTxDataLabel() method

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

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

TODO: Il PfLocalServer rimane in ascolto su una singola porta/IpcSocketName, una volta instaurata
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
     * the well known big 3 rule of C++ says:
     * "if your class needs to define the destructor or the copy constructor or the
     * copy assigment operator then it should probably explicitly define all three"
     * In our case since a Thread cannot be assigned or copied, we declare the
     * copy constructor and the assignment operator as private. This way we can
     * prevent and easily detect any inadverted abuse
     * :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
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
     * the well known big 3 rule of C++ says:
     * "if your class needs to define the destructor or the copy constructor or the
     * copy assigment operator then it should probably explicitly define all three"
     * In our case since a Thread cannot be assigned or copied, we declare the
     * copy constructor and the assignment operator as private. This way we can
     * prevent and easily detect any inadverted abuse
     * :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
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
