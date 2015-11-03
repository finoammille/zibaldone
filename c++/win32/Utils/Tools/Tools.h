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

#ifndef _TOOLS_H
#define	_TOOLS_H

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <iostream>
#include <sstream>
#include <vector>
#include <algorithm>
#include <iterator>
#include <fstream>
#include <map>

#if defined(_MSC_VER)
#define popen _popen
#define pclose _pclose
#endif

namespace Z
{
//-------------------------------------------------------------------------------------------
class OsCmd {
    FILE* sysOut;
    std::string execute(std::string, bool);
public:
    std::vector<std::string> getSerialPortList();
};
//-------------------------------------------------------------------------------------------
class currentDateTime {
    int year;
    int month;
    int day;
    int hour;
    int min;
    int sec;
public:
    currentDateTime();
    int Year() const {return year;}
    int Month() const {return month;}
    int Day() const {return day;}
    int Hour() const {return hour;}
    int Min() const {return min;}
    int Sec() const {return sec;}
};
//-------------------------------------------------------------------------------------------
/*
    SERIALIZATION
    The template functions "serialize" and "deserialize" can be used to serialize an object
    and insert it into an event payload.

    LIMITS:

    DONT USE IF:

    1) the object allocates heap memory

    2) the object is to be transmitted via socket

    infact current implementation:

    1) does not handle a "deep serialization" that is the serialization of data pointed by
       a member of the serialized object. A possible pointer will be inserted into the
       serializad byte stream as is, like any other member and after the serialization it
       have to be ensured there are not dangling reference
       A safe way to complain the above condition is use serialize/deserialize only with
       built-in data type and class/structures containing only built-in data types (int,
       char, ...)

    2) object serialization works only for event within a process (that is between threads in
       the same process). the sent of serialization data via socket is not supported because
       there is no warrant about how the data are used on peer socket side.
*/

template <class T>
std::vector<unsigned char> serialize (const T & o)
{
    size_t size=sizeof(o);
    unsigned char* buffer = new unsigned char[size];
    memcpy(buffer, &o, size);
    std::vector<unsigned char> stream;
    for(size_t i=0; i<size; i++) stream.push_back(buffer[i]);
    return stream;
}

template <class T>
T deserialize (const unsigned char* optr)
{
    return T(*((T*)optr));
}

template <class T>
T deserialize (std::vector<unsigned char>& v)
{
    T ret = T(*((T*)(&v[0])));
    v.erase(v.begin(),v.begin()+(int)(sizeof(T)));
    return ret;
}

std::vector<unsigned char> serializeStdString (const std::string &);

std::string deserializeStdString (std::vector<unsigned char>&);
//-------------------------------------------------------------------------------------------
// CONFIGURATION FILE FACILITIES

/*

configFileHandler handles a configuration file with the following format

<setting>=<value>

for example:
port.name=/dev/ttyS0
port.speed=115200
...

in the example above, the parameter name includes the "." but configFileHandles considers
"port.name" and " port.speed" as two different independent parameters.

Anyway such a notation is useful to ease the comprehension of the configuration file
parameters grouping them in a structured data organization

*/

class configFileHandler {
    std::string configFile;//configuration file name including path
    std::map <std::string, std::string> configData;//configuration data (key-value map)
    std::vector<char> commentTags;//character list that can be indifferently used at the beginning of a line to identify a comment
    std::vector<std::string> commentHeaderRaws;//comment lines that will be inserted at the beginning of the configuration file.
                                               //A tipical use is for some explanation about the configuration file parameters and
                                               //the allowed values for them
public:
    //it is possible to pass to the constructor a new list parameter-value that will override the existing one
    configFileHandler(std::string configFile,
                      std::map<std::string, std::string> configData=std::map<std::string, std::string>(),
                      std::vector<char> commentTags=std::vector<char>());
    void read();//this method reads configuration file data. It creates the file if it does not exist
    void write();//this method writes configData map in the configuration file. It creates the file if it does not exist
    std::map<std::string, std::string> getConfigData();//this method returns the whole config data map
    void addOrUpdateKeyValue(const std::string&, const std::string&);//this method inserts the pair (key-value) into the
                                                                     //configuration file or updates it, if that pair is
                                                                     //present  yet
    void insertCommentHeaderRaw(std::string);//this method insert a new comment line in the configuration file header
    std::string getRawValue(const std::string &);//this method returns the raw value of a parameter that is exactly the
                                                 //string in the value field of the corresponding specified key.
                                                 //If the requested key is not present, it returns an empty string.
    bool getBoolValue(bool& dest, std::string param);//this method looks for the value corresponding to the key "param".
                                                     //If value is equal to the literal string "true" then the reference
                                                     //boolean variable dest is set to true and the method returns true,
                                                     //indicating the key was found and correctly interpretated.
                                                     //Similarly, if value is equal to the literal string "false" then the
                                                     //reference boolean variable dest is set to false and the method
                                                     //returns true, indicating the key was found and correctly interpretated.
                                                     //If the key param was not found in the configuration file or if the
                                                     //value is a literal string other than "true" or "false" then dest is
                                                     //left as is and the method returns false (fail) to warn there is
                                                     //something wrong (the parameter key is not present in the configuration
                                                     //file or the corresponding value is different than "true" or "false")
    bool getIntValue(int& dest, std::string param);//this method looks for the value corresponding to the key "param". If value
                                                   //is a string corresponding to an integer value (atoi), then the int reference
                                                   //variable dest will contain that integer value and the method returns true,
                                                   //indicating the key was found and correctly interpretated. If the key param
                                                   //is not present in the configuration file or the relative value cannot be
                                                   //interpreded as an integer number, then dest is left as is and the method
                                                   //returns false (fail) to warn there is something wrong
};
//-------------------------------------------------------------------------------------------
}//namespace Z
#endif	/* _TOOLS_H */
