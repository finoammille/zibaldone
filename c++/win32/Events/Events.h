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

#ifndef _EVENT_H
#define	_EVENT_H

//-------------------------------------------------------------------------------------------
#include "Thread.h"
//-------------------------------------------------------------------------------------------

namespace Z
{
//-------------------------------------------------------------------------------------------
/*
    StopThreadEvent

    L'evento StopThreadEvent deve essere OBBLIGATORIAMENTE gestito da tutti i thread: quando
    un thread riceve l'evento StopThreadEvent DEVE terminare e uscire prima possibile.

    E` l'unico evento la cui label e` riservata e non puo` essere utilizzata per altri eventi.

*/
const std::string StopThreadLabel = "StopThreadEvent";

class StopThreadEvent : public Event {
    Event* clone()const{return new StopThreadEvent(*this);}
public:
    StopThreadEvent():Event(StopThreadLabel){}
};
//-------------------------------------------------------------------------------------------
/*
    RawByteBufferData

    RawByteBufferData e` un tipo particolare di Event che implementa un generico evento costituito 
    da un buffer di byte identificato da una stringa. La semantica del buffer e` delagata all'utiliz-
    zatore, RawByteBufferData si fa carico solo di contenere i byte in una struttura compatibile con 
    il framework della gestione degli eventi. In particolare RawByteBufferData fa una propria copia 
    del buffer passatogli tramite il costruttore ed e` pertanto compito del chiamante liberare, se 
    necessario, la memoria dopo aver passato i byte RawByteBufferData.

    Osservazione: RawByteBufferData essendo un generico buffer di byte consente di fare praticamente 
                  tutto! Infatti il contenuto potrebbe essere l'elenco dei parametri da passare al 
                  costruttore di una classe identificata univocamente dalla label, oppure potrebbe 
                  contenere qualsiasi cosa, un immagine, un file.....
                  
                  ATTENZIONE!! label PUO` identificare univocamente un tipo, non DEVE! Se cosi` si 
                  decide (ma e` una SCELTA possibile, un'idea di utilizzo, non una regola!) ovvero se
                  la label non e` associata ad altri eventi) allora posso usare l'evento per ricostruire 
                  un oggetto istanza della classe che ho associato deserializzando il contenuto nei para-
                  metri del costruttore. Questa idea di utilizzo ovviamente funziona a patto che la label 
                  non sia associata ad altro! Ma come detto e` un'idea per un possibile utilizzo, non una 
                  regola, deto che invece in generale una stessa label puo` identificare diversi eventi e 
                  la discriminazione avviene tramite il dynamic_cast. 
*/

class RawByteBufferData : public Event {
    unsigned char* _buf;
    int _len;
    virtual Event* clone()const;
public:
    unsigned char* buf() const;
    int len() const;
    RawByteBufferData(const std::string& label, const unsigned char* buf, const int len);
    RawByteBufferData(const std::string& label, const std::vector<unsigned char>&);
    RawByteBufferData(const RawByteBufferData &);
    ~RawByteBufferData();
    RawByteBufferData & operator = (const RawByteBufferData &);
};
//-------------------------------------------------------------------------------------------
/*
    InfoMsg

    InfoMsg e` un evento il cui scopo e` unicamente quello di notificare un messaggio di info
    sotto forma di stringa. Sintaticamente e` identico a zibErr, ma il significato semantico e`
    profondamente diverso (non e` un messaggio di errore!). E` utile per esempio quando serve 
    notificare un certo avvenimento senza dati associati. Per esempio un ack di ricezione, ...

    Ho deciso di non usare la label come messaggio informativo per non mescolare il concetto di
    label con il contenuto informativo (per esempio possono esserci ack relativi a diversi mes-
    saggi e in tal caso posso per esempio usare la label per indicare che si tratta di un ack e 
    msg di per indicare a quale messaggio si riferisce l'ack)
*/

class InfoMsg : public Event {
    std::string _msg;
    virtual Event* clone()const;
public:
    std::string msg() const;
    InfoMsg(const std::string&, const std::string&); 
    InfoMsg(const InfoMsg &);
    ~InfoMsg();
    InfoMsg & operator = (const InfoMsg &);
};
//-------------------------------------------------------------------------------------------
/*
    TimeoutEvent

    la label identifica il timerId
*/
class TimeoutEvent : public Event {
    Event* clone()const{return new TimeoutEvent(*this);}
public:
    std::string timerId() const {return _label;}//rendo piu` esplicito che qui _label ha significato concettuale di timerId 
    TimeoutEvent(std::string timerId):Event(timerId){}
};
//-------------------------------------------------------------------------------------------
/*
    zibErr

    serve per notificare un errore generico.
*/

class zibErr : public Event {
    std::string _errorMsg;
    virtual Event* clone()const;
public:
    std::string errorMsg() const;
    zibErr(const std::string&, const std::string&); 
    zibErr(const zibErr &);
    ~zibErr();
    zibErr & operator = (const zibErr &);
};
//-------------------------------------------------------------------------------------------
}//namespace Z
#endif	/* _EVENT_H */

