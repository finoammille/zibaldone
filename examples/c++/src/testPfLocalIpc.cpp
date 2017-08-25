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

#ifdef linux
#include "PfLocalIpc.h"
#include "Events.h"
using namespace Z;

class PfLocalIpcTestServer : public Thread {
    std::string ipcFile;
    PfLocalServer* srv;
    PfLocalConnHandler* conn;
    bool exit;
public:
    PfLocalIpcTestServer(std::string ipcFile="./IPCtest"):ipcFile(ipcFile), conn(NULL), exit(false) {srv=new PfLocalServer(ipcFile);}
    ~PfLocalIpcTestServer()
    {
        if(conn)delete conn;
        if(srv)delete srv;
    }
    void startListening()
    {
        conn = srv->Accept();
        ziblog(LOG::INF, "connection active");//se sono qui, la connessione e` stata effettuata dato che Accept() e` bloccante...
        register2Label(conn->getRxDataLabel());
        conn->Start();//start of the thread who handles the connection
                      //avvio del thread che gestisce la connessione col socket
        Start();//start of this thread
                //avvio di questo thread
    }
    void run()
    {
        std::string cmd;
	    char txBuf[256];
        std::cout<<"SocketTest>>\ntype ""exit"" to finish.\n";
        while(!exit){
            Event* Ev = pullOut(1000);//max 1 secondo di attesa
            if(Ev){
                ziblog(LOG::INF, "SERVER received event with label %s", Ev->label().c_str());
                if(dynamic_cast<StopThreadEvent *>(Ev)) exit = true;
                else if(dynamic_cast<RawByteBufferData *>(Ev)) {
                    RawByteBufferData* ev = (RawByteBufferData*)Ev;
                    for(int i=0; i<ev->len(); i++) std::cout<<"Received: "<<(ev->buf())[i]<<" from IPC socket "<<std::endl;
                }
                delete Ev;
            }
            if(std::cin.good()){
                std::cin.getline(txBuf, 256);//prendo il comando da console input
                cmd = txBuf;
                if(cmd == "exit") {
                    conn->Stop();
                    exit=true;
                }
                else {
                    RawByteBufferData tx(conn->getTxDataLabel(), (unsigned char*)txBuf, cmd.length());
                    tx.emitEvent();
                }
                cmd.clear();
            }
        }
    }
};

class PfLocalIpcTestClient : public Thread {
    std::string ipcFile;
    PfLocalClient* cli;
    PfLocalConnHandler* conn;
    bool exit;
public:
    PfLocalIpcTestClient(std::string ipcFile="./IPCtest"):ipcFile(ipcFile), conn(NULL), exit(false) {cli=new PfLocalClient(ipcFile);}
    ~PfLocalIpcTestClient()
    {
        if(conn)delete conn;
        if(cli)delete cli;
    }
    bool connect()
    {
        conn = cli->Connect();
        if(!conn) {
            ziblog(LOG::WRN, "connection failed: is server active?");
            return false;
        }
        ziblog(LOG::INF, "connection active");//se sono qui, la connessione e` stata effettuata dato che Accept() e` bloccante...
        register2Label(conn->getRxDataLabel());
        conn->Start();//start of the thread who handles the connection
                      //avvio del thread che gestisce la connessione col socket
        Start();//start of this thread
                //avvio di questo thread
        return true;
    }
    void run()
    {
        std::string cmd;
	    char txBuf[256];
        std::cout<<"SocketTest>>\ntype ""exit"" to finish.\n";
        while(!exit){
            Event* Ev = pullOut(1000);//max 1 secondo di attesa
            if(Ev){
                ziblog(LOG::INF, "CLIENT received event with label %s", Ev->label().c_str());
                if(dynamic_cast<StopThreadEvent *>(Ev)) exit = true;
                else if(dynamic_cast<RawByteBufferData *>(Ev)) {
                    RawByteBufferData* ev = (RawByteBufferData*)Ev;
                    for(int i=0; i<ev->len(); i++) std::cout<<"Received: "<<(ev->buf())[i]<<" from IPC socket "<<std::endl;
                }
                delete Ev;
            }
            if(std::cin.good()){
                std::cin.getline(txBuf, 256);//prendo il comando da console input
                cmd = txBuf;
                if(cmd == "exit") {
                    conn->Stop();
                    exit=true;
                } else {
                    RawByteBufferData tx(conn->getTxDataLabel(), (unsigned char*)txBuf, cmd.length());
                    tx.emitEvent();
                }
                cmd.clear();
            }
        }
    }
};

static void help()
{
    std::cout<<"\n1 => start listening server\n";
    std::cout<<"2 => create client and connect to listening server\n";
    std::cout<<"q => quit\n";
}

void testPfLocalIpc()
{
    char cmd;
    help();
    while (true) {
        std::cin>>cmd;
        if(cmd == '1'){//server
            ziblog(LOG::INF, "THIS IS SERVER");
            PfLocalIpcTestServer server;
            std::cout<<"\n...wait for incoming connection";
            server.startListening();
            std::cout<<"\n...connection done!";
            server.Join();
            return;//se sono qui, il server e` uscito.
        } else if(cmd == '2') {
            ziblog(LOG::INF, "THIS IS CLIENT");
            PfLocalIpcTestClient client;
            std::cout<<"\n...trying to connect";
            if (client.connect()) {
                std::cout<<"\n...connection done!";
                client.Join();
            } else std::cout<<"\n... connection FAILED.";
            return;//se sono qui, il server e` uscito.
        } else if(cmd == 'q') return;
        else {
            std::cout<<"\nunknown choice\n";
            help();
        }
    }
}
#endif
