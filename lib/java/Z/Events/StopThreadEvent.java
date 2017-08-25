/*
 *
 * zibaldone - a C++/Java library for Thread, Timers and other Stuff
 *
 * http://sourceforge.net/projects/zibaldone/
 *
 * version 3.1.2, August 29th, 2015
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

package Z;
//----------------------------------------------------------------------------------------------------------------------
/*
    StopThreadEvent

    StopThreadEvent MUST be handled by all threads: when a thread receives StopThreadEvent MUST terminate what was doing
    and exit as soon as possible (obviously in a fair way, that is freeing allocated memory, ...)

    StopThreadEvent is the only event whose label is reserved and cannot be used for other events

    ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

    L'evento StopThreadEvent deve essere OBBLIGATORIAMENTE gestito da tutti i thread: quando un thread riceve l'evento 
    StopThreadEvent DEVE terminare e uscire prima possibile.

    E` l'unico evento la cui label e` riservata e non puo` essere utilizzata per altri eventi.
*/
public class StopThreadEvent extends Event {
    final static String StopThreadLabel = "StopThreadEvent";
    StopThreadEvent() {super(StopThreadLabel);}//non va istanziato al di fuori del package!
}
//----------------------------------------------------------------------------------------------------------------------