/*
Osservazioni: 

1) L'emissione di un evento non e` legata alla presenza di un thread! (anche se e` ovviamente 
   ragionevole che ci sia almeno un ricevitore!),  per emettere un evento basta chiamare il me-
   todo ''emitEvent''. Quando viene chiamato questo metodo, il gestore degli eventi (event manager) 
   si fa carico di fare una copia indipendente dell'evento emesso sulla coda di ogni thread che si 
   era precedentemente registrato dichiarandosi interessato all'evento stesso. Questo approccio ha 
   il pregio della semplicita` ma per contro occorre ricordare che l'individuazione di un evento su
   cui registrarse avviene SOLO TRAMITE LA LABEL per cui bisogna fare attenzione alle label degli 
   eventi quando un thread si registra per riceverli (lo fa dicendo le label degli eventi che vuole
   ricevere): registrarsi su una label sbagliata (magari a causa di una lettera minuscola/maiuscola) 
   causa la non ricezione dell'evento associato!

2) Un evento è etichettato (ai fini della registrazione per la ricezione) da una stringa ed e` un 
   oggetto qualsiasi che deriva da Event. E` importante sottolineare che la stringa label e` (come 
   dice il nome) un'etichetta, NON IDENTIFICA UNIVOCAMENTE L'EVENTO! una stessa etichetta puo` infatti 
   essere associata ad un insieme di eventi legati concettualmente in qualche modo (per esempio gli 
   eventi che rappresentano un insieme di operazioni che possono essere richieste ad un thread). Per 
   IDENTIFICARE UNIVOCAMENTE UN EVENTO occorre controllare il tipo dell'evento ricevuto tramite il
   dynamic_cast!

3) La classe Event e` un'interfaccia astratta (non e` possibile istanziare un oggetto Event) e quando
   si definisce un evento derivando da Event, E` obbligatorio implementare il metodo clone(), dichiarato 
   per questo motivo puramente virtuale. Il metodo clone() deve fare una deep copy dell'oggetto. Per 
   fissare le idee, detta "Foo" la classe che deriva da Event e implementa l'evento, se si rispetta la 
   regola del big 3 (se  si definisce uno qualsiasi tra distruttore, costruttore di copia, overload dell'
   operatore di assegnazione allora occorre quasi sicuramente definire anche gli altri due) allora il me-
   todo clone puo` banalmente essere definito nella classe derivata come:

   Foo* clone()const{return new Foo(*this);}

   ATTENZIONE! l'overload dell'operatore di assegnazione cosi` come il costruttore di copia devono fare 
   la deep copy dei dati dell'oggetto che deriva da Event e devono copiare la label altrimenti l'evento 
   una volta copiato o assegnato non sarà più emittibile perche` eventManager non sara` piu` in grado di
   notificare l'evento non conoscendo la sua label!

   In altri termini:

   sia Ev la classe derivata da Event. 
   
   L'overload del costruttore di copia (se necessario) dovrà avere la forma:

   Ev::Ev(const Ev& obj):Event(obj)
   {
       ...
   }

   oppure, 

   Ev::Ev(const Ev& obj):Event(_label)
   {
       ...
   }

   Per quando riguarda invece l'overload dell'operatore di assegnazione, la forma dovrà essere:

   Ev & Ev::operator = (const Ev &src)
   {
       if(this == &src) return *this; //self assignment check...
       Event::operator= (src);

       ...

   }

   oppure

   Ev & Ev::operator = (const Ev &src)
   {
       
       _label=src._label;

       ...

   }

4) N.B.:

   => Un thread riceve TUTTI gli eventi con label UGUALE a quelle su cui si e` registrato (e` evidente 
      che deve sapere a cosa corrisponde la label su cui si sta registrando!!!!)

   => una label puo` identificare eventi diversi (la scelta e` all'utilizzatore)

   => quando un thread riceve un evento DEVE utilizzare il dynamic_cast per stabilire il tipo 
      di evento (ovvero quale classe derivata da Event) in modo da accedere correttamente ai campi 
      dell'oggetto ricevuto.
*/




