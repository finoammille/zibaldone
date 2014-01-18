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
class zibTools {
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

    2) object serialization works only for event within a process (that is between threads in
       the same process). the sent of serialization data via socket is not supported because
       there is no warrant about how the data are used on peer socket side. 

-------------------------------------------------------------------------------------------

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

gestisce un file di configurazione avente il formato

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
    std::string configFile;//file di configurazione
    std::map <std::string, std::string> configData;//dati di configurazione (mappa key-value)
    std::vector<char> commentTags;//lista dei caratteri che identificano un commento. 
public:
    //e` possibile passare al costruttore una nuova lista di parametri-valore con cui sovrascrivere quella esistente
    configFileHandler(std::string configFile, 
                      std::map<std::string, std::string> configData=std::map<std::string, std::string>(),
                      std::vector<char> commentTags=std::vector<char>());
    void read();//legge i dati di configurazione dal file
    void write();//scrive i dati di configurazione nel file
    std::map<std::string, std::string> getConfigData();//restituisce l'intera mappa
    void addOrUpdateKeyValue(const std::string&, const std::string&);//inserisce nel file di configurazione la coppia (key-value)
                                                                     //o se e` gia` presente la aggiorna con il nuovo valore.
    std::string getRawValue(const std::string &);//restituisce il valore "raw" di un parametro cioe` esattamente la corrispondente 
                                              //stringa nel file di configurazione. Se il parametro non e` presente, il metodo 
                                              //ritorna una stringa vuota.
    bool getBoolValue(bool& dest, std::string param);//se la key string param e` presente, il parametro input bool dest conterra` 
                                                     //true o false a seconda se la corrispondente string value e` = a "true" o 
                                                     //"false" e il metodo ritorna true (success). Se la key string param non e` 
                                                     //presente nei dati di configurazione oppure se la corrispondente string 
                                                     //value ha un valore diverso da "true" o "false" il metodo ritorna false 
                                                     //(fail) e il valore del parametro dest non viene toccato
    bool getIntValue(int& dest, std::string param);//se la key string param e` presente, il parametro input int dest conterra` la
                                                   //conversione (atoi) ad int della stringa value corrispondente e il metodo ritorna
                                                   //true (success). Se la key string param non e` presente nei dati di  configurazione 
                                                   //oppure se la corrispondente string value ha un valore non  convertibile ad int il 
                                                   //metodo ritorna false (fail) e il valore  del parametro dest non viene toccato
};
//-------------------------------------------------------------------------------------------
}//namespace Z
#endif	/* _TOOLS_H */
