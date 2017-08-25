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

#include "Timer.h"
#include "Events.h"

namespace Z
{
//-------------------------------------------------------------------------------------------
Timer::Timer(const std::string &timerId, int duration):timerId(timerId),_duration(duration),_running(false)
{
    _lock = CreateMutex(NULL, FALSE, NULL);
}

Timer::~Timer(){Stop();}

VOID CALLBACK Timer::onTimeout(PVOID lpParameter, BOOLEAN TimerOrWaitFired)
{
	Timer* t = (Timer*) lpParameter;
	WaitForSingleObject(t->_lock, INFINITE);
    t->_running = false;
	ReleaseMutex(t->_lock);
    TimeoutEvent timeout(t->getTimerId());
    timeout.emitEvent();
}

void Timer::Start(int mSec)
{
    if(mSec!=-1) _duration = mSec;
    if(!_duration) {
        ziblog(LOG::WRN, "timer %s has non valid duration value", timerId.c_str());//se sono qui, nè il costruttore ne il chiamante di Start hanno assegnato un valore utile a _duration
        return;
    }
    WaitForSingleObject(_lock, INFINITE);
    if(_running) {
		DeleteTimerQueueTimer(NULL, _tId, NULL);
        ziblog(LOG::INF, "timer %s previously created set to new value", timerId.c_str());
    }
	ReleaseMutex(_lock);
	CreateTimerQueueTimer(&_tId, NULL, Timer::onTimeout, this, _duration, 0, WT_EXECUTEDEFAULT);
	WaitForSingleObject(_lock, INFINITE);
    _running = true;
	ReleaseMutex(_lock);
}

void Timer::Stop()
{
    WaitForSingleObject(_lock, INFINITE);
    if(_running) DeleteTimerQueueTimer(NULL, _tId, NULL);
    _running = false;
    ReleaseMutex(_lock);
}
//-------------------------------------------------------------------------------------------
}//namespace Z
