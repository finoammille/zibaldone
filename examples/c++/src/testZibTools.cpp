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

#include "Tools.h"

using namespace Z;

class testZibTools {
	OsCmd ZT;
	std::string cmd;
	bool quit;

    struct Struttura {
        unsigned char a;
        int b;
        bool c;
    } struttura;
public:
	testZibTools()
	{
		quit = false;
	}
	void help();
	void go();
};

void testZibTools::help()
{
	std::cout<<"\n*** ZIB TOOLS TEST ***\n";
	std::cout<<"Available commands:\n";
#ifdef linux
	std::cout<<"\"listSerialPorts\" - shows available serial ports\n";
#endif
	std::cout<<"\"getFileSize\" - get size of a file\n";
    std::cout<<"\"testSerializeString\" - serialize and deserialize the string \"goofyPlutoDonaldduck\"\n";
    std::cout<<"\"testSerializeBuiltInTypes\" - serialize and deserialize a set ob POD in data\n";
    std::cout<<"\"testConfigFile\" - test configuration file management\n";
	std::cout<<"\"quit\" - exit\n";
}

void testZibTools::go()
{
    help();
    std::cout<<"\nZt>";
    while (!quit) {
        if(std::cin.peek() != std::iostream::traits_type::eof()) {
            std::cin>>cmd;
            if(cmd == "quit") quit=true;
#ifdef linux
			else if (cmd == "listSerialPorts") {
				std::vector<std::string> result = ZT.getSerialPortList();
                for(size_t i=0; i<result.size(); i++) std::cout<<result[i]<<std::endl;
			}
#endif
			else if (cmd == "getFileSize") {
                std::string fileName;
                std::cout<<"insert full path file name: ";
                std::cin>>fileName;
                std::cout<<"size ="<<ZT.getFileSizeInBytes(fileName)<<"\n";
            } else if(cmd == "testSerializeString") {
                std::string test="goofyPlutoDonaldduck";
                std::vector<unsigned char> serializedStr=serializeStdString(test);
                for(int i=0; i<10; i++) serializedStr.push_back('a'+i);
                std::cout<<"... serializeStdString applied to the string \"goofyPlutoDonaldduck\"\n";
                std::cout<<"... and added 10 stuff bytes at end\n";
                std::cout<<"the result is:\n";
                for(size_t j=0; j<serializedStr.size(); j++) std::cout<<(char) serializedStr[j];
                std::string deserializedString = deserializeStdString(serializedStr);
                std::cout<<"\nthe deserialized string is: "<<deserializedString<<"\n";
                std::cout<<"the vector should contain only added stuff now! That is:\n";
                for(size_t k=0; k<serializedStr.size(); k++) std::cout<<(char) serializedStr[k];
            } else if(cmd == "testSerializeBuiltInTypes") {
                int intero=15;
                struttura.a='a';
                struttura.b=1;
                struttura.c=true;
                char carattere='a';
                bool booleano1=false;
                bool booleano2=true;
                std::vector<unsigned char> S_intero=serialize(intero);
                std::vector<unsigned char> S_struttura=serialize(struttura);
                std::vector<unsigned char> S_carattere=serialize(carattere);
                std::vector<unsigned char> S_booleano1=serialize(booleano1);
                std::vector<unsigned char> serializzazione;
                std::copy(S_intero.begin(), S_intero.end(),std::back_inserter(serializzazione));
                std::copy(S_struttura.begin(), S_struttura.end(),std::back_inserter(serializzazione));
                std::copy(S_carattere.begin(), S_carattere.end(),std::back_inserter(serializzazione));
                std::copy(S_booleano1.begin(), S_booleano1.end(),std::back_inserter(serializzazione));
                serializzazione.push_back(booleano2);
                std::cout<<"risultato della serializzazione: ";
                for(size_t j=0; j<serializzazione.size(); j++) std::cout<<(int) serializzazione[j]<<"  ";
                int interoEstratto= deserialize<int>(serializzazione);
                std::cout<<"\ninteroEstratto="<<interoEstratto<<"\n";
                std::cout<<"\nrimanenza: ";
                for(size_t j=0; j<serializzazione.size(); j++) std::cout<<(int) serializzazione[j]<<"  ";
                Struttura strutturaEstratta= deserialize<Struttura>(serializzazione);
                std::cout<<"\nstrutturaEstratta.a="<<strutturaEstratta.a<<"\n";
                std::cout<<"strutturaEstratta.b="<<strutturaEstratta.b<<"\n";
                std::cout<<"strutturaEstratta.c="<<strutturaEstratta.c<<"\n";
                std::cout<<"\nrimanenza: ";
                for(size_t j=0; j<serializzazione.size(); j++) std::cout<<(int) serializzazione[j]<<"  ";
                int carattereEstratto= deserialize<char>(serializzazione);
                std::cout<<"carattereEstratto="<<carattereEstratto<<"\n";
                std::cout<<"\nrimanenza: ";
                for(size_t j=0; j<serializzazione.size(); j++) std::cout<<(int) serializzazione[j]<<"  ";
                bool booleano1Estratto= deserialize<bool>(serializzazione);
                std::cout<<"booleano1Estratto="<<booleano1Estratto<<"\n";
                std::cout<<"\nrimanenza: ";
                for(size_t j=0; j<serializzazione.size(); j++) std::cout<<(int) serializzazione[j]<<"  ";
                bool booleano2Estratto=(serializzazione.back()!=0);
                serializzazione.pop_back();
                std::cout<<"booleano2Estratto="<<booleano2Estratto<<"\n";
                std::cout<<"\nrimanenza: ";
                for(size_t j=0; j<serializzazione.size(); j++) std::cout<<(int) serializzazione[j]<<"  ";
            } else if(cmd=="testConfigFile") {
                std::cout<<"\ncreating a config file named config.conf...\n";
                configFileHandler cf("config.conf");
                std::cout<<"\nit's empty, so reading it would give empty map ...\n";
                cf.read();
                std::map<std::string, std::string> cfData = cf.getConfigData();
                std::cout<<"\ncfData size is: "<<cfData.size()<<"\n";
                std::cout<<"\nadding entry port.name = /dev/ttyS0 ...\n";
                cf.addOrUpdateKeyValue("port.name","/dev/ttyS0");
                std::cout<<"\nadding entry port.speed = 115200 ...\n";
                cf.addOrUpdateKeyValue("port.speed","115200");
                cf.write();
                std::cout<<"\nopening existing config file named config2.conf\n";
                configFileHandler cf2("config2.conf");
                cf2.read();
                std::map<std::string, std::string> cf2Data = cf2.getConfigData();
                std::cout<<"\ncf2Data size is: "<<cf2Data.size()<<"\n";
                std::cout<<"data are:\n";
                std::map<std::string, std::string>::iterator cf2Data_iter;
                for(cf2Data_iter=cf2Data.begin(); cf2Data_iter!=cf2Data.end(); cf2Data_iter++)
                    std::cout<<cf2Data_iter->first<<" = "<<cf2Data_iter->second<<"\n";
            }
            else std::cout<<"unknown command.\n";
        }
        if(!quit)std::cout<<"\nZt>";
    }
}

void testTools()
{
    testZibTools tools;
	tools.go();
}
