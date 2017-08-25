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
Timer::Timer(const std::string& timerId, int duration):timerId(timerId),_duration(duration),_running(false)
{
    pthread_mutex_init(&_lock, NULL);
}

Timer::~Timer(){Stop();}

void Timer::onTimeout(sigval_t val)
{
    pthread_mutex_lock(&(((Timer*)val.sival_ptr)->_lock));
    ((Timer*)val.sival_ptr)->_running = false;
    pthread_mutex_unlock(&(((Timer*)val.sival_ptr)->_lock));
    TimeoutEvent timeout(((Timer*)val.sival_ptr)->timerId);
    timeout.emitEvent();
}

void Timer::Start(int mSec)
{
    if(mSec!=-1) _duration = mSec;
    if(!_duration) {
        ziblog(LOG::WRN, "timer %s has non valid duration value", timerId.c_str());//se sono qui, nè il costruttore ne il chiamante di Start hanno assegnato un valore utile a _duration
        return;
    }
    pthread_mutex_lock(&_lock);
    if(_running) {
        timer_delete(_tId);
        ziblog(LOG::WRN, "timer %s previously created set to new value", timerId.c_str());
    }
    pthread_mutex_unlock(&_lock);
    sigevent e;
    e.sigev_notify = SIGEV_THREAD;
    e.sigev_notify_function = Timer::onTimeout;
    e.sigev_value.sival_ptr = (void*)this;
    e.sigev_notify_attributes = NULL;
    timer_create(CLOCK_REALTIME, &e, &_tId);
    itimerspec timerspec;
    timerspec.it_value.tv_sec = _duration/1000;
    timerspec.it_value.tv_nsec = (_duration%1000)*1000000;
    timerspec.it_interval.tv_sec = 0;
    timerspec.it_interval.tv_nsec = 0;
    timer_settime(_tId, 0, &timerspec, NULL);
    pthread_mutex_lock(&_lock);
    _running = true;
    pthread_mutex_unlock(&_lock);
}

void Timer::Stop()
{
    pthread_mutex_lock(&_lock);
    if(_running) timer_delete(_tId);
    _running = false;
    pthread_mutex_unlock(&_lock);
}
//-------------------------------------------------------------------------------------------
}//namespace Z
