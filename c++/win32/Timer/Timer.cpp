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
