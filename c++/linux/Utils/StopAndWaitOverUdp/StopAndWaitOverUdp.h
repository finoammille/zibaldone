/*
 *
 * zibaldone - a C++/Java library for Thread, Timers and other Stuff
 *
 * http://sourceforge.net/projects/zibaldone/
 *
 * version 3.1.2, August 29th, 2015
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
//------------------------------------------------------------------------------
// event for internal use for self-notifying of return in idle state
// MUST NOT BE USED EXTERNALLY!
struct idleAndReady : public Event {
    idleAndReady():Event("idleAndReady")  {}
    static std::string idleAndReadyLabel(){return "idleAndReady";}
protected:
    Event* clone() const {return new idleAndReady(*this);}
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
//IDLE state
class StopAndWaitOverUdpIdleState : public StopAndWaitOverUdpState {
    virtual StopAndWaitOverUdpState* messageInTxQueueHndl(StopAndWaitOverUdpStateContext*);
    virtual StopAndWaitOverUdpState* dgramRxDataEventHndl(StopAndWaitOverUdpStateContext*, const UdpPkt* const);
public:
    StopAndWaitOverUdpIdleState(){(idleAndReady()).emitEvent();}
};
//------------------------------------------------------------------------------
//WAIT4ACK state
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
