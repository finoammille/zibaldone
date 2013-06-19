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

#include "Log.h"

namespace Z
{
//-------------------------------------------------------------------------------------------
LOG* LOG::_instance = NULL;
bool LOG::_isRunning = false;
LOG::Level LOG::level = LOG::ERROR;

LOG::LOG()
{
    exit = false;
    std::string logFileName = "log";
    time_t now;
    time(&now);
    struct tm* current = localtime(&now);
    std::stringstream tmp;
    tmp<<"_"<<std::setw(2)<<std::setfill('0')<<current->tm_mday//day
    <<"-"<<std::setw(2)<<std::setfill('0')<<(current->tm_mon+1)//month
    <<"-"<<std::setw(2)<<std::setfill('0')<<(current->tm_year-100);//year
    logFileName += tmp.str();
    register2Event("logCmdEvent");//autoregistrazione per le richieste di log
    logFile.open(logFileName.c_str(), std::ios::app);
}

void LOG::set(Level logLev, bool showLogOnConsole)
{
    if(!_instance) _instance=new LOG();
    _instance->level=logLev;
    _instance->_showLogOnConsole=showLogOnConsole;
    if(!_instance->_isRunning){
        _instance->Start();
        _instance->_isRunning = true;
    }
}

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
        std::vector<unsigned char> logMsg;
        for(unsigned int i=0; i<log.size(); i++) logMsg.push_back((unsigned char)log[i]);
        Event logCmd("logCmdEvent", logMsg); 
        logCmd.emitEvent();
    }
}

void LOG::run()
{
    while(!exit){
        Event* Ev = pullOut(10);//max 10 msec di attesa
        if(Ev){
            std::string eventId = Ev->eventId();
            if(eventId == StopThreadEventId) exit = true; 
            else if(eventId == "logCmdEvent") {
                std::string logMsg = std::string((const char*)Ev->buf(), Ev->len());
                logFile<<logMsg<<std::endl;
                if(_showLogOnConsole) std::cerr<<logMsg<<std::endl;
            } else std::cerr<<"LOG THREAD HAS RECEIVED AN UNEXPECTED EVENT ("<<eventId<<")\n"; 
        }
        delete Ev;
    }
}
//-------------------------------------------------------------------------------------------
}//namespace Z
