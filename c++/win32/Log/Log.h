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

// Utilizzo di log:
// 1) chiamare LOG::set() per impostare la directory in cui andranno i log, il prefisso 
//    per il file di log, il livello di log, la dimensione del buffer di log, se 
//    visualizzare i log anche su console (di default il log parte disabilitato)
// 2) una volta abilitato il log chiamare ziblog per loggare
// 3) chiamare LOG::disable() per terminare il log e disabilitarlo (disable chiude il file
//    di log, stoppa il thread che serializza le richieste di log e dealloca la memoria

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

/*
NOTA: mi e` capitato di avere un conflitto sul nome a causa buffer definito nella macro ziblog!
Trattandosi di una macro, e` difficile accorgersi del problema e l'errore dato dal compilatore
sembra strano ... invece ha ragione!
Usare "buffer" non e` una buona idea dato che si tratta di un nome utilizzato frequentemente.
Per ridurre la probabilita` di avere problemi analoghi in futuro, ho rinominato buffer come
bUfFeR, che difficilmente sara` usato!!!!
*/
