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

import java.nio.ByteBuffer;
import java.util.*;


//---------------------------------------------------------------------------------------------------------------------
/*
    RawByteBufferData

    RawByteBufferData implements a generic event made up by a byte buffer. The semantic meaning
    is left to the user.

    Warning! all get methods return a (deep) copy of their stuff (for example the buf() method
             returns a deep copy of the _buf member variable. This means that if someone wants 
             to perform some cumulative actions to a got stuff it has to get it once and then
             do whatever he has to do to the same object because if not each action will be done
             to a different object copy!
             
             Example:

             the following code will correctly iterate over the ByteBuffer content of the event:
             
             ....
             RawByteBufferData rbbd;
             ByteBuffer data=rbbd.buf();
             data.rewind();
             while(data.hasRemaining())
                 ...
                 System.out.println((char)data.get());
                 ...
             }

             The following code will instead rewind an object and then iterate over another object!

             ...
             RawByteBufferData rbbd;
             rbbd.buf().rewind();//rewind() is called on a copy of _buf member variabile
             while(rbbd.buf().hasRemaining())//hasRemaining() is called on another copy
                                             //of _buf different than the previous one
                 ...
                 System.out.println((char)rbbd.buf().get());//get() is callend on another
                                                            //additional copy different
                                                            //from both the previous
                 ...
             }


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

    Attenzione!   Tutti i metodi get ritornano una (deep) copy! Quindi se voglio eseguire delle azioni 
                  cumulative su un oggetto ottenuto tramite una get, devo fare la get una volta sola e
                  "salvato" il reference su una variabile agire sempre su quest'ultima. In caso contrario
                  (se chiamo get ogni volta) ogni azione sara` eseguita su una copia diversa dalla precedente!
                  
                  Esempio:

                  Il codice che segue itera correttamente sul contenuto ByteBuffer dell'evento:

                  ....
                  RawByteBufferData rbbd;
                  ByteBuffer data=rbbd.buf();
                  data.rewind();
                  while(data.hasRemaining())
                      ...
                      System.out.println((char)data.get());
                      ...
                  }

                  Il codice seguente invece applica i metodi rewind, hasRemaining e get su tre diverse
                  copie di ByteBuffer:

                  ...
                  RawByteBufferData rbbd;
                  rbbd.buf().rewind();//rewind() is called on a copy of _buf member variabile
                  while(rbbd.buf().hasRemaining())//hasRemaining() is called on another copy
                                                  //of _buf different than the previous one
                      ...
                      System.out.println((char)rbbd.buf().get());//get() is callend on another
                                                                 //additional copy different
                                                                 //from both the previous
                      ...
                  }

                  oltretutto in questo caso specifico, il ciclo restera` bloccato indefinitamente!
                  Infatti ogni volta applico hasRemaining su una nuova copia, ma continuero` a fare
                  get del primo elemento di un'altra nuova copia!
*/

public class RawByteBufferData extends Event {
    private ByteBuffer _buf;
    public ByteBuffer buf() 
    {
        ByteBuffer buf = ByteBuffer.allocate(_buf.capacity());
        buf.put(_buf.array());
        return buf;
    }
    public int len() {return _buf.flip().capacity();}
    public RawByteBufferData(String label, ByteBuffer buf)
    {
        super(label);//note: deep copy of label is done by super (Event) constructor
        _buf= ByteBuffer.allocate(buf.capacity());
        _buf.put(buf.array());
    }
    public RawByteBufferData(String label, ArrayList<Byte> buf)
    {
        super(label);//note: deep copy of label is done by super (Event) constructor
        _buf= ByteBuffer.allocate(buf.size());
        for(int i=0; i<buf.size(); i++) _buf.put(buf.get(i).byteValue());
    }
}
//---------------------------------------------------------------------------------------------------------------------
