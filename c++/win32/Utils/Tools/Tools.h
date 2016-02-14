/*
 *
 * zibaldone - a C++/Java library for Thread, Timers and other Stuff
 *
 * http://sourceforge.net/projects/zibaldone/
 *
 * version 3.2.0, February 14th, 2016
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
    long getFileSizeInBytes(const std::string&);
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

    2) object serialization works only for event within a process (that is between threads in
       the same process). the sent of serialization data via socket is not supported because
       there is no warrant about how the data are used on peer socket side.

    :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

    SERIALIZATION
    le funzioni template "serialize" e "deserialize" permettono di serializzare un oggetto
    in modo da poterlo inserire facilmente nel payload di un evento.

    LIMITI:

    NON USARE SE:

    1) l'oggetto alloca memoria heap

    2) l'oggetto deve essere trasmesso su un socket.

    Infatti, per semplificare l'implementazione:

    1) non viene gestita la "deep serialization" ovvero serializzazione dei dati puntati
       da un membro della classe serializzata. Un eventuale puntatore viene inserito nello
       stream come un membro qualsiasi e dopo la deserializzazione occorrerebbe garantire
       l'assenza di dangling reference.

    2) ovviamente la serializzazione funziona solo per gli eventi all'interno di un thread.
       L'invio attraverso un socket non e` supportato perche` non c'e` garanzia sul tratta-
       mento dei dati dall'altra parte del socket. Infatti l'allineamento dei dati potrebbe
       essere diverso e quindi la deserialization non funzionerebbe correttamente. Teorica-
       mente sarebbe possibile usare serialization con un socket solo se dall'altra parte
       del socket c'e` una macchina identica a quella che trasmette, in modo che la ricostru-
       zione dell'oggetto sia identica!
*/

//serialize funziona solo con i dati built in e le strutture/classi contenenti solo dati built in.
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

//come il precedente ma utilizzabile su un bytestream contenente piu` oggetti contenenti solo tipi builtIn
//serializzati uno dietro l'altro. modifica il vector ricevuto in ingresso rimuovendo la parte deserializzata
template <class T>
T deserialize (std::vector<unsigned char>& v)
{
    T ret = T(*((T*)(&v[0])));
    v.erase(v.begin(),v.begin()+(int)(sizeof(T)));
    return ret;
}

//serializeStdString permette di serializzare una std::string
std::vector<unsigned char> serializeStdString (const std::string &);

//deserializeStdString e` l'inversa di serializeStdString: ricostruisce una
//std::string serializzata precedentemente con serializeStdString. La funzione
//restituisce quello che resta dell'std::vector in ingresso tolta la stringa
//estratta (nel caso limite, un std::vector vuoto se questo conteneva solo la stringa)
//in questo modo e` possibile estrarre da una serializzazione eterogenea la stringa
//serializzata e continuare la deserializzazione degli altri tipi!
//In altre parole l'std::vecton in ingresso deve iniziare con una stringa serializzata
//precedentemente con serializeStdString ma a seguire puo` contenere qualsiasi altro dato
//serializzato!
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

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

configFileHandler gestisce un file di configurazione avente il formato

<setting>=<value>

Per esempio:
port.name=/dev/ttyS0
port.speed=115200
...

il nome del parametro e` tutto compreso il "." ma e` utile utilizzare una notazione di questo tipo
per facilitare la comprensione del file di configurazione organizzandolo in gruppi di dati.

*/

/*
Occorre passare al costruttore:
nome del file di configurazione (stringa con path)
carattere x comment lines (il default = #)
*/

class configFileHandler {
    std::string configFile;//configuration file name including path
                           //nome completo di path del file di configurazione
    std::map <std::string, std::string> configData;//configuration data (key-value map)
                                                   //dati di configurazione (mappa key-value)
    std::vector<char> commentTags;//character list that can be indifferently used at the beginning of a line to identify a comment
                                  //lista dei caratteri che identificano un commento.
    std::vector<std::string> commentHeaderRaws;//comment lines that will be inserted at the beginning of the configuration file.
                                               //A tipical use is for some explanation about the configuration file parameters and
                                               //the allowed values for them
                                               //::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
                                               //righe di commento a inizio file (tipicamente elenco valori, spiegazioni....)
public:
    //it is possible to pass to the constructor a new list parameter-value that will override the existing one
    //e` possibile passare al costruttore una nuova lista di parametri-valore con cui sovrascrivere quella esistente
    configFileHandler(std::string configFile,
                      std::map<std::string, std::string> configData=std::map<std::string, std::string>(),
                      std::vector<char> commentTags=std::vector<char>());
    void read();//this method reads configuration file data. It creates the file if it does not exist
                //legge i dati di configurazione dal file. Se il file non esiste, lo crea
    void write();//this method writes configData map in the configuration file. It creates the file if it does not exist
                 //scrive i dati di configurazione nel file. Se il file non esiste, lo crea
    std::map<std::string, std::string> getConfigData();//this method returns the whole config data map
                                                       //restituisce l'intera mappa
    void addOrUpdateKeyValue(const std::string&, const std::string&);//this method inserts the pair (key-value) into the
                                                                     //configuration file or updates it, if that pair is
                                                                     //present  yet
                                                                     //::::::::::::::::::::::::::::::::::::::::::::::::::
                                                                     //inserisce nel file di configurazione la coppia (key-value)
                                                                     //o se e` gia` presente la aggiorna con il nuovo valore.
    void insertCommentHeaderRaw(std::string);//this method insert a new comment line in the configuration file header
                                             //inserisce il commento nell'intestazione del file
    std::string getRawValue(const std::string &);//this method returns the raw value of a parameter that is exactly the
                                                 //string in the value field of the corresponding specified key.
                                                 //If the requested key is not present, it returns an empty string.
                                                 //::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
                                                 //restituisce il valore "raw" di un parametro cioe` esattamente la corrispondente
                                                 //stringa nel file di configurazione. Se il parametro non e` presente, il metodo
                                                 //ritorna una stringa vuota.
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
                                                     //::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
                                                     //se la key string param e` presente, il parametro input bool dest conterra`
                                                     //true o false a seconda se la corrispondente string value e` = a "true" o
                                                     //"false" e il metodo ritorna true (success). Se la key string param non e`
                                                     //presente nei dati di configurazione oppure se la corrispondente string
                                                     //value ha un valore diverso da "true" o "false" il metodo ritorna false
                                                     //(fail) e il valore del parametro dest non viene toccato
    bool getIntValue(int& dest, std::string param);//this method looks for the value corresponding to the key "param". If value
                                                   //is a string corresponding to an integer value (atoi), then the int reference
                                                   //variable dest will contain that integer value and the method returns true,
                                                   //indicating the key was found and correctly interpretated. If the key param
                                                   //is not present in the configuration file or the relative value cannot be
                                                   //interpreded as an integer number, then dest is left as is and the method
                                                   //returns false (fail) to warn there is something wrong
                                                   //:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
                                                   //se la key string param e` presente, il parametro input int dest conterra` la
                                                   //conversione (atoi) ad int della stringa value corrispondente e il metodo ritorna
                                                   //true (success). Se la key string param non e` presente nei dati di  configurazione
                                                   //oppure se la corrispondente string value ha un valore non  convertibile ad int il
                                                   //metodo ritorna false (fail) e il valore  del parametro dest non viene toccato
};
//-------------------------------------------------------------------------------------------
}//namespace Z
#endif	/* _TOOLS_H */
