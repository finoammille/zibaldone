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

package Z;

import java.net.*;
import java.io.IOException;
import java.nio.channels.IllegalBlockingModeException;

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
      
      In realta` credo che nel caso java sia invece gia` cosi` dato che il socket
      torna disponibile dopo la accept.... Verificarlo!

*/

public class TcpServer {
    ServerSocket sock;
    public TcpServer(int port)
    {
        try {
            sock=new ServerSocket(port);
        } catch(IOException ex) {
            LOG.ziblog(LOG.LogLev.ERR, "TcpServer contructor - IOException(%s)", ex.getMessage());
        } catch(SecurityException ex) {
            LOG.ziblog(LOG.LogLev.ERR, "TcpServer contructor - SecurityException(%s)", ex.getMessage());
        } catch(IllegalArgumentException ex) {
            LOG.ziblog(LOG.LogLev.ERR, "TcpServer contructor - IllegalArgumentException(%s)", ex.getMessage());
        }
    }
    public TcpConnHandler Accept()
    {
        Socket connSock;
        try {
            connSock=sock.accept();
        } catch(IOException ex) {
            LOG.ziblog(LOG.LogLev.ERR, "Accept - IOException(%s)", ex.getMessage());
            return null;
        } catch(SecurityException ex) {
            LOG.ziblog(LOG.LogLev.ERR, "Accept - SecurityException(%s)", ex.getMessage());
            return null;
        } catch(IllegalBlockingModeException ex) {
            LOG.ziblog(LOG.LogLev.ERR, "Accept - IllegalBlockingModeException(%s)", ex.getMessage());
            return null;
        }
        return new TcpConnHandler(connSock);
    }
}
