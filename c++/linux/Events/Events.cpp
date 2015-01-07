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

#include "Events.h"
#include "Log.h"

namespace Z
{
//-------------------------------------------------------------------------------------------
/*
    RawByteBufferData
*/
RawByteBufferData::RawByteBufferData(const std::string& label, const unsigned char* buf, const int len):Event(label), _buf(0), _len(0)
{
    if(buf && len) {
        _len = len;
        _buf = new unsigned char[len];
        memcpy(_buf, buf, len);
    }
}

RawByteBufferData::RawByteBufferData(const std::string& label, const std::vector<unsigned char>& data):Event(label), _buf(0), _len(0)
{
    if(!data.empty()) {
        _len = data.size();
        _buf = new unsigned char[_len];
        memcpy(_buf, &data[0], _len);
    }
}

RawByteBufferData::~RawByteBufferData()
{
    _label.clear();
    delete[] _buf;
    _buf=NULL,
    _len=0;
}

RawByteBufferData::RawByteBufferData(const RawByteBufferData &obj):Event(obj)
{
    if(obj._len) {
        _buf = new unsigned char[obj._len];//allocate new space
        memcpy(_buf, obj._buf, obj._len);//copy values
    } else _buf = NULL;
    _len = obj._len;
}

RawByteBufferData & RawByteBufferData::operator = (const RawByteBufferData &src)
{
    if(this == &src) return *this; //self assignment check...
    Event::operator= (src);
    if(_buf) delete[] _buf;//deallocate
    if(src._len) {
        _buf = new unsigned char[src._len];//allocate new space
        memcpy(_buf, src._buf, src._len);//copy values
    } else _buf = NULL;
    _len = src._len;
    return *this;
}

unsigned char* RawByteBufferData::buf()const{return _buf;}

int RawByteBufferData::len()const{return _len;}

Event* RawByteBufferData::clone()const{return new RawByteBufferData(*this);}
//-------------------------------------------------------------------------------------------
/*
    zibErr
*/
zibErr::zibErr(const std::string& label, const std::string& errorMsg):Event(label), _errorMsg(errorMsg){}

zibErr::~zibErr(){_errorMsg.clear();}

std::string zibErr::errorMsg() const{return _errorMsg;}

zibErr::zibErr(const zibErr &obj):Event(obj){_errorMsg=obj._errorMsg;}

zibErr & zibErr::operator = (const zibErr &src)
{
    if(this == &src) return *this; //self assignment check...
    Event::operator= (src);
    _errorMsg=src._errorMsg;
    return *this;
}

Event* zibErr::clone()const{return new zibErr(*this);} 
//-------------------------------------------------------------------------------------------
/*
    InfoMsg
*/
InfoMsg::InfoMsg(const std::string& label, const std::string& msg):Event(label), _msg(msg){}

InfoMsg::~InfoMsg(){_msg.clear();}

std::string InfoMsg::msg() const{return _msg;}

InfoMsg::InfoMsg(const InfoMsg &obj):Event(obj){_msg=obj._msg;}

InfoMsg & InfoMsg::operator = (const InfoMsg &src)
{
    if(this == &src) return *this; //self assignment check...
    Event::operator= (src);
    _msg=src._msg;
    return *this;
}

Event* InfoMsg::clone()const{return new InfoMsg(*this);} 
//-------------------------------------------------------------------------------------------
}//namespace Z
