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

#include "Tools.h"
#include "Log.h"

namespace Z
{
//-------------------------------------------------------------------------------------------
//NOTA: alcuni comandi non ricevono una risposta dall'OS (per esempio
//quando lancio un programma esterno in background con "&"). In questi
//casi se lascio wait4answ a true (com'e' di default) la fgets rimane
//bloccata in attesa di dati sullo stream ma i dati non arriveranno mai!
//In questi casi e` necessario impostare wait4answ a false.
std::string OsCmd::execute(std::string cmd, bool wait4answ=true)
{
    char buffer[3072];//3k buffer... dovrebbe bastare!
    std::string out;
    if(cmd[cmd.length()-1]!='\n') cmd+='\n';
    sysOut = popen(cmd.c_str(), "r");
    if (!sysOut) {
        ziblog(LOG::ERR, "system error!");
        exit(-1);
    }
    if(wait4answ) while(fgets(buffer, sizeof(buffer), sysOut)) out +=buffer;
    pclose(sysOut);
    return (wait4answ ? out.substr (0,out.length()-1) : out);//elimino l'ultimo "\n" in caso di wait4answ altrimenti
                                                             //ritorno la stringa vuota (che al chiamante non serve,
                                                             //dato che ha deciso di non leggere la risposta)
}
//-------------------------------------------------------------------------------------------
std::vector<std::string> OsCmd::getSerialPortList()
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
//-------------------------------------------------------------------------------------------
currentDateTime::currentDateTime()
{
    time_t now;
    time(&now);
    struct tm* current = localtime(&now);
    year=current->tm_year-100;
    month=current->tm_mon+1;
    day=current->tm_mday;
    hour=current->tm_hour;
    min=current->tm_min;
    sec=current->tm_sec;
}
//-------------------------------------------------------------------------------------------
std::vector<unsigned char> serializeStdString (const std::string& o)
{
    //la lunghezza della stringa messa all'inizio serve per poter concatenare diversi dati
    //serializzati (altrimenti non saprei dove finisce la serializzazione della stringa e
    //dove inizia quella di un'altro dato!
    std::vector<unsigned char> sStdS;
    std::vector<unsigned char> sStdSLen = serialize((int)(o.size()));
    std::copy(sStdSLen.begin(), sStdSLen.end(), std::back_inserter(sStdS));
    for(size_t i=0; i<o.size(); i++) sStdS.push_back(o[i]);
    return sStdS;
}

std::string deserializeStdString (std::vector<unsigned char> & s)
{
    std::string ret;
    int sLen = deserialize<int>(&s[0]);
    for(int i=4; i<sLen+(int)(sizeof(int)); i++) ret+=s[i];
    s.erase(s.begin(),s.begin()+sLen+(int)(sizeof(int)));
    return ret;
}
//-------------------------------------------------------------------------------------------
// CONFIGURATION FILE FACILITIES

configFileHandler::configFileHandler(std::string configFile,
                                     std::map<std::string, std::string> configData,
                                     std::vector<char> commentTags) :configFile(configFile),
                                                                     configData(configData),
                                                                     commentTags(commentTags)
{
    if(configFile.empty()){
        ziblog(LOG::ERR, "invalid file name");
        return;
    }
    if(commentTags.empty()) {//se non specificato diversamente, di default sono commenti le linee che iniziano con "#" o con ";"
        this->commentTags.push_back('#');
        this->commentTags.push_back(';');
    }
}

void configFileHandler::read()
{
    std::ifstream cf(configFile.c_str());
    if(!cf) {
        ziblog(LOG::ERR, "configuration file not found!");
        return;
    }
    std::string s, key, value;
    while (std::getline(cf, s)) {
        std::string::size_type begin=s.find_first_not_of(" \f\t\v");
        if(begin==std::string::npos) continue;//salto le linee vuote
        if(std::string(commentTags.begin(), commentTags.end()).find(s[begin])!=std::string::npos) continue;//salto i commenti
        //estrazione del nome del parametro (key)
        std::string::size_type end=s.find('=', begin);
        key=s.substr(begin, end-begin);
        key.erase(key.find_last_not_of(" \f\t\v")+1);//elimino gli eventuali spazi iniziali e finali
        if(key.empty()) continue;//scarto eventuali valori vuoti (parametro nel file ma nessun valore)
        //estrazione del valore corrispondente (value)
        begin=s.find_first_not_of(" \f\n\r\t\v", end+1);
        end=s.find_last_not_of(" \f\n\r\t\v")+1;
        value=s.substr(begin, end-begin);
        //inserisco la coppia (key, value) nella mappa
        configData[key]=value;
    }
}

void configFileHandler::write()
{
    std::ofstream cf(configFile.c_str(), std::fstream::trunc);
    if(!cf) {
        ziblog(LOG::ERR, "configuration file not found!");
        return;
    }
    std::map <std::string, std::string>::const_iterator iter;
    for (iter = configData.begin(); iter != configData.end(); iter++) cf<<iter->first<<" = "<<iter->second<<std::endl;
}

std::map<std::string, std::string> configFileHandler::getConfigData(){return configData;}

std::string configFileHandler::getRawValue(const std::string & parameter)
{
    std::string key;
    if(configData.count(parameter)) key=configData.find(parameter)->second;
    return key;
}

void configFileHandler::addOrUpdateKeyValue(const std::string& parameter, const std::string& value)
{
    configData[parameter]=value;//aggiorno il parametro esistente o se non c'e` creo una entry nuova
}

bool configFileHandler::getBoolValue(bool& dest, std::string param)
{
    bool ret=false;
    if(configData.count(param)) {
        ret=true;
        std::string value=configData[param];
        if(value == "true") dest = true;
        else if(value == "false") dest = false;
        else {
            ret=false;
            ziblog(LOG::ERR, "invalid %s value (%s) ... It's supposed to be bool", param.c_str(), value.c_str());
        }
    } else ziblog(LOG::ERR, "parameter %s not found in condifuration data", param.c_str());
    return ret;
}

bool configFileHandler::getIntValue(int& dest, std::string param)
{
    bool ret=false;
    if(configData.count(param)) {
        ret=true;
        dest=atoi((configData[param]).c_str());
    } else ziblog(LOG::ERR, "parameter %s not found in condifuration data", param.c_str());
    return ret;
}
//-------------------------------------------------------------------------------------------
}//namespace Z
