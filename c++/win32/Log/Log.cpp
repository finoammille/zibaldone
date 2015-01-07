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

#include "Log.h"
#include "Events.h"

namespace Z
{
//-------------------------------------------------------------------------------------------
LOG* LOG::_instance = NULL;
bool LOG::_isRunning = false;
LOG::Level LOG::level = LOG::ERR;
int LOG::_bufferSize = 14;//spazio minimo necessario per scrivere "...TRUNCATED!" in caso di buffer insufficiente

//l'evento logCmdEvent e` utilizzato internamente da LOG pertanto non va messo in Log.h altrimenti
//sarebbe visibile a tutti!
struct logCmdEvent : public Event {
    std::string logMsg;
    logCmdEvent(const std::string & msg):Event("logCmdEvent"), logMsg(msg){}
    Event* clone()const{return new logCmdEvent(*this);}
};

LOG::LOG(const std::string& logFileDstDirPath, const std::string& logFileNamePrefix)
{
    exit = false;
    std::string logFileName = logFileDstDirPath;
    if(!logFileName.empty() && *logFileName.rbegin() != '\\') logFileName+='\\';
    if(!logFileNamePrefix.empty()) logFileName+=(logFileNamePrefix+"_");
    logFileName+="log";
	SYSTEMTIME now;
	GetLocalTime(&now);
	std::stringstream tmp;
    tmp<<"_"<<std::setw(2)<<std::setfill('0')<<now.wDay//day
       <<"-"<<std::setw(2)<<std::setfill('0')<<now.wMonth//month
       <<"-"<<std::setw(2)<<std::setfill('0')<<now.wYear;//year    
    logFileName+=tmp.str();
    register2Label("logCmdEvent");//autoregistrazione per le richieste di log
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
    if(!_instance) return;//non c'e' nulla da disabilitare!
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
        Event* Ev = pullOut();//max 10 msec di attesa
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
