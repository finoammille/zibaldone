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
import Z.Tools.*;
import java.io.*;
import java.util.*;

class CustomObj implements Serializable {
    Character a;
    Integer b;
    Boolean c;
    CustomObj()
    {
        a = new Character('x');
        b = new Integer('0');
        c = new Boolean(false);
    }
}

class testZibTools {
	private OsCmd ZT=new OsCmd();
	private String cmd;
	private boolean quit; 
    private CustomObj myObj=new CustomObj();
	public testZibTools() {quit = false;}
	void help()
    {
        System.out.print("\n*** ZIB TOOLS TEST ***\n");
        System.out.print("Available commands:\n");
        System.out.print("\"listSerialPorts\" - shows available serial ports\n");
        System.out.print("\"getFileSize\" - get size of a file\n");
        System.out.print("\"testSerializeString\" - serialize and deserialize the string \"goofyPlutoDonaldduck\"\n");
        System.out.print("\"testSerializeObj\" - serialize and deserialize a object instance of a custom class\n");
        System.out.print("\"testConfigFile\" - test configuration file management\n");
        System.out.print("\"quit\" - exit\n");
    }
	void test()
    {
        help();
        System.out.print("\nZt>>");
        BufferedReader stdin = new BufferedReader(new InputStreamReader(System.in));
        String cmd;
        while(!quit)
        {
            try {
                cmd = stdin.readLine();
                if(!cmd.isEmpty()) {
                    if(cmd.equalsIgnoreCase("quit")) quit=true;
                    else if(cmd.equalsIgnoreCase("listSerialPorts")) {
                        ArrayList<String> result = ZT.getSerialPortList();
                        System.out.print("\nfound "+result.size()+" serial ports:\n");
                        for (String s : result) System.out.println(s);
                    } else if(cmd.equalsIgnoreCase("getFileSize")) {
                        System.out.print("\ninsert full path file name: ");
                        String filename = stdin.readLine();
                        long len = ZT.getFileSizeInBytes(filename);
                        System.out.print("\nsize =" + Long.toString(len));
                    } else if(cmd.equalsIgnoreCase("testSerializeString")) {
                        String foo = "goofyPlutoDonaldduck";
                        ArrayList<Byte> serializedStr= (new Serialize<String> (foo)).serializedObj;
                        for(int i=0; i<10; i++) serializedStr.add(new Byte((byte)i));
                        System.out.print("... serializeStdString applied to the string \"goofyPlutoDonaldduck\"\n");
                        System.out.print("... and added 10 stuff bytes at end\n");
                        System.out.print("the result is:\n");
                        Iterator<Byte> it = serializedStr.iterator();
                        while (it.hasNext()) System.out.print((it.next()).toString()+" ");
                        String deserializedString = (new Deserialize<String>(serializedStr)).obj;
                        System.out.print("\nthe deserialized string is: "+deserializedString+"\n");
                    } else if(cmd.equalsIgnoreCase("testSerializeObj")) {
                        Integer intero=15;
                        myObj.a='a';
                        myObj.b=1;
                        myObj.c=true;
                        ArrayList<Byte> S_intero = (new Serialize<Integer> (new Integer(intero))).serializedObj;
                        ArrayList<Byte> S_myObj = (new Serialize<CustomObj> (myObj)).serializedObj;
                        ArrayList<Byte> serializzazione=new ArrayList<Byte>();
                        serializzazione.addAll(S_intero);
                        serializzazione.addAll(S_myObj);
                        System.out.print("risultato della serializzazione: ");
                        Iterator<Byte> it = serializzazione.iterator();
                        while (it.hasNext()) System.out.print((it.next()).toString()+" ");
                        int interoEstratto = (new Deserialize<Integer>(S_intero)).obj;
                        serializzazione.remove(0);
                        System.out.print("\ninteroEstratto="+Integer.toString(interoEstratto)+"\n");
                        CustomObj deserializedMyObj = (new Deserialize<CustomObj>(S_myObj)).obj;
                        System.out.printf("\ndeserializedMyObj.a=%c\n", deserializedMyObj.a);
                        System.out.printf("\ndeserializedMyObj.b=%d\n", deserializedMyObj.b);
                        System.out.printf("\ndeserializedMyObj.c=%b\n", deserializedMyObj.c);
                    } else if(cmd.equalsIgnoreCase("testConfigFile")) {
                        System.out.print("\ncreating a config file named config.conf...\n");
                        configFileHandler cf = new configFileHandler("config.conf");
                        System.out.print("\nit's empty, so reading it would give empty map ...\n");
                        cf.read();
                        HashMap<String, String> cfData = cf.getConfigData();
                        System.out.print("\ncfData size is: "+ Integer.toString(cfData.size())+"\n");
                        System.out.print("\nadding entry port.name = /dev/ttyS0 ...\n");
                        cf.addOrUpdateKeyValue("port.name","/dev/ttyS0");
                        System.out.print("\nadding entry port.speed = 115200 ...\n");
                        cf.addOrUpdateKeyValue("port.speed","115200");
                        cf.write();
                        System.out.print("\nreading config file again...\n");
                        cf.read();
                        cfData = cf.getConfigData();
                        System.out.print("\nnow cfData size is: "+ Integer.toString(cfData.size())+"\n");
                        System.out.print("\nand content is ...\n");
                        for(Map.Entry<String, String> entry : cfData.entrySet()) System.out.print(entry.getKey()+"="+entry.getValue()+"\n");
                    } else if(cmd.equalsIgnoreCase("help")) help();
                    else System.out.println("unknown command.\n");
                    if(!quit) System.out.print("\nZt>> ");
                }
            } catch (IOException ex) {
                LOG.ziblog(LOG.LogLev.ERR, "IO Exception (%s)", ex.getMessage());
            }
        }
    }
}
