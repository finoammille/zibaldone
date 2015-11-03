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

std::string zibErr::errorMsg() const{return _errorMsg;}

Event* zibErr::clone()const{return new zibErr(*this);}
//-------------------------------------------------------------------------------------------
/*
    InfoMsg
*/
InfoMsg::InfoMsg(const std::string& label, const std::string& msg):Event(label), _msg(msg){}

std::string InfoMsg::msg() const{return _msg;}

Event* InfoMsg::clone()const{return new InfoMsg(*this);}
//-------------------------------------------------------------------------------------------
}//namespace Z
