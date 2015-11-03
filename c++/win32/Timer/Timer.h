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

#ifndef _TIMER_H
#define	_TIMER_H

#include "Log.h"
#include "Thread.h"

namespace Z
{
//-------------------------------------------------------------------------------------------
/*
NOTE: the class Timer implements an "alarm clock". The end user instantiate a Timer object
      and gives it a name. When the timer expires it's emitted an event with label equal
      to the name assigned to it.
*/
class Timer {
    HANDLE _tId;
    const std::string timerId;
    int _duration;
    bool _running;
    HANDLE _lock;
    static VOID CALLBACK onTimeout(PVOID lpParameter, BOOLEAN TimerOrWaitFired);
public:
    std::string getTimerId() const {return timerId;}
    Timer(const std::string& timerId, int duration=-1);
    ~Timer();
    void Start(int mSec=-1);// the default mSec value (-1) indicates the timer keeps the previously
                            // assigned duration (variable _duration). So a call to Start() without
                            // parameters rearms the Timer with previous duration.
    void Stop();
};
//-------------------------------------------------------------------------------------------
}//namespace Z
#endif	/* _TIMER_H */
