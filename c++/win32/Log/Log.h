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

#ifndef _LOG_H
#define	_LOG_H

#include "Thread.h"
#include <time.h>
#include <fstream>
#include <sstream>
#include <iomanip>

#if !defined(_MSC_VER)
#include <malloc.h>
#endif

// Use of log:
// 1) call LOG::set() to enable log (no log will be written before the call to LOG::set)
//    the LOG::set requires the following parameters:
//    - the path of the directory where will be open/created log file
//    - the prefix for log file name
//    - the log level
//    the following parameters are optional:
//    - whether if view log messages in the console too (default=true)
//    - the log buffer size (default= 4096 bytes)
// 2) now you can call ziblog and log!
// 3) you have to call LOG::disable() to disable log.
//    LOG::disable() closes the log file, stops the log thread (whose task is to serialize
//    the log requests) and deallocates the memory.

namespace Z
{
//-------------------------------------------------------------------------------------------
class LOG : public Thread {
    LOG(const std::string&, const std::string&);
    static int _bufferSize;
    static bool _isRunning;
    std::ofstream logFile;
    static LOG* _instance;
    bool exit;
    bool _showLogOnConsole;
    void run();
public:
    static enum Level {DBG, INF, WRN, ERR} level;
    static void set(const std::string& logFileDstDirPath, const std::string& logFileNamePrefix, Level logLev, bool showLogOnConsole=true, int bufferSize=4096);
    static int bufferSize();
    static void disable();
    static void enqueueMessage(LOG::Level, const std::string&);
};

#if defined(_MSC_VER)
#define snprintf sprintf_s
#endif

#define ziblog(level, logMsg, ...) do \
{ \
    const int ziblogMaxLogSize=LOG::bufferSize(); \
    char* bUfFeR = (char*) alloca(ziblogMaxLogSize); \
    if (snprintf(bUfFeR, ziblogMaxLogSize, (logMsg), ##__VA_ARGS__)>=ziblogMaxLogSize) { \
        std::string truncatedWarning="...TRUNCATED!"; \
        int warningMsgLen = truncatedWarning.length(); \
        for(int i=ziblogMaxLogSize-warningMsgLen-1, j=0; i<ziblogMaxLogSize && j<warningMsgLen; i++, j++) bUfFeR[i]=truncatedWarning[j]; \
    } \
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
    if((level) == LOG::DBG) logLevel = "(DEBUG) "; \
    else if((level) == LOG::INF) logLevel = "(INFO) "; \
    else if((level) == LOG::WRN) logLevel = "(WARNING) "; \
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
    std::string msg = bUfFeR; \
    std::string log = timestamp + logLevel + position + msg; \
    LOG::enqueueMessage((level), log); \
} while (0)
//-------------------------------------------------------------------------------------------
}//namespace Z
#endif	/* _LOG_H */
