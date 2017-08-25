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
import java.util.*;
import java.nio.ByteBuffer;

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
          StopAndWaitOverUdp emits a SawUdpPktError event

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
          un evento SawUdpPktError

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
             - riceve normali pacchetti udp (label dgramRxDataEvent) tramite ctxt.srv (socket
               udp) ai quali toglie l'header prima di passare il contenuto all'utilizzatore
               come pacchetto udp (UdpPkt) con label sawDgramRxDataEvent.
             - riceve normali pacchetti udp (label sawDgramTxDataEvent) dall'utilizzatore
               ai quali aggiunge l'header prima di trasmetterli come pacchetto UdpPkt con label
               dgramTxDataEvent verso il peer tramite ctxt.srv.
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

//------------------------------------------------------------------------------
// event for internal use for self-notifying of return in idle state
// MUST NOT BE USED EXTERNALLY!
// :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
// EVENTO INTERNO PER LA AUTONOTIFICA DI RIENTRO IN STATO IDLE
class idleAndReady extends Event {
    final static String idleAndReadyLabel = "idleAndReady";
    idleAndReady() {super(idleAndReadyLabel);}//non va istanziato al di fuori del package!
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//STATI
//------------------------------------------------------------------------------
//context
class StopAndWaitOverUdpStateContext {
    ArrayDeque<UdpPkt> txQueue = new ArrayDeque<UdpPkt>();
    Udp srv;
    Timer ackTimer;
    int retryN;
    Byte messageId;
    enum MessageType {
        NONE(0), DATA(132), ACK(231);//arbitrary choice....
        private final int messageType;
        MessageType(int messageTypeCode) {this.messageType = messageTypeCode;}
        byte toByte(){return ((byte)messageType);}
    }
    String rxDataLabel4user;//label dell'evento UdpPkt con i dati ricevuti per la notifica verso utilizzatore di StopAndWaitOverUdp
}
//------------------------------------------------------------------------------
class StopAndWaitOverUdpState {
    StopAndWaitOverUdpState unhandledEvent(String event)
    {
        LOG.ziblog(LOG.LogLev.ERR, "UNHANDLED EVENT! %s while in state %s", event, this.getClass().getSimpleName()); 
        return this;
    }
    StopAndWaitOverUdpState sawDgramTxDataEventHndl(StopAndWaitOverUdpStateContext ctxt, UdpPkt pkt)
    {
        String currentState=this.getClass().getSimpleName();
        LOG.ziblog(LOG.LogLev.INF, "%s", (currentState+" - sawDgramTxDataEvent"));
        UdpPkt tx = new UdpPkt(pkt.label(), pkt.udpPort(), pkt.ipAddr(), pkt.buf());//copia di pkt
        ctxt.txQueue.addLast(tx);
        LOG.ziblog(LOG.LogLev.INF, "Message enqueued");
        LOG.ziblog(LOG.LogLev.INF, "%s ==> %s",currentState, currentState);
        return this;
    }
    StopAndWaitOverUdpState messageInTxQueueHndl(StopAndWaitOverUdpStateContext ctxt)
    {
        return unhandledEvent("messageInTxQueueHndl");
    }
    StopAndWaitOverUdpState dgramRxDataEventHndl(StopAndWaitOverUdpStateContext ctxt, UdpPkt pkt)
    {
        return unhandledEvent("dgramRxDataEventHndl");
    }
    StopAndWaitOverUdpState ackTimeoutHndl(StopAndWaitOverUdpStateContext ctxt)
    {
        return unhandledEvent("ackTimeoutHndl");
    }
}
//------------------------------------------------------------------------------
//stato IDLE
class StopAndWaitOverUdpIdleState extends StopAndWaitOverUdpState {
    StopAndWaitOverUdpState messageInTxQueueHndl(StopAndWaitOverUdpStateContext ctxt)
    {
        LOG.ziblog(LOG.LogLev.INF, "IDLE - messageInTxQueue");
        UdpPkt txData = ctxt.txQueue.peekFirst();//il pop lo faccio solo dopo l'ack
        ArrayList<Byte> sawTxData = new ArrayList<Byte>();
        StopAndWaitOverUdpStateContext.MessageType type = StopAndWaitOverUdpStateContext.MessageType.DATA;
        sawTxData.add(type.toByte());
        sawTxData.add(++(ctxt.messageId));
        ByteBuffer txDataBB = txData.buf();
        for(int i=0; i<txData.len(); i++) sawTxData.add(txDataBB.get(i));
        (new UdpPkt(ctxt.srv.getTxDataLabel(), txData.udpPort(), txData.ipAddr(), sawTxData)).emitEvent();
        LOG.ziblog(LOG.LogLev.INF, "tx first message in queue, messageId=%d", ctxt.messageId);
        ctxt.ackTimer.Start();
        LOG.ziblog(LOG.LogLev.INF, "IDLE ==> WAIT4ACK");
        return new StopAndWaitOverUdpWait4AckState();
    }
    StopAndWaitOverUdpState dgramRxDataEventHndl(StopAndWaitOverUdpStateContext ctxt, UdpPkt pkt)
    {
        LOG.ziblog(LOG.LogLev.INF, "IDLE - dgramRxDataEvent");
        ArrayList<Byte> rxData = new ArrayList<Byte>();
        ByteBuffer rxDataBB = pkt.buf();
        for(int i=0; i<pkt.len(); i++) rxData.add(rxDataBB.get(i));
        Byte messageType = rxData.remove(0);
        Byte messageId = rxData.remove(0);
        if(messageType != StopAndWaitOverUdpStateContext.MessageType.DATA.toByte()) {
            LOG.ziblog(LOG.LogLev.ERR, "unexpected udp packet");
            LOG.ziblog(LOG.LogLev.INF, "IDLE ==> IDLE");
            return this;
        }
        LOG.ziblog(LOG.LogLev.INF, "rx messageId=%d", messageId);
        ArrayList<Byte> ackToPeer = new ArrayList<Byte>();
        ackToPeer.add(StopAndWaitOverUdpStateContext.MessageType.ACK.toByte());
        ackToPeer.add(messageId);
        (new UdpPkt(ctxt.srv.getTxDataLabel(), pkt.udpPort(), pkt.ipAddr(), ackToPeer)).emitEvent();//ack al peer
        LOG.ziblog(LOG.LogLev.INF, "tx ACK to peer");
        (new UdpPkt(ctxt.rxDataLabel4user, pkt.udpPort(), pkt.ipAddr(), rxData)).emitEvent();//rxData x user
        LOG.ziblog(LOG.LogLev.INF, "emit data event 4 user");
        LOG.ziblog(LOG.LogLev.INF, "IDLE ==> IDLE");
        return this;
    }
    StopAndWaitOverUdpIdleState()
    {
        (new idleAndReady()).emitEvent();
    }
}
//------------------------------------------------------------------------------
//stato WAIT4ACK
class StopAndWaitOverUdpWait4AckState extends StopAndWaitOverUdpState {
    StopAndWaitOverUdpState messageInTxQueueHndl(StopAndWaitOverUdpStateContext ctxt)
    {
        LOG.ziblog(LOG.LogLev.INF, "WAIT4ACK - messageInTxQueueHndl");
        LOG.ziblog(LOG.LogLev.INF, "nothing to be done... wait 4 beeing in idle state");
        LOG.ziblog(LOG.LogLev.INF, "WAIT4ACK ==> WAIT4ACK");
        return this;
    }
    StopAndWaitOverUdpState ackTimeoutHndl(StopAndWaitOverUdpStateContext ctxt)
    {
        LOG.ziblog(LOG.LogLev.INF, "WAIT4ACK - ackTimeout");
        if(++(ctxt.retryN)<3) {
            LOG.ziblog(LOG.LogLev.INF, "RETRY N %d", ctxt.retryN);
            UdpPkt txData = ctxt.txQueue.peekFirst();//il pop lo faccio solo dopo l'ack
            ArrayList<Byte> sawTxData = new ArrayList<Byte>();
            sawTxData.add(StopAndWaitOverUdpStateContext.MessageType.DATA.toByte());
            sawTxData.add(++(ctxt.messageId));//same messageId: it's a retransmission!
                                             //messageId e` lo stesso: e` una ritrasmissione!
            ByteBuffer txDataBB = txData.buf();
            for(int i=0; i<txData.len(); i++) sawTxData.add(txDataBB.get(i));
            (new UdpPkt(ctxt.srv.getTxDataLabel(), txData.udpPort(), txData.ipAddr(), sawTxData)).emitEvent();
            ctxt.ackTimer.Start();
            LOG.ziblog(LOG.LogLev.INF, "WAIT4ACK ==> WAIT4ACK");
            return this;
        } else {//no retries available
                //tentativi esauriti
            ctxt.retryN=0;
            UdpPkt txData = ctxt.txQueue.pollFirst();//trash the datagram for which the transmission has failed
                                                     //prendo dal pacchetto fallito i dati per l'emissione dell'errore
            int udpPort=txData.udpPort();
            String destIpAddr=txData.ipAddr();
            LOG.ziblog(LOG.LogLev.INF, "first message removed from txQueue");
            LOG.ziblog(LOG.LogLev.ERR, "TX ERROR");
            (new SawUdpPktError(udpPort, destIpAddr)).emitEvent();
            LOG.ziblog(LOG.LogLev.INF, "emit error event 4 user");
            LOG.ziblog(LOG.LogLev.INF, "WAIT4ACK ==> IDLE");
            return new StopAndWaitOverUdpIdleState();
        }
    }
    StopAndWaitOverUdpState dgramRxDataEventHndl(StopAndWaitOverUdpStateContext ctxt, UdpPkt pkt)
    {
        LOG.ziblog(LOG.LogLev.INF, "WAIT4ACK - dgramRxDataEvent");
        ArrayList<Byte> rxData = new ArrayList<Byte>();
        ByteBuffer rxDataBB = pkt.buf();
        for(int i=0; i<pkt.len(); i++) rxData.add(rxDataBB.get(i));
        Byte messageType = rxData.remove(0);
        Byte messageId = rxData.remove(0);
        if(messageType == StopAndWaitOverUdpStateContext.MessageType.DATA.toByte()) {
            LOG.ziblog(LOG.LogLev.INF, "rx DATA");
            ArrayList<Byte> ackToPeer = new ArrayList<Byte>();
            ackToPeer.add(StopAndWaitOverUdpStateContext.MessageType.ACK.toByte());
            ackToPeer.add(messageId);
            (new UdpPkt(ctxt.srv.getTxDataLabel(), pkt.udpPort(), pkt.ipAddr(), ackToPeer)).emitEvent();//ack al peer
            (new UdpPkt(ctxt.rxDataLabel4user, pkt.udpPort(), pkt.ipAddr(), rxData)).emitEvent();//rxData x user
            LOG.ziblog(LOG.LogLev.INF, "WAIT4ACK ==> WAIT4ACK");
            return this;
        } else if(messageType == StopAndWaitOverUdpStateContext.MessageType.ACK.toByte()) {
            ctxt.ackTimer.Stop();
            LOG.ziblog(LOG.LogLev.INF, "rx ACK");
            if(messageId!=ctxt.messageId) {
                LOG.ziblog(LOG.LogLev.ERR, "ack messageId mismatch");
                if(++(ctxt.retryN)<3) {
                    LOG.ziblog(LOG.LogLev.INF, "RETRY N %d", ctxt.retryN);
                    UdpPkt txData = ctxt.txQueue.peekFirst();//il pop lo faccio solo dopo l'ack
                    ArrayList<Byte> sawTxData = new ArrayList<Byte>();
                    sawTxData.add(StopAndWaitOverUdpStateContext.MessageType.DATA.toByte());
                    sawTxData.add(++(ctxt.messageId));//same messageId: it's a retransmission!
                                                     //messageId e` lo stesso: e` una ritrasmissione!
                    ByteBuffer txDataBB = txData.buf();
                    for(int i=0; i<txData.len(); i++) sawTxData.add(txDataBB.get(i));
                    (new UdpPkt(ctxt.srv.getTxDataLabel(), txData.udpPort(), txData.ipAddr(), sawTxData)).emitEvent();
                    ctxt.ackTimer.Start();
                    LOG.ziblog(LOG.LogLev.INF, "WAIT4ACK ==> WAIT4ACK");
                    return this;
                } else {//no retries available
                        //tentativi esauriti
                    ctxt.retryN=0;
                    UdpPkt txData = ctxt.txQueue.pollFirst();//rimuovo il pacchetto fallito dalla coda emetto errore
                    int udpPort=txData.udpPort();
                    String destIpAddr=txData.ipAddr();
                    LOG.ziblog(LOG.LogLev.ERR, "TX ERROR");
                    (new SawUdpPktError(udpPort, destIpAddr)).emitEvent();
                    LOG.ziblog(LOG.LogLev.INF, "first message removed from txQueue");
                    LOG.ziblog(LOG.LogLev.INF, "WAIT4ACK ==> IDLE");
                    return new StopAndWaitOverUdpIdleState();
                }
            } else {//tx successful!!!
                ctxt.retryN=0;
                ctxt.txQueue.pollFirst();
                LOG.ziblog(LOG.LogLev.INF, "first message removed from txQueue");
                LOG.ziblog(LOG.LogLev.INF, "WAIT4ACK ==> IDLE");
                return new StopAndWaitOverUdpIdleState();
            }
        } else {
            LOG.ziblog(LOG.LogLev.ERR, "UNKNOWN MESSAGE TYPE %d", messageType);
            LOG.ziblog(LOG.LogLev.INF, "WAIT4ACK ==> WAIT4ACK");
            return this;
        }
    }
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//THREAD
public class StopAndWaitOverUdp extends Thread {
    StopAndWaitOverUdpStateContext ctxt;
    StopAndWaitOverUdpState state;
    void changeState(StopAndWaitOverUdpState newState)
    {
        if(newState!=state) {
            if(newState != null) state=newState;
            else LOG.ziblog(LOG.LogLev.ERR, "newState=null");
        }
    }
    public void run()
    {
        boolean exit=false;
        while(!exit){
            Event ev = pullOut();
            if(ev!=null) {
                LOG.ziblog(LOG.LogLev.INF, "received event with label %s", ev.label());
                LOG.ziblog(LOG.LogLev.DBG, "current TxQueue size=%d", ctxt.txQueue.size());
                if(ev instanceof StopThreadEvent) exit=true;
                else if(ev instanceof UdpPkt) {
                    UdpPkt pkt = (UdpPkt)ev;       
                    if(pkt.label().equals(ctxt.srv.getRxDataLabel())) changeState(state.dgramRxDataEventHndl(ctxt, pkt));
                    else {
                        changeState(state.sawDgramTxDataEventHndl(ctxt, pkt));//non puo` che essere label=this->getTxDataLabel()
                        if(!ctxt.txQueue.isEmpty()) changeState(state.messageInTxQueueHndl(ctxt));
                    }
                } else if(ev instanceof TimeoutEvent) changeState(state.ackTimeoutHndl(ctxt));
                else if(ev instanceof idleAndReady) {
                    if(!ctxt.txQueue.isEmpty()) changeState(state.messageInTxQueueHndl(ctxt));
                } else LOG.ziblog(LOG.LogLev.ERR, "UNHANDLED EVENT");
            }
        }
    }
    String _sap;
    public StopAndWaitOverUdp(int udpPort)
    {
        ctxt = new StopAndWaitOverUdpStateContext();
	    ctxt.srv = new Udp(udpPort);
        ctxt.ackTimer = new Timer("StopAndWaitOverUdpAckTimer", 5000);//ack timer = 5 sec
	    ctxt.messageId=0;
	    ctxt.retryN=0;
        state = new StopAndWaitOverUdpIdleState();
        _sap=Integer.toString(udpPort);
        ctxt.rxDataLabel4user=getRxDataLabel();
	    register2Label(getTxDataLabel());
	    register2Label(ctxt.srv.getRxDataLabel());
        register2Label(ctxt.ackTimer.getTimerId());
        register2Label(idleAndReady.idleAndReadyLabel);
    }
    public void Start()
    {
        ctxt.srv.Start();
        super.Start();//avvio di questo thread
    }
    public void Stop()
    {
        ctxt.ackTimer.Stop();
        ctxt.srv.Stop();
        super.Stop();
    }
    public String getTxDataLabel(){return "sawDgramTxDataEvent"+_sap;}//label for datagram tx request event
                                                               //id dell'evento di richiesta trasmissione datagram
    public String getRxDataLabel(){return "sawDgramRxDataEvent"+_sap;}//label for datagram rx event
                                                               //id dell'evento di notifica ricezione datagram
}

