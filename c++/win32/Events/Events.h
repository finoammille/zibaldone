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

    StopThreadEvent MUST be handled by all threads: when a thread receives StopThreadEvent
    MUST terminate what was doing and exit as soon as possible (obviously in a fair way, that
    is freeing allocated memory, ...)

    StopThreadEvent is the only event whose label is reserved and cannot be used for other
    events

    :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

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

    RawByteBufferData implements a generic event made up by a byte buffer. The semantic meaning
    is left to the user.

    :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

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

    the purpose of this event is notify an info message by means of a string. The structure if
    InfoMsg event is identical to that of zibErr event, but the use purpose is different (zibErr
    is meant to notify an error)

    :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
    
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
};
//-------------------------------------------------------------------------------------------
/*
    TimeoutEvent
    
    Note: timerId is identified by the label

    :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
    
    nota: la label identifica il timerId
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

    the purpose of this event is to notify a generic error
    
    :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
    
    serve per notificare un errore generico.
*/

class zibErr : public Event {
    std::string _errorMsg;
    virtual Event* clone()const;
public:
    std::string errorMsg() const;
    zibErr(const std::string&, const std::string&);
};
//-------------------------------------------------------------------------------------------
}//namespace Z
#endif	/* _EVENT_H */

/*
Tips:

1) It's not necessary to have a thread to emit an event (although, of course, it's reasonable
   there is almost one receiver!). All the threads who have registered to an event will receive
   a copy of that event in their event queue, whenever someone somewhere calls emitEvent() method
   on that event. A thread can register to an event (thus will receive a copy of it when it's
   emitted) by calling register2Label method and specifying the string label that uniquely identifies
   the event (the string passed to the constructor of that event). A thread can check out an event
   from his event queue by calling the method pullout. Once a thread has taken an event, it's his
   responibility to delete it after use.

2) Remember that a thread receives all the event whose string identifier are equal to the label
   it has registered to, no matter what type are that events. This means that if more events share
   the same label, they all will be received and then we can discriminate between them by means of a
   dynamic_cast operation. Similarly a single object ot type Event can be used for different events
   using different label (for example the event ``RawByteBufferData`` is used by SerialPortHandler
   as well as TcpIpc, ...)

3) Event class is an abstract interface (it cannot be directly instantiated). To create a new event
   you have to inherit the Event class and define the pure virtual method clone that makes a deep
   copy of the event data (your inherited class). The best way to accomplish this requirement is to
   use the well known clone design pattern, e.g. to clone the class foo:

   foo* clone()const{return new foo(*this);}

   remember that this pattern works correctly provided that you comply to the well known "big 3 rule
   of C++" that is if your class needs to define the destructor or the copy constructor or the copy
   assigment operator then it should probably explicitly define all three.

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

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




