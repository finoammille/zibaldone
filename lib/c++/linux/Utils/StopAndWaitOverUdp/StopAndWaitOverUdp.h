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

#ifndef _STOPANDWAITOVERUDP_H
#define	_STOPANDWAITOVERUDP_H


#include "UdpIpc.h"
#include "Timer.h"
#include <typeinfo>

//------------------------------------------------------------------------------
/*
    StopAndWaitOverUdp implements the Stop And Wait protocol over a UDP protocol
    communication (connectionless)

    Use:

    => instantiate a StopAndWaitOverUdp object specifying the listening udp port

    => from the user point of view, StopAndWaitOverUdp acts as a normal Udp with
       the addition of an event that warns about the occurrence of a transmission
       error. The retransmission mechanism is managed by StopAndWaitOverUdp and
       it's transparent for the user.

       a) to receive data, the user has to register to a UdpPkt event with the
          label exactly equal to that returned by getRxDataLabel().

       b) to transmit data, the user has to emit an event of type UdpPks with
          label equal to that returned by getTxDataLabel().

       c) if a transmission request fails (that is StopAndWaitOverUdp runs out
          of the retransmission retries without receiving the ack from peer)
          StopAndWaitOverUdp emits a sawUdpPktError event

    Note: the transmission queue manages multiple transmission requests in a
          asynchronous fashion. A StopAndWaitOverUdp user does not have to stand
          and wait for his transmission request result: if an error occurs he will
          be notified by the sawUdpPkdError (he obviously has to register to this
          event). This way the requests from various users are queued and orderly
          served whenever the previous one has successfully transmitted or it run
          out of retries (tx error)

    ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

    La classe StopAndWaitOverUdp implementa il protocollo Stop And Wait su
    una comunicazione UDP (connectionless).

    Utilizzo:

    => istanziare StopAndWaitOverUdp specificando la porta Udp su cui il thread e`
	   in ascolto come nel caso dell'Udp l'indirizzo ip non viene specificato per-
	   che` e` sottointeso che sia localhost (non posso ricevere pacchetti destina-
	   ti ad un ip diverso!)

       N.B.: ricordarsi di far partire il thread con Start come al solito!

    => dal punto di vista dell'utilizzatore, StopAndWaitOverUdp si comporta come un
	   normale Udp con l'aggiunta di un evento di notifica di errore in casi di fal-
	   limento della richiesta di trasmissione

       a) per ricevere dati occorre registrarsi sull'evento di tipo UdpPkt la cui
          label viene restituita dal metodo getRxDataLabel().

       b) per trasmettere dati occorre emettere un Evento di tipo UdpPkt la cui
          label viene restitita dal metodo getTxDataLabel().

       c) se una richiesta di trasmissione fallisce (ovvero il thread StopAndWaitOverUdp
          esaurisce i tentativi senza ricevere l'acknowledge dal peer) viene emesso
          un evento sawUdpPktError

    Note: 1) L'UDP fa il controllo di errore tramite una checksum ma non gestisce un
             eventuale errore. Questa implementazione fornisce la gestione dell' errore
             forte del fatto che il meccanismo di individuazione degli errori tramite
             checksum e` implementato da UDP. In caso di errore sulla checksum UDP
             semplicemente scarta il pacchetto. Tramite lo stop and wait e` possibile
             gestire una tale eventualita` tramite ritrasmissione.
          2) gli eventi UdpPkt con label sawDgramTxDataEvent e sawDgramRxDataEvent
             sono normalissimi pacchetti UDP.
             Il thread StopAndWaitOverUdp prende i dati contenuti in un evento
             sawDgramTxDataEvent e li ri-impacchetta in un nuovo pacchetto udp a cui
             appende un header con il tipo di messaggio (dati o ack) e il numero di
             sequenza per l'implementazione dello stop and wait.
             Analogamente il thread StopAndWaitOverUdp prende i dati ricevuti in un
             pacchetto UdpPkt  (label=dgramRxDataEvent) e fa l'operazione inversa ovvero
             stabilisce se e` un ack o un pacchetto dati e nel secondo caso elimina
             l'header (messageType-messageId) e emette un nuovo evento sawDgramRxDataEvent
             contente i soli dati (evento su cui si e` registrato l'utilizzatore).
             Praticamente il thread StopAndWaitOverUdp tratta in ricezione e in trasmissione
             eventi UdpPkt (pacchetti UDP) e li distingue tramite la label:
             - riceve normali pacchetti udp (label dgramRxDataEvent) tramite ctxt->srv (socket
               udp) ai quali toglie l'header prima di passare il contenuto all'utilizzatore
               come pacchetto udp (UdpPkt) con label sawDgramRxDataEvent.
             - riceve normali pacchetti udp (label sawDgramTxDataEvent) dall'utilizzatore
               ai quali aggiunge l'header prima di trasmetterli come pacchetto UdpPkt con label
               dgramTxDataEvent verso il peer tramite ctxt->srv.
             In altre parole i pacchetti con label sawDgramTxDataEvent e sawDgramRxDataEvent
             sono pacchetti udp "normali", mentre i pacchetti con label dgramTxDataEvent e
             dgramRxDataEvent sono pacchetti con header per stop and wait.
          3) la coda di trasmissione permette l'accodamento di richieste di trasmissioni in
             modo asincrono: l'utilizzatore di StopAndWaitOverUdp non e` vincolato a restare
             li ad aspettare l'esito della sua richiesta di trasmissione: in caso di errore gli
             viene notificato l'evento TX ERROR in modo asincrono. In questo modo tutti gli
             utenti possono richiedere la trasmissione dei propri dati che vengono accodati
             e serviti in ordine di arrivo non appena viene trasmesso con successo il pacchetto
             precedente o si esaurisce il numero di tentativi (TX ERROR). Per implementare
             questo comportamento, viene utilizzato l'evento interno "IdleAndReady" che viene
             emesso quando si passa allo stato IDLE proveniendo da uno stato diverso da IDLE.
             Senza questo evento, se la richiesta di trasmissione viene fatta quando
             StopAndWaitOverUdp sta completando una trasmissione precedente (per esempio e`
             in attesa dell'ack) il pacchetto viene correttamente accodato (pullOut riceve
             l'evento) ma poi il thread torna nella pullOut e resta li sinchè non arriva
             un nuovo evento. Posto che l'unico ad accodare gli eventi nella coda di trasmissione
             e` StopAndWaitOverUdp stesso, per risolvere il problema è sufficiente permettere
             al thread di svegliarsi quando torna in idle da un altro stato (ovvero quando ha
             finito di fare quello che stava facendo e ha concluso il processo di trasmissione
             con successo o con errore: durante gli eventuali retry infatti il protocollo resta
             in wait4ack). Cosi` facendo, quando il protocollo torna in idle, si sveglia dalla
             pullOut, controlla se c'e` qualcosa in coda e se c'e` trasmette, se no non fa nulla.
             In questo modo ho la certezza di non bloccarmi con dei dati da trasmettere, dato che
             se dovesse arrivare una nuova richiesta, il thread si sveglia dalla pullOut
*/
//------------------------------------------------------------------------------
namespace Z
{
//------------------------------------------------------------------------------
// EVENTO TX ERROR
struct sawUdpPktError : public Event {
    int udpPort;//destination udp port of the not received datagram
                //porta udp del destinatario del messaggio non ricevuto
    std::string destIpAddr;//destination ip address of the not received datagram
                           //ip del destinatario del messaggio non ricevuto
    sawUdpPktError(int udpPort, const std::string& destIpAddr):Event("sawUdpPktError"),
                                                               udpPort(udpPort), destIpAddr(destIpAddr) {}
    static std::string sawUdpPktErrorLabel(){return "sawUdpPktError";}
protected:
    Event* clone() const {return new sawUdpPktError(*this);}
};
//------------------------------------------------------------------------------
// event for internal use for self-notifying of return in idle state
// MUST NOT BE USED EXTERNALLY!
// :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
// EVENTO INTERNO PER LA AUTONOTIFICA DI RIENTRO IN STATO IDLE
struct idleAndReady : public Event {
    idleAndReady():Event("idleAndReady")  {}
    static std::string idleAndReadyLabel(){return "idleAndReady";}
protected:
    Event* clone() const {return new idleAndReady(*this);}
};
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//STATI
//------------------------------------------------------------------------------
//context
struct StopAndWaitOverUdpStateContext {
    std::deque<UdpPkt*>  txQueue;
    Udp* srv;
    Timer* ackTimer;
    int retryN;
    unsigned char messageId;
    enum MessageType {NONE, DATA=132, ACK=231};//arbitrary choice....
                                               //scelta arbitraria....
    std::string rxDataLabel4user;//label dell'evento UdpPkt con i dati ricevuti per la notifica verso utilizzatore di StopAndWaitOverUdp
};
//------------------------------------------------------------------------------
class StopAndWaitOverUdpState {
    StopAndWaitOverUdpState* unhandledEvent(const std::string &event)
    {
        ziblog(LOG::ERR, "UNHANDLED EVENT! %s while in state %s", event.c_str(), typeid(*this).name());
        return this;
    }
public:
    virtual ~StopAndWaitOverUdpState(){}
    virtual StopAndWaitOverUdpState* sawDgramTxDataEventHndl(StopAndWaitOverUdpStateContext*, const UdpPkt* const);
    virtual StopAndWaitOverUdpState* messageInTxQueueHndl(StopAndWaitOverUdpStateContext*){return unhandledEvent("messageInTxQueueHndl");}
    virtual StopAndWaitOverUdpState* dgramRxDataEventHndl(StopAndWaitOverUdpStateContext*, const UdpPkt* const){return unhandledEvent("dgramRxDataEventHndl");}
    virtual StopAndWaitOverUdpState* ackTimeoutHndl(StopAndWaitOverUdpStateContext*){return unhandledEvent("ackTimeoutHndl");}
};
//------------------------------------------------------------------------------
//stato IDLE
class StopAndWaitOverUdpIdleState : public StopAndWaitOverUdpState {
    virtual StopAndWaitOverUdpState* messageInTxQueueHndl(StopAndWaitOverUdpStateContext*);
    virtual StopAndWaitOverUdpState* dgramRxDataEventHndl(StopAndWaitOverUdpStateContext*, const UdpPkt* const);
public:
    StopAndWaitOverUdpIdleState(){(idleAndReady()).emitEvent();}
};
//------------------------------------------------------------------------------
//stato WAIT4ACK
class StopAndWaitOverUdpWait4AckState : public StopAndWaitOverUdpState {
    virtual StopAndWaitOverUdpState* messageInTxQueueHndl(StopAndWaitOverUdpStateContext*);
    virtual StopAndWaitOverUdpState* ackTimeoutHndl(StopAndWaitOverUdpStateContext*);
    virtual StopAndWaitOverUdpState* dgramRxDataEventHndl(StopAndWaitOverUdpStateContext*, const UdpPkt* const);
};
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//THREAD
class StopAndWaitOverUdp : public Thread {
    StopAndWaitOverUdpStateContext *ctxt;
    StopAndWaitOverUdpState* state;
    void changeState(StopAndWaitOverUdpState* newState){if(newState!=state) delete state; state=newState;}
    void run();
    std::string _sap;
public:
    StopAndWaitOverUdp(int udpPort);
    ~StopAndWaitOverUdp();
    void Start();
    void Stop();
    std::string getTxDataLabel(){return "sawDgramTxDataEvent"+_sap;}//label for datagram tx request event
                                                                    //id dell'evento di richiesta trasmissione datagram
    std::string getRxDataLabel(){return "sawDgramRxDataEvent"+_sap;}//label for datagram rx event
                                                                    //id dell'evento di notifica ricezione datagram
};
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
}//namespace Z
#endif	/* _STOPANDWAITOVERUDP_H */
