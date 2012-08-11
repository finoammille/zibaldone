/*
 *
 * zibaldone - a C++ library for Thread, Timers and other Stuff
 *
 * Copyright (C) 2012  Antonio Buccino
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
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

const std::string TimeoutEvent::TimeoutEventName = "TimeoutEvent";

Timer::Timer(std::string timerName, Thread* target, int duration):_target(target),_duration(duration),_running(false),timerName(timerName)
{
    pthread_mutex_init(&_lock, NULL);
}

Timer::~Timer()
{
    Stop();
}

void Timer::onTimeout(sigval_t val)
{
    pthread_mutex_lock(&(((Timer*)val.sival_ptr)->_lock));
    ((Timer*)val.sival_ptr)->_running = false;
    pthread_mutex_unlock(&(((Timer*)val.sival_ptr)->_lock));
    TimeoutEvent* te = new TimeoutEvent(((Timer*)val.sival_ptr)->_target, ((Timer*)val.sival_ptr)->timerName);
    te->notify();
    delete te;
}

void Timer::Start(int mSec)
{
    if(mSec) _duration = mSec;
    if(!_duration) {
        ziblog("timer %s has non valid duration value", timerName.c_str());//se sono qui, nè il costruttore ne il chiamante di Start hanno assegnato un valore utile a _duration
        return;
    }
   
    pthread_mutex_lock(&_lock);
    if(_running) {
        timer_delete(_tId);
        ziblog("timer %s previously created set to new value", timerName.c_str());
    }
    pthread_mutex_unlock(&_lock);
    sigevent e;
    e.sigev_notify = SIGEV_THREAD;
    e.sigev_notify_function = Timer::onTimeout;
    e.sigev_value.sival_ptr = (void*)this;
    e.sigev_notify_attributes = NULL;
    timer_create(CLOCK_REALTIME, &e, &_tId);
    itimerspec timerspec;
    timerspec.it_value.tv_sec = mSec/1000;
    timerspec.it_value.tv_nsec = (mSec%1000)*1000000;
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
