/*
 *
 * zibaldone - a C++ library for Thread, Timers and other Stuff
 * http://sourceforge.net/projects/zibaldone/
 *
 * Copyright (C) 2012  ilant (ilant@users.sourceforge.net)
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

#include "Log.h"
#include "Events.h"

namespace Z
{
//-------------------------------------------------------------------------------------------
LOG* LOG::_instance = NULL;
bool LOG::_isRunning = false;
LOG::Level LOG::level = LOG::ERR;
int LOG::_bufferSize = 14;//minimum length to write "...TRUNCATED!" just in case of insufficient buffer space

//The logCmdEvent event is used internally by LOG and is not intended for public use. This is why it's
//declared here instead of in the Log.h file
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
    logFile.open(logFileName.c_str(), std::ios::app);
}

void LOG::set(const std::string& logFileDstDirPath, const std::string& logFileNamePrefix, Level logLev, bool showLogOnConsole, int bufferSize)
{
    if(!_instance) _instance=new LOG(logFileDstDirPath, logFileNamePrefix);
    _instance->level=logLev;
    _instance->_showLogOnConsole=showLogOnConsole;
    if(bufferSize<14) bufferSize=14;
    _instance->_bufferSize=bufferSize;
    if(!_instance->_isRunning){
        _instance->Start();
        _instance->_isRunning = true;
    }
}

int LOG::bufferSize() {return _bufferSize;}

void LOG::disable()
{
    if(!_instance) return;
    if(_instance->_isRunning) {
        _instance->Stop();
        _instance->_isRunning = false;
    }
    _instance->logFile.close();
    delete _instance;
    _instance=NULL;
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
