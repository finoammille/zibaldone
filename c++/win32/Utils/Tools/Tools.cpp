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

#include "Tools.h"
#include "Log.h"
#include <windows.h>


namespace Z
{
//-------------------------------------------------------------------------------------------
//NOTE: some commands do not receive an explicit answer from OS (for example a command
//followed by "&"). In those cases, we have to set the wait4answ flag to false (by default
//it's set to true) to avoid a hang waiting for a never coming answer.
std::string OsCmd::execute(std::string cmd, bool wait4answ=true)
{
    char buffer[3072];//3k buffer... should be enough!
    std::string out;
    if(cmd[cmd.length()-1]!='\n') cmd+='\n';
    sysOut = popen(cmd.c_str(), "r");
    if (!sysOut) {
        ziblog(LOG::ERR, "system error!");
        exit(-1);
    }
    if(wait4answ) while(fgets(buffer, sizeof(buffer), sysOut)) out +=buffer;
    pclose(sysOut);
    return (wait4answ ? out.substr (0,out.length()-1) : out);
}
//-------------------------------------------------------------------------------------------
std::vector<std::string> OsCmd::getSerialPortList()
{
    std::vector<std::string> result;

    //TODO

    /*
    std::string tmp = execute("ls /dev/ttyS* 2>/dev/null");//serial port list
    std::stringstream lsdevttysxResult(tmp);
    while(lsdevttysxResult >> tmp) result.push_back(tmp);
    tmp = execute("ls /dev/ttyUSB* 2>/dev/null");//USB ftdi serial port list
    std::stringstream lsdevttyusbxResult(tmp);
    while(lsdevttyusbxResult >> tmp) result.push_back(tmp);
    tmp = execute("ls /dev/ttyACM* 2>/dev/null");//USB cdc_acm serial port list
    std::stringstream lsdevttyacmxResult(tmp);
    while(lsdevttyacmxResult >> tmp) result.push_back(tmp);
    */
    return result;
}
//-------------------------------------------------------------------------------------------
currentDateTime::currentDateTime()
{
	SYSTEMTIME now;
	GetLocalTime(&now);
    year=now.wYear;
    month=now.wMonth;
    day=now.wDay;
    hour=now.wHour;
    min=now.wMinute;
    sec=now.wSecond;
}
//-------------------------------------------------------------------------------------------
std::vector<unsigned char> serializeStdString (const std::string& o)
{
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
    if(commentTags.empty()) {//by default, unless otherwise specified, all lines starting with " #" or " ; " are comments
        this->commentTags.push_back('#');
        this->commentTags.push_back(';');
    }
}

void configFileHandler::read()
{
    std::ifstream cf(configFile.c_str());
    if(!cf) {
        ziblog(LOG::ERR, "configuration file could not be opened (%s)", strerror(errno));
        return;
    }
    std::string s, key, value;
    while (std::getline(cf, s)) {
        std::string::size_type begin=s.find_first_not_of(" \f\t\v");
        if(begin==std::string::npos) continue;//skip of empty lines
        if(std::string(commentTags.begin(), commentTags.end()).find(s[begin])!=std::string::npos) continue;//skip of comments
        //parameter name extraction (key)
        std::string::size_type end=s.find('=', begin);
        key=s.substr(begin, end-begin);
        key.erase(key.find_last_not_of(" \f\t\v")+1);//deletion of any leading and trailing spaces
        if(key.empty()) continue;//ignore of any empty key value
        //parameter value extraction
        begin=s.find_first_not_of(" \f\n\r\t\v", end+1);
        end=s.find_last_not_of(" \f\n\r\t\v")+1;
        value=s.substr(begin, end-begin);
        //insertion of the (key, value) pair into the map
        configData[key]=value;
    }
}

void configFileHandler::write()
{
    std::ofstream cf(configFile.c_str(), std::fstream::trunc);
    if(!cf) {
        ziblog(LOG::ERR, "configuration file could not be opened (%s)", strerror(errno));
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

void configFileHandler::insertCommentHeaderRaw(std::string commentRaw)
{
    std::string::size_type begin=commentRaw.find_first_not_of(" \f\t\v");
    if(std::string(commentTags.begin(), commentTags.end()).find(commentRaw[begin])==std::string::npos) {
        std::string commentTag(1, commentTags.front());
        commentRaw = commentTag + " " + commentRaw;
    }
    commentHeaderRaws.push_back(commentRaw);
}


void configFileHandler::addOrUpdateKeyValue(const std::string& parameter, const std::string& value)
{
    configData[parameter]=value;//update of existing parameter, if any, or creation of new entry
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
