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

#include "Tools.h"

//NOTA: alcuni comandi non ricevono una risposta dall'OS (per esempio
//quando lancio un programma esterno in background con "&"). In questi
//casi se lascio wait4answ a true (com'e' di default) la fgets rimane
//bloccata in attesa di dati sullo stream ma i dati non arriveranno mai!
//In questi casi e` necessario impostare wait4answ a false.
std::string zibTools::execute(std::string cmd, bool wait4answ=true)
{
    char buffer[3072];//3k buffer... dovrebbe bastare!
    std::string out;
    if(cmd[cmd.length()-1]!='\n') cmd+='\n';
    sysOut = popen(cmd.c_str(), "r");
    if (!sysOut) {
        std::cerr<<"system error!";
        exit(-1);
    }
    if(wait4answ) while(fgets(buffer, sizeof(buffer), sysOut)) out +=buffer;
    pclose(sysOut);
    return (wait4answ ? out.substr (0,out.length()-1) : out);//elimino l'ultimo "\n" in caso di wait4answ altrimenti 
                                                             //ritorno la stringa vuota (che al chiamante non serve,
                                                             //dato che ha deciso di non leggere la risposta)
}

std::vector<std::string> zibTools::getSerialPortList()
{
    std::vector<std::string> result;

    //TODO

    /*
    std::string tmp = execute("ls /dev/ttyS* 2>/dev/null");//lista delle porte seriali
    std::stringstream lsdevttysxResult(tmp);
    while(lsdevttysxResult >> tmp) result.push_back(tmp);
    tmp = execute("ls /dev/ttyUSB* 2>/dev/null");//lista delle porte seriali USB ftdi
    std::stringstream lsdevttyusbxResult(tmp);
    while(lsdevttyusbxResult >> tmp) result.push_back(tmp);
    tmp = execute("ls /dev/ttyACM* 2>/dev/null");//lista delle porte seriali USB cdc_acm
    std::stringstream lsdevttyacmxResult(tmp);
    while(lsdevttyacmxResult >> tmp) result.push_back(tmp);
    */
    return result;
}
