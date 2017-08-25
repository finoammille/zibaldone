/*
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

import Z.*;
import java.io.*;
import java.util.Date;

public class Main
{
    public static void main (String[] args)
    {
        String preLogMessages = new String();//sinche` non faccio partire il log non posso loggare, ma
                                             //i parametri sono nel file di configurazione! Metto allora
                                             //in  preLogMessages eventuali messaggi che non posso ancora
                                             //loggare (log non ancora partito) e che loggero` come prima
                                             //cosa appena faro` partire il log!
        Tools.configFileHandler configFile=new Tools.configFileHandler("zibTests.conf");
        if(configFile.exists()) {
            preLogMessages+="reading configuration file\n";
            System.out.print(preLogMessages);
            configFile.read();
        } else {//il file di configurazione non esiste. Ne creo uno con valori di default
            preLogMessages+="configuration file not found\n";
            configFile.insertCommentHeaderRaw("available log level: DBG, INF, WRN, ERR");
            configFile.insertCommentHeaderRaw("available test: Threads, Timers, Tcp, Udp, SerialPort, StopAndWaitOverUdp,");
            configFile.insertCommentHeaderRaw("                autonomousThread, sameThreadAddressSpaceEventReceiver, zibTools");
            configFile.insertCommentHeaderRaw("                oneLabel4MoreEvents");
            configFile.insertCommentHeaderRaw("showLogOnConsole maybe true or false");
            configFile.addOrUpdateKeyValue("logLevel", "DBG");
            configFile.addOrUpdateKeyValue("showLogOnConsole", "true");
            configFile.write();
        }
        //qui comunque ho configFile con dei parametri. Se il file di configurazione non esisteva allora
        //ho anche la certezza di avere tutti i parametri che mi servono perche` ho creato un file di
        //configurazione con i valori di default. Se invece il file esisteva gia` questa garanzia non ce l'ho
        //(potrebbe esserci scritta qualsiasi cosa!). Per questo motivo inizializzo comunque i parametri
        //con i valori di default e poi vado a vedere cosa c'e` nel file di configurazione: se il parametro nel
        //file di configurazione e` corretto allora lo uso, in caso contrario loggo l'errore e uso il parametro
        //preimpostato al valore di default. Si noti che poiche` il log deve ancora partire (tra i parametri di
        //configurazione c'e` il livello di log) eventuali errori in questa fase vanno loggati con cerr o cout
        //oppure raccolti e sparati sul log file appena fatto partire il log.

        //esempio di parametri di configurazione
        //logLevel=DBG
        //shoLogOnConsole=true
        //test=Threads
        
        //default
        boolean showLogOnConsole = true;
        LOG.LogLev loglevel = LOG.LogLev.DBG;

        //from configuration file
        Boolean ShowLogOnConsole = configFile.getBoolValue("showLogOnConsole");
        if(ShowLogOnConsole != null) showLogOnConsole = ShowLogOnConsole.booleanValue();
        else preLogMessages+="showLogOnConsole not found in configuration data\n";

        String strLogLevel = configFile.getRawValue("logLevel");
        if(strLogLevel != null) {
            if(strLogLevel.equals("DBG")) loglevel=LOG.LogLev.DBG;
            else if(strLogLevel.equals("INF")) loglevel=LOG.LogLev.INF;
            else if(strLogLevel.equals("WRN")) loglevel=LOG.LogLev.WRN;
            else if(strLogLevel.equals("ERR")) loglevel=LOG.LogLev.ERR;
            else preLogMessages+=("unknown value "+strLogLevel+" in configuration file\n");
        }

        LOG.set("./", "zibTests", loglevel, showLogOnConsole);
        LOG.ziblog(LOG.LogLev.INF, "%s", preLogMessages);

        String testName = new String();
        if(args.length==0) {//cerco il valore "test" nel configuration file solo se non sovrascritto da comando esplicito.
                            //In altri termini se chiamo il programma con "java -jar zibTests.jar --testThreads" il test eseguito
                            //sara` "Threads" indipendentemente da quello che e` specificato nel configuration file. Il test
                            //indicato nel file di configurazione viene preso in considerazione solo se chiamo il progeamma
                            //senza argomenti "java -jar zibTests.jar
            String test = configFile.getRawValue("test");
            if(test!=null) testName="--test"+test;
            else {
                System.out.println("test name not specified in command line nor in configuration file");
                LOG.ziblog(LOG.LogLev.ERR, "%s", "test name not specified in command line nor in configuration file");
                LOG.disable();
                return;
            }
        } else testName=args[0];
        
        //uso "equalsIgnoreCase anziche` equals cosi` accetto per buone anche espressioni tipo "--testthreads" o "--TestThreads" ...
        if (testName.equalsIgnoreCase("--testThreads")) {
            (new testThreads()).test();                 
        } else if (testName.equalsIgnoreCase("--testTimers")) {
            (new testTimers()).test();
        } else if (testName.equalsIgnoreCase("--testUdp")) {
            (new testUdp()).test();
        } else if (testName.equalsIgnoreCase("--testTcp")) {
            (new testTcp()).test();
        } else if (testName.equalsIgnoreCase("--testSerialPort")) {
            (new testSerialPort()).test();
        } else if (testName.equalsIgnoreCase("--testStopAndWaitOverUdp")) {
            (new testStopAndWaitOverUdp()).test();
        } else if (testName.equalsIgnoreCase("--testAutonomousThread")) {
            (new testAutonomousThread()).test();
        } else if (testName.equalsIgnoreCase("--testSameThreadAddressSpaceEventReceiver")) {
            (new testSameThreadAddressSpaceEventReceiver()).test();
        } else if (testName.equalsIgnoreCase("--testZibTools")) {
            (new testZibTools()).test();
        } else if (testName.equalsIgnoreCase("--testoneLabel4MoreEvents")) {
            (new testOneLabel4MoreEvents()).test();
        } else  {
            System.out.println("ERROR => test name unknown");
            LOG.ziblog(LOG.LogLev.ERR, "%s", "ERROR => test name unknown");
        } 
        LOG.disable();
    }
}

