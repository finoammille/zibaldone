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

#ifndef _EVENT_H
#define	_EVENT_H

//-------------------------------------------------------------------------------------------
#include "Thread.h"
//-------------------------------------------------------------------------------------------

namespace Z
{
//-------------------------------------------------------------------------------------------
/*
    StopThreadEvent

    StopThreadEvent MUST be handled by all threads: when a thread receives StopThreadEvent
    MUST terminate what was doing and exit as soon as possible (obviously in a fair way, that
    is freeing allocated memory, ...)

    StopThreadEvent is the only event whose label is reserved and cannot be used for other
    events
*/
const std::string StopThreadLabel = "StopThreadEvent";

class StopThreadEvent : public Event {
    Event* clone()const{return new StopThreadEvent(*this);}
public:
    StopThreadEvent():Event(StopThreadLabel){}
};
//-------------------------------------------------------------------------------------------
/*
    RawByteBufferData

    RawByteBufferData implements a generic event made up by a byte buffer. The semantic meaning
    is left to the user.

*/
class RawByteBufferData : public Event {
    unsigned char* _buf;
    int _len;
    virtual Event* clone()const;
public:
    unsigned char* buf() const;
    int len() const;
    RawByteBufferData(const std::string& label, const unsigned char* buf, const int len);
    RawByteBufferData(const std::string& label, const std::vector<unsigned char>&);
    RawByteBufferData(const RawByteBufferData &);
    ~RawByteBufferData();
    RawByteBufferData & operator = (const RawByteBufferData &);
};
//-------------------------------------------------------------------------------------------
/*
    InfoMsg

    the purpose of this event is notify an info message by means of a string. The structure if
    InfoMsg event is identical to that of zibErr event, but the use purpose is different (zibErr
    is meant to notify an error)

*/
class InfoMsg : public Event {
    std::string _msg;
    virtual Event* clone()const;
public:
    std::string msg() const;
    InfoMsg(const std::string&, const std::string&);
    InfoMsg(const InfoMsg &);
    ~InfoMsg();
    InfoMsg & operator = (const InfoMsg &);
};
//-------------------------------------------------------------------------------------------
/*
    TimeoutEvent

    Note: timerId is identified by the label
*/
class TimeoutEvent : public Event {
    Event* clone()const{return new TimeoutEvent(*this);}
public:
    std::string timerId() const {return _label;}
    TimeoutEvent(std::string timerId):Event(timerId){}
};
//-------------------------------------------------------------------------------------------
/*
    zibErr

    the purpose of this event is to notify a generic error
*/
class zibErr : public Event {
    std::string _errorMsg;
    virtual Event* clone()const;
public:
    std::string errorMsg() const;
    zibErr(const std::string&, const std::string&);
    zibErr(const zibErr &);
    ~zibErr();
    zibErr & operator = (const zibErr &);
};
//-------------------------------------------------------------------------------------------
}//namespace Z
#endif	/* _EVENT_H */

/*
Tips:

1) It's not necessary to have a thread to emit an event (although, of course, it's reasonable
   there is almost one receiver!). All the threads who have registered to an event will receive
   a copy of that event in their event queue, whenever someone somewhere calls emitEvent() method
   on that event. A thread can register to an event (thus will receive a copy of it when it's
   emitted) by calling register2Label method and specifying the string label that uniquely identifies
   the event (the string passed to the constructor of that event). A thread can check out an event
   from his event queue by calling the method pullout. Once a thread has taken an event, it's his
   responibility to delete it after use.

2) Remember that a thread receives all the event whose string identifier are equal to the label
   it has registered to, no matter what type are that events. This means that if more events share
   the same label, they all will be received and then we can discriminate between them by means of a
   dynamic_cast operation. Similarly a single object ot type Event can be used for different events
   using different label (for example the event ``RawByteBufferData`` is used by SerialPortHandler
   as well as TcpIpc, ...)

3) Event class is an abstract interface (it cannot be directly instantiated). To create a new event
   you have to inherit the Event class and define the pure virtual method clone that makes a deep
   copy of the event data (your inherited class). The best way to accomplish this requirement is to
   use the well known clone design pattern, e.g. to clone the class foo:

   foo* clone()const{return new foo(*this);}

   remember that this pattern works correctly provided that you comply to the well known "big 3 rule
   of C++" that is if your class needs to define the destructor or the copy constructor or the copy
   assigment operator then it should probably explicitly define all three.
*/
