/*
 *
 * zibaldone - a C++ library for Thread, Timers and other Stuff
 * http://sourceforge.net/projects/zibaldone/
 *
 * Copyright (C) 2012  ilant (ilant@users.sourceforge.net)
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
*/
//------------------------------------------------------------------------------
namespace Z
{
//------------------------------------------------------------------------------
// TX ERROR EVENT
struct sawUdpPktError : public Event {
    int udpPort;//destination udp port of the not received datagram
    std::string destIpAddr;//destination ip address of the not received datagram
    sawUdpPktError(int udpPort, const std::string& destIpAddr):Event("sawUdpPktError"),
                                                               udpPort(udpPort), destIpAddr(destIpAddr) {}
    static std::string sawUdpPktErrorLabel(){return "sawUdpPktError";}
protected:
    Event* clone() const {return new sawUdpPktError(*this);}
};
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//STATE
//------------------------------------------------------------------------------
//context
struct StopAndWaitOverUdpStateContext {
    std::deque<UdpPkt*>  txQueue;
    Udp* srv;
    Timer* ackTimer;
    int retryN;
    unsigned char messageId;
    enum MessageType {NONE, DATA=132, ACK=231};//arbitrary choice....
    std::string rxDataLabel4user;
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
    std::string getRxDataLabel(){return "sawDgramRxDataEvent"+_sap;}//label for datagram rx event
};
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
}//namespace Z
#endif	/* _STOPANDWAITOVERUDP_H */
