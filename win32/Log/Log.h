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

#ifndef _LOG_H
#define	_LOG_H

#include "Thread.h"
#include <time.h>
#include <fstream>
#include <sstream>
#include <iomanip>

// Utilizzo di log:
// 1) chiamare LOG::set() per impostare il livello di log e se visualizzare
//    i log anche su console (di default il log parte disabilitato)
// 2) una volta abilitato il log chiamare ziblog per loggare
// 3) chiamare LOG::disable() per terminare il log e disabilitarlo (disable chiude il file
//    di log, stoppa il thread che serializza le richieste di log e dealloca la memoria

namespace Z
{
//-------------------------------------------------------------------------------------------
class LOG : public Thread {
    LOG();
    static bool _isRunning;
    std::ofstream logFile;
    static LOG* _instance;
    bool exit;
    bool _showLogOnConsole;
    void run();
public:
    static enum Level {DBG, INF, WRN, ERR} level; 
    static void set(Level lev, bool showLogOnConsole=true);//stabilisce il livello di log e se visualizzare i log anche sulla console (oltre che su log file)
    static void disable();
    static void enqueueMessage(LOG::Level, const std::string&);
};

#if defined(_MSC_VER)
#define sprintf sprintf_s
#endif

#define ziblog(level, logMsg, ...) \
{ \
    char buffer[1024]; \
    sprintf(buffer, (logMsg), ##__VA_ARGS__); \
	SYSTEMTIME now; \
	GetLocalTime(&now); \
    std::stringstream sstimeStamp; \
    sstimeStamp<<std::setw(2)<<std::setfill('0')<<now.wDay \
               <<"-"<<std::setw(2)<<std::setfill('0')<<now.wMonth \
               <<"-"<<std::setw(2)<<std::setfill('0')<<now.wYear \
               <<" "<<std::setw(2)<<std::setfill('0')<<now.wHour \
               <<":"<<std::setw(2)<<std::setfill('0')<<now.wMinute \
               <<":"<<std::setw(2)<<std::setfill('0')<<now.wSecond<<", "; \
    std::string logLevel; \
    if(level == LOG::DBG) logLevel = "(DEBUG) "; \
    else if(level == LOG::INF) logLevel = "(INFO) "; \
    else if(level == LOG::WRN) logLevel = "(WARNING) "; \
    else logLevel = "(ERROR) "; \
    std::string timestamp = sstimeStamp.str(); \
    std::string position(__FILE__); \
    position+=", "; \
    std::stringstream lineNumber; \
    lineNumber<<__LINE__; \
    position+= lineNumber.str(); \
    position+=", "; \
    position+=__FUNCTION__; \
    position+=": "; \
    std::string msg = buffer; \
    std::string log = timestamp + logLevel + position + msg; \
    LOG::enqueueMessage(level, log); \
}
//-------------------------------------------------------------------------------------------
}//namespace Z
#endif	/* _LOG_H */
