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

#define ziblog( s, arg... ) \
({ \
    char buffer[1024]; \
    sprintf(buffer, (s), ##arg); \
    std::string x(__FILE__); \
    x+=", "; \
    std::stringstream tmp; \
    tmp<<__LINE__; \
    x+= tmp.str(); \
    x+=", "; \
    x+=__PRETTY_FUNCTION__; \
    x+=": "; \
    x+=buffer; \
    LOG::write(x); \
})

class LOG : public Thread {
    LOG();
    static bool _isRunning;
    std::ofstream logFile;
    static LOG* _instance;
    bool exit;
    void run();
    struct logCmd : public Event {
        static std::string logCmdEventName;
        std::string logMsg;
        logCmd(std::string logMsg):Event(logCmdEventName), logMsg(logMsg){}
    protected:
        Event* clone() const {return new logCmd(*this);}
    };
public:
    static LOG* Instance(){return _instance = ((_instance) ? _instance : new LOG());}
    ~LOG(){logFile.close();}
    static void write(std::string);//TODO: aggiungere livelli di log e gestione di piu' dispositivi di log (api "add logDevice")
    void startLogging();
    void stopLogging();
};




//il problema lettori scrittori sul log device (file, console....) si risolve facendo diventare il log un thread che riceve gli eventi di log e li scrive.
// si tratta di un thread un po particolare dato che deve ricevere eventi da tutti ma non invia eventi a nessuno. Allora si puo' fare un'api tipo quella
//di timer che gira nello spazio di indirizzamento del thread chiamante e posta un messaggio sulla coda di logger che nel suo timeslice provvede a
//serializzare le scritture sul log device estraendo i messaggi dalla coda.


//requisiti desiderati:
//1) inizio con log su file e su console, poi dovrei poter aggiungere, abilitare/disabilitare tanto il dispositivo di log quanto il livello di log
//   con una semplice parametro del metodo di log.
//2) il log dovrebbe essere presente solo se la compilazione avviene con un flag che chiamiamo "LOG_DEBUG_ACTIVE", in caso contrario il log deve
//   essere inesistente
//3) probabilmente la classe log dovra' essere un singleton che nasce con l'applicazione e muore con essa (la istanzio dentro main()....)


#endif	/* _LOG_H */

