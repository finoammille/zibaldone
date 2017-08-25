/*
 *
 * zibaldone - a C++/Java library for Thread, Timers and other Stuff
 *
 * http://sourceforge.net/projects/zibaldone/
 *
 * version 3.2.0, February 14th, 2016
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

import java.net.*;
import java.util.*;
import java.nio.ByteBuffer;
import java.io.*;
import java.lang.*;

public class Tools {
//-------------------------------------------------------------------------------------------
    public static class OsCmd {
        private void execute(String cmd)
        {
            Runtime rt = Runtime.getRuntime();
            try {
                String shell;
                String shellParameter;
                String thisOs=System.getProperty("os.name");
                if(thisOs.toLowerCase().contains("linux")) {
                    shell="bash";
                    shellParameter="-c";
                } else if(thisOs.toLowerCase().contains("windows")) {
                    shell="cmd.exe";
                    shellParameter="/c";
                } else {
                    LOG.ziblog(LOG.LogLev.ERR, "this OS (%s) is not currently supported...", thisOs);
                    return;
                }
                Process p = rt.exec(new String[] {shell, shellParameter, cmd});
            } catch(IOException ex) {
                LOG.ziblog(LOG.LogLev.ERR, "Tools.OsCmd.execute(%s) - IOException(%s)", cmd, ex.getMessage());
                return;
            }
        }
        private String execute(String cmd, Integer retCode)
        {
            Runtime rt = Runtime.getRuntime();
            Process p;
            try {
                String shell;
                String shellParameter;
                String thisOs=System.getProperty("os.name");
                if(thisOs.toLowerCase().contains("linux")) {
                    shell="bash";
                    shellParameter="-c";
                } else if(thisOs.toLowerCase().contains("windows")) {
                    shell="cmd.exe";
                    shellParameter="/c";
                } else {
                    LOG.ziblog(LOG.LogLev.ERR, "this OS (%s) is not currently supported...", thisOs);
                    return "OS not supported";
                }
                p = rt.exec(new String[] {shell, shellParameter, cmd});
                p.waitFor();
            } catch(IOException | InterruptedException ex) {
                LOG.ziblog(LOG.LogLev.ERR, "Tools.OsCmd.execute(%s, %d) - %s", cmd, retCode, ex.getMessage());
                return "exception error";
            }
            retCode = p.exitValue();
            java.io.InputStream is = p.getInputStream();
            java.io.BufferedReader reader = new java.io.BufferedReader(new InputStreamReader(is));
            String result = new String();
            String s = null;
            try {
                while ((s = reader.readLine()) != null) result += (s+"\n");
                is.close();
                return result;
            } catch(IOException ex){
                LOG.ziblog(LOG.LogLev.ERR, "IO Exception (%s)", ex.getMessage());
                return "exception error";
            }
        }
        public ArrayList<String> getSerialPortList()
        {
            ArrayList<String> result = new ArrayList<String>();
            Integer retCode = new Integer(-1);
            if((System.getProperty("os.name")).equalsIgnoreCase("linux")) {
                String r = execute("ls /dev/ttyS* 2>/dev/null", retCode);
                String[] rs = r.split("\\s+");
                for(int i=0; i<rs.length; i++) if(!rs[i].isEmpty()) result.add(rs[i]);
            }
            return result;
        }
        public long getFileSizeInBytes(String filename)
        {
            String cmd = new String("stat -c %s ");
            cmd +=filename;
            Integer retCode = new Integer(-1);
            String result = execute(cmd, retCode);
            long len = Long.parseLong(result.trim());
            return len;
        }
    }
//-------------------------------------------------------------------------------------------
    public static class currentDateTime {
        private Calendar cNow;
        public currentDateTime()
        {
            cNow=Calendar.getInstance();
            cNow.setTimeInMillis(System.currentTimeMillis());//unic epoch = msec from 01.01.1970 till now
        }
        public int Year() {return cNow.get(Calendar.YEAR);}
        public int Month() {return cNow.get(Calendar.MONTH);}
        public int Day() {return cNow.get(Calendar.DAY_OF_MONTH);}
        public int Hour() {return cNow.get(Calendar.HOUR);}
        public int Min() {return cNow.get(Calendar.MINUTE);}
        public int Sec() {return cNow.get(Calendar.SECOND);}
    }
//-------------------------------------------------------------------------------------------
/*
    SERIALIZATION
    The template functions "serialize" and "deserialize" can be used to serialize an object
    and insert it into an event payload.
    Most impressive is that the entire process is JVM independent, meaning an object can be 
    serialized on one platform and deserialized on an entirely different platform.

    Notice that for a class to be serialized successfully, two conditions must be met:
    1) The class must implement the java.io.Serializable interface
    2) All of the fields in the class must be serializable. If a field is not serializable, 
       it must be marked transient 

    :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

    SERIALIZATION
    le funzioni template "serialize" e "deserialize" permettono di serializzare un oggetto
    in modo da poterlo inserire facilmente nel payload di un evento.
    Un aspetto particolarmente interessante e` che l'intero processo e` indipendente dalla 
    JVM e quindi un oggetto puo` essere serializzato in una piattaforma e deserializzato in
    una piattaforma totalmente differente.

    Si noti che affinche` una classe possa essere serializzata con successo, devono essere
    rispettate le due condizioni seguenti:
    1) la classe deve implementare l'interfaccia java.io.Serializable
    2) tutti i membri della classe devono essere serializzabili. Se un membro non e`
       serializzabile deve essere marcato con la keyword "transient"
*/
    public static class Serialize<T> {
        public ArrayList<Byte> serializedObj;
        public Serialize (T o) 
        {
            serializedObj=new ArrayList<Byte>();
            try {
                ByteArrayOutputStream buffer = new ByteArrayOutputStream();
                ObjectOutputStream x = new ObjectOutputStream(buffer);
                x.writeObject(o);
                x.close();
                byte[] bbuffer=buffer.toByteArray();    
                for(Byte b : bbuffer) serializedObj.add(new Byte(b));
            } catch (IOException ex) {
                LOG.ziblog(LOG.LogLev.ERR, "Tools.Serialize - %s", ex.getMessage());
            }
        }
    }
    public static class Deserialize<T> {
        public T obj;
        @SuppressWarnings("unchecked")
        public Deserialize(ArrayList<Byte> s)
        {
            ObjectInput in = null;
            try {
                byte[] bbuffer = new byte[s.size()];
                for(int i = 0; i < s.size(); i++) bbuffer[i] = s.get(i).byteValue();
                ByteArrayInputStream bis = new ByteArrayInputStream(bbuffer);
                in = new ObjectInputStream(bis);
                obj = (T) in.readObject();
                if (in != null) in.close();
            } catch (IOException | ClassNotFoundException ex) {
                LOG.ziblog(LOG.LogLev.ERR, "Tools.Deserialize - %s", ex.getMessage());
            }  
        }
    }
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
    public static class configFileHandler {
        private String configFile;//configuration file name including path
                                  //nome completo di path del file di configurazione
        private HashMap<String, String> configData=new HashMap<String,String>();//configuration data (key-value map)
                                                                                //dati di configurazione (mappa key-value)
        private ArrayList<Character> commentTags = new ArrayList<Character>();//character list that can be indifferently used at the beginning of a line to identify a comment
                                                                              //lista dei caratteri che identificano un commento.
        private ArrayList<String> commentHeaderRows = new ArrayList<String>();//header comments
                                                                              //commenti a inizio file

        //-------------------------------------------------------------------------------------------------------------
        //
        // CONSTRUCTOR
        //
        // it is possible to pass to the constructor a new list parameter-value that will override the existing one
        //
        // e` possibile passare al costruttore una nuova lista di parametri-valore con cui sovrascrivere quella esistente
        //
        //-------------------------------------------------------------------------------------------------------------
        public configFileHandler(String ConfigFile, HashMap<String, String> ConfigData, ArrayList<Character> CommentTags)
        {
            if(ConfigFile == null || ConfigFile.isEmpty()) {
                LOG.ziblog(LOG.LogLev.ERR, "invalid file name");
                return;
            }
            configFile=ConfigFile;
            if(ConfigData != null) configData = ConfigData;
            if(CommentTags != null) commentTags=CommentTags;
            if(commentTags.isEmpty()) {
                //by default, unless otherwise specified, all lines starting with " #" or " ; " are comments
                //se non specificato diversamente, di default sono commenti le linee che iniziano con "#" o con ";"   
                commentTags.add('#');
                commentTags.add(';');
            }
        }

        public configFileHandler(String ConfigFile, HashMap<String, String> ConfigData)
        {
            this(ConfigFile, ConfigData, null);
        }

        public configFileHandler(String ConfigFile)
        {
            this(ConfigFile, null, null);
        }

        //-------------------------------------------------------------------------------------------------------------
        //
        // exists
        //
        // exist method checks if the configuration file exists or not (name and path were specified in constructor). 
        //
        // il metodo exists semplicemente controlla se il file di configurazione esiste oppure no (il nome del file
        // completo di path e` stato passato al costruttore)
        //
        //-------------------------------------------------------------------------------------------------------------
        public boolean exists()
        {
            try {
                return (new File(configFile)).exists();
            } catch (SecurityException ex) {
                LOG.ziblog(LOG.LogLev.ERR, "security exception - %s", ex.getMessage());
                return false;
            }
        }

        //-------------------------------------------------------------------------------------------------------------
        //
        // read
        //
        // read method reads configuration file data. It creates the file if it does not exist
        //
        // il metodo read legge i dati di configurazione dal file. Se il file non esiste, lo crea
        //
        //-------------------------------------------------------------------------------------------------------------
        public void read()
        {
            try {
                BufferedReader br = new BufferedReader(new FileReader(configFile));
                String cf_line;//a line of the configFile
                boolean isComment=false;
                while ((cf_line = br.readLine()) != null) {
                    cf_line = cf_line.trim();//delete leading and trailing whitespaces
                    if(cf_line.trim().isEmpty()) continue;//skip empty lines
                    for(Character commentTag : commentTags) {
                        if(cf_line.startsWith((new String()).valueOf(commentTag))) {
                            isComment=true;
                            break;
                        }
                    }
                    if(isComment) {
                        isComment=false;
                        continue;
                    }
                    //se sono qui cf_line e` una riga, senza spazi iniziali e finali, non vuota, non commento
                    String[] keyValue = cf_line.split("=");
                    if(keyValue.length!=2) {
                        //riga non valida: non e` della forma "key=value" (non c'e` "=" o ce c'e` piu` di uno)
                        LOG.ziblog(LOG.LogLev.WRN, "skip of invalid configuration file line (%s)", cf_line);
                        continue;
                    }
                    //se sono qui cf_line e` del tipo "key=value" al netto di spazi interni
                    String key = keyValue[0].trim();
                    String value = keyValue[1].trim();
                    if(key.isEmpty() || value.isEmpty()) {
                        LOG.ziblog(LOG.LogLev.WRN, "skip of empty key (%s), or value (%s)", key, value);
                        continue;
                    }
                    configData.put(key, value);
                }
            } catch (/*FileNotFoundException | */IOException ex) {
                LOG.ziblog(LOG.LogLev.ERR, "read exception - %s", ex.getMessage());
                return;
            }
        }
        
        //-------------------------------------------------------------------------------------------------------------
        //
        // write
        //
        // write method writes configData map in the configuration file. It creates the file if it does not exist
        //
        // il metodo write scrive i dati di configurazione nel file. Se il file non esiste, lo crea
        //
        //-------------------------------------------------------------------------------------------------------------
        public void write()
        {
            try {
                BufferedWriter bw = new BufferedWriter(new FileWriter(configFile));
                for(String commentRow : commentHeaderRows) {
                    bw.write(commentRow);
                    bw.newLine();
                }
                String cf_line;//a line of the configFile
                for(Map.Entry<String, String> entry : configData.entrySet()) {
                    cf_line=(entry.getKey()+"="+entry.getValue());
                    bw.write(cf_line);
                    bw.newLine();
                }
                bw.close();
            } catch (IOException/* | FileNotFoundException*/ ex) {
                LOG.ziblog(LOG.LogLev.ERR, "write exception - %s", ex.getMessage());
                return;
            }
        }

        //-------------------------------------------------------------------------------------------------------------
        //
        // getConfigData
        //
        // this method returns a copy of the whole config data map
        //
        // il metodo restituisce una copia dell'intera mappa 
        //
        //-------------------------------------------------------------------------------------------------------------
        public HashMap<String, String> getConfigData() {return new HashMap<String, String>(configData);}

        //-------------------------------------------------------------------------------------------------------------
        //
        // addOrUpdateKeyValue
        //
        // this method inserts the pair (key-value) into the configuration file or updates it, if that pair is present yet

        // inserisce nel file di configurazione la coppia (key-value) o se e` gia` presente la aggiorna con il nuovo valore.
        //
        //-------------------------------------------------------------------------------------------------------------
        public void addOrUpdateKeyValue(String key, String value)
        {
            if(key != null && value != null) configData.put(key, value);
            else LOG.ziblog(LOG.LogLev.ERR, "key and/or value is null");
        }
        
        //-------------------------------------------------------------------------------------------------------------
        //
        // insertCommentHeaderRaw
        //
        // this method insert a new comment line in the configuration file header
        //
        // inserisce il commento nell'intestazione del file
        //
        //-------------------------------------------------------------------------------------------------------------
        public void insertCommentHeaderRaw(String comment)
        {
            //utilizzo il primo carattere della lista dei tag x commenti
            comment=(new Character(commentTags.get(0)).toString() + comment);
            commentHeaderRows.add(comment);
        }

        
        //-------------------------------------------------------------------------------------------------------------
        //
        // getRawValue
        //
        // this method returns the raw value of a parameter that is exactly the string in the value field of the 
        // corresponding specified key. If the requested key is not present, it returns an empty string.
        //
        // restituisce il valore "raw" di un parametro cioe` esattamente la corrispondente stringa nel file di 
        // configurazione. Se il parametro non e` presente, il metodo
        //
        //-------------------------------------------------------------------------------------------------------------
        public String getRawValue(String parameter) {return configData.get(parameter);}

        //-------------------------------------------------------------------------------------------------------------
        //
        // getBoolValue
        //
        // this method looks for the value corresponding to the key "parameter". If value is equal to the literal string 
        // "true" then the method returns true, indicating the key was found and correctly interpretated. Similarly, 
        // if value is equal to the literal string "false" then the method returns false, indicating the key was 
        // found and correctly interpretated. If the key param was not found in the configuration file or if the value
        // is a literal string other than "true" or "false" then the method returns null (fail) to warn there is 
        // something wrong (the parameter key is not present in the configuration file or the corresponding value is 
        // different than "true" or "false").
        // REM: the reason why the method returns a Boolean object instead of a primitive boolean variable is that I need
        // to have three possible values (true, false, null) to understand when something has gone wrong (return=null in
        // this case). If I would not use the Boolean object I would have to return always true or false and I would not 
        // be able to know if false would mean the parameter exists and it's set to false of if actually the parameter 
        // does not exist (or it has an unconsistent value) and the false returned refers to the fail of the operation.
        //
        // se la key string param e` presente, il metodo restituisce true o false a seconda se la corrispondente string 
        // value e` = a "true" o "false". Se la key string parameter non e` presente nei dati di configurazione oppure 
        // se la corrispondente string value ha un valore diverso da "true" o "false" il metodo ritorna null (fail)
        // REM: il motivo per cui il metodo ritorna l'oggetto Boolean anziche` la variabile primitiva boolean e` che devo 
        // avere tre valori possibili (true, false, null) per poter capire se qualcosa e` andato storto (null in tal caso).
        // Se non utilizzassi Boolean dovrei ritornare comunque true o false e non sarei in grado di capire se false significa
        // che effettivamente il parametro esiste ed e` impostato a false oppure se in realta` il parametro non esiste e
        // false fa riferimento al fallimento dell'operazione.
        //
        //-------------------------------------------------------------------------------------------------------------
        public Boolean getBoolValue(String parameter)
        {
            Boolean ret=null;
            if(parameter != null) {
                String value = configData.get(parameter);
                if(value.equals("true")) ret = new Boolean(true);
                else if(value.equals("false")) ret = new Boolean(false);
                else LOG.ziblog(LOG.LogLev.ERR, "value for key %s is %s but a boolean value is expected", parameter, value);
            }
            return ret;
        }

        //-------------------------------------------------------------------------------------------------------------
        //
        // getIntValue
        //
        // this method looks for the value corresponding to the key "param". If value is a string corresponding to an 
        // integer value then the method returns the corresponding Integer. indicating the key was found and correctly 
        // interpretated. If the key param is not present in the configuration file or the relative value cannot be
        // interpreded as an integer number, then the method returns null (fail) to warn there is something wrong
        //
        // se la key string param e` presente, il metodo restituisce un Integer corrispondente al valore. Se la key 
        // string param non e` presente nei dati di  configurazione oppure se la corrispondente string value ha un 
        // valore non  convertibile ad int il metodo ritorna null (fail)
        //
        //-------------------------------------------------------------------------------------------------------------a
        public Integer getIntValue(String parameter)
        {
            Integer ret=null;
            if(parameter != null) {
                String value = configData.get(parameter);
                try {
                    ret = new Integer(value);
                } catch(NumberFormatException ex) {
                    ret = null;
                    LOG.ziblog(LOG.LogLev.ERR, "cannot create Integer from %s - NumberFormatException(%s)", value, ex.getMessage());
                }
            }
            return ret;
        }
    }
//-------------------------------------------------------------------------------------------
}

