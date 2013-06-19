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

#ifndef _TIMER_H
#define	_TIMER_H

#include "Log.h"
#include "Thread.h"

namespace Z
{
//-------------------------------------------------------------------------------------------
//NOTA: la classe Timer implementa sostanzialmente una "sveglia". L'utilizzatore, istanzia un oggetto Timer, al quale
//assegna un nome, Alla scadenza del timer, viene inviato un evento "timeout" associato al nome della sveglia.
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
    void Start(int mSec=-1);//Nota: il valore di default (-1) indica che il timer va armato con la durata già specificata nella variabile _duration
    void Stop();
};
//-------------------------------------------------------------------------------------------
}//namespace Z
#endif	/* _TIMER_H */
