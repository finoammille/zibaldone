/*
 *
 * zibaldone - a C++/Java library for Thread, Timers and other Stuff
 *
 * http://sourceforge.net/projects/zibaldone/
 *
 * version 3.1.2, August 29th, 2015
 *
 * Copyright (C) 2012  bucc4neer (bucc4neer@users.sourceforge.net)
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
 * bucc4neer bucc4neer@users.sourceforge.net
 *
 */

package Z;
//----------------------------------------------------------------------------------------------------------------------
/*
    InfoMsg

    the purpose of this event is notify an info message by means of a string. The structure if InfoMsg event is
    identical to that of zibErr event, but the use purpose is different (zibErr is meant to notify an error)

    Warning! the msg() get method returns a deep copy of the _msg member variabile, not a reference to it. This means 
    that to perform cumulative actions on the String got by calling msg(), we have first to store the reference of
    the obtained copy in a String variabile and then operate on that variabile! Infact each operation done directly 
    on the result of a msg() call is actually performed each time on a different copy of _msg

    ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

    InfoMsg e` un evento il cui scopo e` unicamente quello di notificare un messaggio di info sotto forma di stringa.
    Sintaticamente e` identico a zibErr, ma il significato semantico e` profondamente diverso (non e` un messaggio di
    errore!). E` utile per esempio quando serve notificare un certo avvenimento senza dati associati. Per esempio un ack
    di ricezione, ...

    Ho deciso di non usare la label come messaggio informativo per non mescolare il concetto di label con il contenuto
    informativo (per esempio possono esserci ack relativi a diversi messaggi e in tal caso posso per esempio usare la
    label per indicare che si tratta di un ack e msg di per indicare a quale messaggio si riferisce l'ack)

    Attenzione! il metodo get msg() ritorna una deep copy della variabile membro _msg, non un reference ad esso. 
    Questo significa che per eseguire azioni cumulative sulla stringa ottenuta chiamando msg(), dobbiamo prima memorizzare 
    il reference della copia ottenuta in una variabile String e quindi operare su tale variabile! Infatti ogni operazione 
    eseguita direttamente sul risultato di una chiamata msg () viene eseguita ogni volta su una diversa copia di _msg
*/
public class InfoMsg extends Event {
    private String _msg;
    public String msg() {return new String(_msg);}
    public InfoMsg(String label, String msg)
    {
        super(label);//note: deep copy of label is done by super (Event) constructor
        _msg=new String(msg);
    }
}
//----------------------------------------------------------------------------------------------------------------------
