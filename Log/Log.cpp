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

std::string LOG::logCmd::logCmdEventName = "logCmd";
LOG* LOG::_instance = NULL;
bool LOG::_isRunning = false;

LOG::LOG()
{
    exit = false;
    std::string logFileName = "log";
    register2Event(LOG::logCmd::logCmdEventName);//autoregistrazione per le richieste di log
    logFile.open(logFileName.c_str(), std::ios::app);
}

void LOG::startLogging()
{
    if(!_isRunning){
        Start();
        _isRunning = true;
    }
}

void LOG::stopLogging()
{
    if(_isRunning) {
        Stop();
        _isRunning = false;
    }
}

void LOG::write(std::string logMsg)
{
    if(!_isRunning) {
        std::cerr<<"Can't log: logger is not running!\n";
        return;
    }
    time_t now;
    time(&now);
    struct tm* current = localtime(&now);
    std::stringstream tmp;
    tmp<<std::setw(2)<<current->tm_mday<<std::setfill('0')//day
    <<"-"<<std::setfill('0')<<std::setw(2)<<(current->tm_mon+1)//month
    <<"-"<<(current->tm_year-100)<<" "//year
    <<std::setfill('0')<<std::setw(2)<<current->tm_hour//hour
    <<":"<<std::setfill('0')<<std::setw(2)<<current->tm_min//min
    <<":"<<std::setfill('0')<<std::setw(2)<<current->tm_sec<<", ";//sec
    std::string date = tmp.str();
    logCmd log(date + logMsg);
    log.emitEvent();
}

void LOG::run()
{
    while(!exit){
        Event* Ev = pullOut(10);//max 10 msec di attesa
        if(Ev){
            std::string eventName = Ev->eventName();
            if(eventName == StopThreadEvent::StopThreadEventName) exit = true; 
            else if(eventName == LOG::logCmd::logCmdEventName){
                logCmd* log = (logCmd *) Ev;
                logFile<<log->logMsg<<std::endl;
                std::cerr<<log->logMsg<<std::endl;
            } else std::cerr<<"LOG THREAD HAS RECEIVED AN UNEXPECTED EVENT ("<<eventName<<")\n"; 
        }
        delete Ev;
    }
}

//usare:

//__FILE__
//__LINE__
//__DATE__
//__TIME__
//__PRETTY_FUNCTION__ (versione di "__FUNCTION__" contenente anche lo scope c++ del metodo)
