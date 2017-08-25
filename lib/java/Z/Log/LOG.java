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

import java.util.logging.*;
import java.util.Date;
import java.io.*;
import java.nio.file.*;
import java.text.SimpleDateFormat;

public class LOG {
    private static LOG instance = null;
    private static Logger logger;
    private String logFileDstDirPath;
    private String logFileNamePrefix;
    private String logFileName;
    private boolean showLogOnConsole;
    private LogLev logLev;
    private FileHandler fileHandler=null;
    private ConsoleHandler consoleHandler=null;
    private LOG(String logFileDstDirPath, String logFileNamePrefix, LogLev lev, boolean showLogOnConsole)
    {
        class LogFormatter extends Formatter {
            @Override
            public String format(LogRecord record)
            {
                return new Date(record.getMillis())+", "+
                           record.getSourceClassName()+", "+
                           record.getSourceMethodName()+": "+
                           //"thread-"+record.getThreadID()+": "+
                           record.getMessage()+"\n";
            }
        }
        logFileName=new String(logFileDstDirPath);
        if(logFileDstDirPath.substring(logFileDstDirPath.length()-1)!=File.separator) logFileName+=File.separator;
        logFileName+=logFileNamePrefix;
        logFileName+="_log_";
        logFileName+=((new SimpleDateFormat("dd-MM-yyyy")).format(new Date()));
        logger=Logger.getLogger("zibaldoneLogger");
        try {
            fileHandler = new FileHandler(logFileName, 10000000, 1, true);//max dimensione del file di log 10Mb
            fileHandler.setFormatter(new LogFormatter());
            logger.addHandler(fileHandler);
            if(showLogOnConsole==true) {
                consoleHandler = new ConsoleHandler();
                consoleHandler.setFormatter(new LogFormatter());
                logger.addHandler(consoleHandler);
            }
            logger.setUseParentHandlers(false);
        } catch (Exception ex) {
            ziblog(LogLev.ERR, "%s, %s", "Fail to create/access logger file handler.", ex);
        }
        //NOTA: consoleHandler (default loggin handler) ha un log level separato, preimpostato a INFO. Quindi
        //oltre a settare logger.setLevel (che a questo punto setta solo il log del file e tanto varrebbe
        //utilizzare direttamente fileHandler) occorre settare esplicitamente il logLevel di consoleHandler
        Level lv=Level.FINE;
        if(lev==LogLev.DBG) lv=Level.FINE;
        else if(lev==LogLev.INF) lv=Level.INFO;
        else if(lev==LogLev.WRN) lv=Level.WARNING;
        else if(lev==LogLev.ERR) lv=Level.SEVERE;
        logger.setLevel(lv);
        if(consoleHandler!=null) consoleHandler.setLevel(lv);
        this.logFileDstDirPath=logFileDstDirPath;
        this.logFileNamePrefix=logFileNamePrefix;
        this.showLogOnConsole=showLogOnConsole;
        this.logLev=lev;
    }

    //-------------------------------------------------------------------------
    //PUBLIC
    public enum LogLev {DBG, INF, WRN, ERR};
    public static void set(String logFileDstDirPath, String logFileNamePrefix, LogLev logLev, boolean showLogOnConsole)
    {
        if(instance==null) {
            instance=new LOG(logFileDstDirPath, logFileNamePrefix, logLev, showLogOnConsole);
        } else {
            if(!instance.logFileDstDirPath.equals(logFileDstDirPath) ||
               !instance.logFileNamePrefix.equals(logFileNamePrefix) ||
               instance.logLev!=logLev ||
               instance.showLogOnConsole!=showLogOnConsole) {
                //se e` diverso, disabilito quello attuale e lo riistanzio
                instance.logger.removeHandler(instance.consoleHandler);
                instance.logger.removeHandler(instance.fileHandler);
                instance=new LOG(logFileDstDirPath, logFileNamePrefix, logLev, showLogOnConsole);
            }
        }
        try {
            Files.write(Paths.get(instance.logFileName), "\n=> NEW LOG SESSION\n".getBytes(), StandardOpenOption.APPEND);
        } catch (IOException ex) {
            ziblog(LogLev.ERR, "%s, %s", "I/O exception", ex);
        }
    }
    public static void disable()//non e` strettamente necessario chiamarla, ma assicura il flush del buffer, per cui e`
                                //buona pratica chiamarla al termine del programma (tipicamente prima di uscire dal Main)
    {
        if(instance!=null) {
            if(instance.fileHandler != null) {
                instance.fileHandler.flush();
                instance.fileHandler.close();
            }
        }
    }
    public static void ziblog(LogLev level, String format, Object... args)
    {
        if(instance!=null) {
            Level lev=Level.FINE;
            String logLevTagMsg="";
            if(level==LogLev.DBG) {
                logLevTagMsg="(DBG) ";
                lev=Level.FINE;
            } else if(level==LogLev.INF) {
                logLevTagMsg="(INF) ";
                lev=Level.INFO;
            } else if(level==LogLev.WRN) {
                logLevTagMsg="(WRN) ";
                lev=Level.WARNING;
            } else if(level==LogLev.ERR) {
                logLevTagMsg="(ERR) ";
                lev=Level.SEVERE;
            }
            java.lang.StackTraceElement ste=java.lang.Thread.currentThread().getStackTrace()[2];
            String srcFileCoordinate=logLevTagMsg+ste.getFileName()+", "+ste.getLineNumber();
            String methodCoordinate=ste.getClassName()+"."+ste.getMethodName();
            logger.logp(lev, srcFileCoordinate, methodCoordinate, String.format(format, args));
        }
    }
}
