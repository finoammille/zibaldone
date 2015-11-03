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

#include "StopAndWaitOverUdp.h"

namespace Z
{
//-------------------------------------------------------------------------------------------
StopAndWaitOverUdp::StopAndWaitOverUdp(int udpPort)
{
    ctxt = new StopAndWaitOverUdpStateContext();
	ctxt->srv = new Udp(udpPort);
    ctxt->ackTimer = new Timer("StopAndWaitOverUdpAckTimer", 5000);//ack timer = 5 sec
	ctxt->messageId=0;
	ctxt->retryN=0;
    state=new StopAndWaitOverUdpIdleState();
    std::stringstream sap;
    sap<<udpPort;
    _sap = sap.str();
    ctxt->rxDataLabel4user=getRxDataLabel();
	register2Label(getTxDataLabel());
	register2Label(ctxt->srv->getRxDataLabel());
    register2Label(ctxt->ackTimer->getTimerId());
    register2Label(idleAndReady::idleAndReadyLabel());
}
//------------------------------------------------------------------------------
StopAndWaitOverUdp::~StopAndWaitOverUdp()
{
    Stop();
    for (std::deque<UdpPkt*>::iterator it=ctxt->txQueue.begin(); it!=ctxt->txQueue.end(); ++it) {
        delete *it;
        *it=NULL;
    }
    ctxt->txQueue.clear();
    delete ctxt->srv;
    ctxt->srv=NULL;
    delete ctxt->ackTimer;
    ctxt->ackTimer=NULL;
    delete ctxt;
    ctxt=NULL;
    delete state;
    state=NULL;
}
//------------------------------------------------------------------------------
void StopAndWaitOverUdp::Start()
{
    ctxt->srv->Start();
    Thread::Start();
}
//------------------------------------------------------------------------------
void StopAndWaitOverUdp::Stop()
{
    ctxt->ackTimer->Stop();
    ctxt->srv->Stop();
    Thread::Stop();
}
//------------------------------------------------------------------------------
void StopAndWaitOverUdp::run()
{
    bool exit=false;
    Event *Ev = NULL;
    while(!exit){
        Ev = pullOut();
        if(Ev) {
            ziblog(LOG::INF, "received event with label %s", Ev->label().c_str());
            ziblog(LOG::DBG, "current TxQueue size=%zu", ctxt->txQueue.size());
            if(dynamic_cast<StopThreadEvent *>(Ev)) exit = true;
            else if(dynamic_cast<UdpPkt *>(Ev)) {
                UdpPkt* ev = (UdpPkt*)Ev;
                if(ev->label()==ctxt->srv->getRxDataLabel()) changeState(state->dgramRxDataEventHndl(ctxt, ev));
                else {
                    changeState(state->sawDgramTxDataEventHndl(ctxt, ev));
                    if(!ctxt->txQueue.empty()) changeState(state->messageInTxQueueHndl(ctxt));
                }
            } else if(dynamic_cast<TimeoutEvent *>(Ev)) changeState(state->ackTimeoutHndl(ctxt));
            else if(dynamic_cast<idleAndReady *>(Ev)) {
                if(!ctxt->txQueue.empty()) changeState(state->messageInTxQueueHndl(ctxt));
            } else ziblog(LOG::ERR, "UNHANDLED EVENT");
            delete Ev;
        }
    }
}
//==============================================================================
//STATE MANAGEMENT
//------------------------------------------------------------------------------
// FOR ALL STATE
StopAndWaitOverUdpState* StopAndWaitOverUdpState::sawDgramTxDataEventHndl(StopAndWaitOverUdpStateContext* ctxt, const UdpPkt* const pkt)
{
    std::string currentState=typeid(*this).name();
    ziblog(LOG::INF, "%s", (currentState+" - sawDgramTxDataEvent").c_str());
    UdpPkt* tx = new UdpPkt(*pkt);
    ctxt->txQueue.push_back(tx);
    ziblog(LOG::INF, "Message enqueued");
    ziblog(LOG::INF, "%s ==> %s",currentState.c_str(), currentState.c_str());
    return this;
}
//------------------------------------------------------------------------------
//IDLE STATE
StopAndWaitOverUdpState* StopAndWaitOverUdpIdleState::messageInTxQueueHndl(StopAndWaitOverUdpStateContext* ctxt)
{
    ziblog(LOG::INF, "IDLE - messageInTxQueue");
    UdpPkt* txData = ctxt->txQueue.front();
    std::vector<unsigned char> sawTxData;
    StopAndWaitOverUdpStateContext::MessageType type=StopAndWaitOverUdpStateContext::DATA;
    sawTxData.push_back((unsigned char) type);
    sawTxData.push_back((unsigned char) ++ctxt->messageId);
    for(int i=0; i<txData->len(); i++) sawTxData.push_back((txData->buf())[i]);
    (UdpPkt(ctxt->srv->getTxDataLabel(), txData->UdpPort(), txData->IpAddr(), sawTxData)).emitEvent();
    ziblog(LOG::INF, "tx first message in queue, messageId=%d", ctxt->messageId);
    ctxt->ackTimer->Start();
    ziblog(LOG::INF, "IDLE ==> WAIT4ACK");
    return new StopAndWaitOverUdpWait4AckState();
}

StopAndWaitOverUdpState* StopAndWaitOverUdpIdleState::dgramRxDataEventHndl(StopAndWaitOverUdpStateContext* ctxt, const UdpPkt* const pkt)
{
    ziblog(LOG::INF, "IDLE - dgramRxDataEvent");
    std::vector<unsigned char> rxData;
    for(int i=0; i<pkt->len(); i++) rxData.push_back((pkt->buf())[i]);
    unsigned char messageType = rxData[0];//front
    rxData.erase(rxData.begin());//pop_front
    unsigned char messageId = rxData[0];//front
    rxData.erase(rxData.begin());//pop_front
    if((StopAndWaitOverUdpStateContext::MessageType)messageType != StopAndWaitOverUdpStateContext::DATA) {
        ziblog(LOG::ERR, "unexpected udp packet");
        ziblog(LOG::INF, "IDLE ==> IDLE");
        return this;
    }
    ziblog(LOG::INF, "rx messageId=%d", messageId);
    std::vector<unsigned char> ackToPeer;
    StopAndWaitOverUdpStateContext::MessageType type=StopAndWaitOverUdpStateContext::ACK;
    ackToPeer.push_back((unsigned char) type);
    ackToPeer.push_back((unsigned char) messageId);
    (UdpPkt(ctxt->srv->getTxDataLabel(), pkt->UdpPort(), pkt->IpAddr(), ackToPeer)).emitEvent();//ack to peer
    ziblog(LOG::INF, "tx ACK to peer");
    (UdpPkt(ctxt->rxDataLabel4user, pkt->UdpPort(), pkt->IpAddr(), rxData)).emitEvent();//rxData for user
    ziblog(LOG::INF, "emit data event 4 user");
    ziblog(LOG::INF, "IDLE ==> IDLE");
    return this;
}
//------------------------------------------------------------------------------
//WAIT4ACK STATE
StopAndWaitOverUdpState* StopAndWaitOverUdpWait4AckState::messageInTxQueueHndl(StopAndWaitOverUdpStateContext* ctxt)
{
    ziblog(LOG::INF, "WAIT4ACK - messageInTxQueueHndl");
    ziblog(LOG::INF, "nothing to be done... wait 4 beeing in idle state");
    ziblog(LOG::INF, "WAIT4ACK ==> WAIT4ACK");
    return this;
}

StopAndWaitOverUdpState* StopAndWaitOverUdpWait4AckState::ackTimeoutHndl(StopAndWaitOverUdpStateContext* ctxt)
{
    ziblog(LOG::INF, "WAIT4ACK - ackTimeout");
    if(++ctxt->retryN<3) {
        ziblog(LOG::INF, "RETRY N %d", ctxt->retryN);
        UdpPkt* txData = ctxt->txQueue.front();
        std::vector<unsigned char> sawTxData;
        StopAndWaitOverUdpStateContext::MessageType type=StopAndWaitOverUdpStateContext::DATA;
        sawTxData.push_back((unsigned char) type);
        sawTxData.push_back((unsigned char) ctxt->messageId);//same messageId: it's a retransmission!
        for(int i=0; i<txData->len(); i++) sawTxData.push_back((txData->buf())[i]);
        (UdpPkt(ctxt->srv->getTxDataLabel(), txData->UdpPort(), txData->IpAddr(), sawTxData)).emitEvent();
        ctxt->ackTimer->Start();
        ziblog(LOG::INF, "WAIT4ACK ==> WAIT4ACK");
        return this;
    } else {//no retries available
        ctxt->retryN=0;
        UdpPkt* txData = ctxt->txQueue.front();
        int udpPort=txData->UdpPort();
        std::string destIpAddr=txData->IpAddr();
        ctxt->txQueue.pop_front();//trash the datagram for which the transmission has failed
        ziblog(LOG::INF, "first message removed from txQueue");
        ziblog(LOG::ERR, "TX ERROR");
        (sawUdpPktError(udpPort, destIpAddr)).emitEvent();
        ziblog(LOG::INF, "emit error event 4 user");
        ziblog(LOG::INF, "WAIT4ACK ==> IDLE");
        return new StopAndWaitOverUdpIdleState();
    }
}

StopAndWaitOverUdpState* StopAndWaitOverUdpWait4AckState::dgramRxDataEventHndl(StopAndWaitOverUdpStateContext* ctxt, const UdpPkt* const pkt)
{
    ziblog(LOG::INF, "WAIT4ACK - dgramRxDataEvent");
    std::vector<unsigned char> rxData;
    for(int i=0; i<pkt->len(); i++) rxData.push_back((pkt->buf())[i]);
    unsigned char messageType = rxData[0];//front
    rxData.erase(rxData.begin());//pop_front
    unsigned char messageId = rxData[0];//front
    rxData.erase(rxData.begin());//pop_front
    if((StopAndWaitOverUdpStateContext::MessageType)messageType == StopAndWaitOverUdpStateContext::DATA) {
        ziblog(LOG::INF, "rx DATA");
        std::vector<unsigned char> ackToPeer;
        StopAndWaitOverUdpStateContext::MessageType type=StopAndWaitOverUdpStateContext::ACK;
        ackToPeer.push_back((unsigned char) type);
        ackToPeer.push_back((unsigned char) messageId);
        (UdpPkt(ctxt->srv->getTxDataLabel(), pkt->UdpPort(), pkt->IpAddr(), ackToPeer)).emitEvent();//ack to peer
        (UdpPkt(ctxt->rxDataLabel4user, pkt->UdpPort(), pkt->IpAddr(), rxData)).emitEvent();//rxData for user
        ziblog(LOG::INF, "WAIT4ACK ==> WAIT4ACK");
        return this;
    } else if((StopAndWaitOverUdpStateContext::MessageType)messageType == StopAndWaitOverUdpStateContext::ACK) {
        ctxt->ackTimer->Stop();
        ziblog(LOG::INF, "rx ACK");
        if(messageId!=ctxt->messageId) {
            ziblog(LOG::ERR, "ack messageId mismatch");
            if(++ctxt->retryN<3) {
                ziblog(LOG::INF, "RETRY N %d", ctxt->retryN);
                UdpPkt* txData = ctxt->txQueue.front();
                std::vector<unsigned char> sawTxData;
                StopAndWaitOverUdpStateContext::MessageType type=StopAndWaitOverUdpStateContext::DATA;
                sawTxData.push_back((unsigned char) type);
                sawTxData.push_back((unsigned char) ctxt->messageId);//same messageId: it's a retransmission!
                for(int i=0; i<txData->len(); i++) sawTxData.push_back((txData->buf())[i]);
                (UdpPkt(ctxt->srv->getTxDataLabel(), txData->UdpPort(), txData->IpAddr(), sawTxData)).emitEvent();
                ctxt->ackTimer->Start();
                ziblog(LOG::INF, "WAIT4ACK ==> WAIT4ACK");
                return this;
            } else {//no retries available
                ctxt->retryN=0;
                UdpPkt* txData = ctxt->txQueue.front();
                int udpPort=txData->UdpPort();
                std::string destIpAddr=txData->IpAddr();
                ctxt->txQueue.pop_front();//trash the datagram for which the transmission has failed
                ziblog(LOG::ERR, "TX ERROR");
                (sawUdpPktError(udpPort, destIpAddr)).emitEvent();
                ziblog(LOG::INF, "first message removed from txQueue");
                ziblog(LOG::INF, "WAIT4ACK ==> IDLE");
                return new StopAndWaitOverUdpIdleState();
            }
        } else {//tx successful!!!
            ctxt->retryN=0;
            ctxt->txQueue.pop_front();
            ziblog(LOG::INF, "first message removed from txQueue");
            ziblog(LOG::INF, "WAIT4ACK ==> IDLE");
            return new StopAndWaitOverUdpIdleState();
        }
    } else {
        ziblog(LOG::ERR, "UNKNOWN MESSAGE TYPE %d", messageType);
        ziblog(LOG::INF, "WAIT4ACK ==> WAIT4ACK");
        return this;
    }
}
//-------------------------------------------------------------------------------------------
}//namespace Z
