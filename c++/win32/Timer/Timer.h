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
