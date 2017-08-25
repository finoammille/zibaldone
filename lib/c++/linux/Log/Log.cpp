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

#include "Log.h"
#include "Events.h"

namespace Z
{
//-------------------------------------------------------------------------------------------
LOG* LOG::_instance = NULL;
LOG::Level LOG::level = LOG::ERR;
int LOG::_bufferSize = 14;//minimum length to write "...TRUNCATED!" just in case of insufficient buffer space
                          //spazio minimo necessario per scrivere "...TRUNCATED!" in caso di buffer insufficiente
pthread_mutex_t LOG::_lock = PTHREAD_MUTEX_INITIALIZER;

//The logCmdEvent event is used internally by LOG and is not intended for
//public use. This is why it's declared here instead of in the Log.h file
//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
//l'evento logCmdEvent e` utilizzato internamente da LOG pertanto non va
//messo in Log.h altrimenti sarebbe visibile a tutti!
struct logCmdEvent : public Event {
    std::string logMsg;
    logCmdEvent(const std::string & msg):Event("logCmdEvent"), logMsg(msg){}
    Event* clone()const{return new logCmdEvent(*this);}
};

LOG::LOG(const std::string& logFileDstDirPath, const std::string& logFileNamePrefix)
{
    exit = false;
    std::string logFileName = logFileDstDirPath;
    if(!logFileName.empty() && *logFileName.rbegin() != '/') logFileName+='/';
    if(!logFileNamePrefix.empty()) logFileName+=(logFileNamePrefix+"_");
    logFileName+="log";
    time_t now;
    time(&now);
    struct tm* current = localtime(&now);
    std::stringstream tmp;
    tmp<<"_"<<std::setw(2)<<std::setfill('0')<<current->tm_mday//day
    <<"-"<<std::setw(2)<<std::setfill('0')<<(current->tm_mon+1)//month
    <<"-"<<std::setw(2)<<std::setfill('0')<<(current->tm_year-100);//year
    logFileName += tmp.str();
    register2Label("logCmdEvent");//self-registration for log requests
                                  //autoregistrazione per le richieste di log
    logFile.open(logFileName.c_str(), std::ios::app);
}

void LOG::set(const std::string& logFileDstDirPath, const std::string& logFileNamePrefix, Level logLev, bool showLogOnConsole, int bufferSize)
{
    pthread_mutex_lock(&_lock);
    if(!_instance) _instance=new LOG(logFileDstDirPath, logFileNamePrefix);
    _instance->level=logLev;
    _instance->_showLogOnConsole=showLogOnConsole;
    if(bufferSize<14) bufferSize=14;
    _instance->_bufferSize=bufferSize;
    _instance->Start();
    pthread_mutex_unlock(&_lock);
}

int LOG::bufferSize() {return _bufferSize;}

void LOG::disable()
{
    pthread_mutex_lock(&_lock);
    if(!_instance) {
        pthread_mutex_unlock(&_lock);
        return;//non c'e' nulla da disabilitare!
    }
    _instance->Stop();
    _instance->logFile.close();
    delete _instance;
    _instance=NULL;
    pthread_mutex_unlock(&_lock);
}

void LOG::enqueueMessage(LOG::Level level, const std::string& log)
{
	if(level >= LOG::level) {
        logCmdEvent logCmd(log);
        logCmd.emitEvent();
    }
}

void LOG::run()
{
    const std::string StartLogSessionMessage = "\n\n => NEW LOG SESSION";
    logFile<<StartLogSessionMessage<<std::endl;
    while(!exit){
        Event* Ev = pullOut();
        if(Ev){
            if(dynamic_cast<StopThreadEvent *>(Ev)) exit = true;
            else if(dynamic_cast<logCmdEvent *>(Ev)) {
                logCmdEvent* log = (logCmdEvent*)Ev;
                logFile<<log->logMsg<<std::endl;
                if(_showLogOnConsole) std::cerr<<log->logMsg<<std::endl;
            } else std::cerr<<"unexpected event\n";
        }
        delete Ev;
    }
}
//-------------------------------------------------------------------------------------------
}//namespace Z
