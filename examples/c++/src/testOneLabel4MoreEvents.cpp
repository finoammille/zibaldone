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

#include "Thread.h"
#include "Events.h"
#include "Log.h"
#include <typeinfo>

using namespace Z;//namespace for zibaldone library

const std::string sharedLabel="sharedLabel";

class anEvent : public Event {
public:
    unsigned char* allocatedArea;
    int integer;
    std::string aString;
    std::vector<int> andVectorToo;
    anEvent(int bufferSize, std::string aStringValue):Event(sharedLabel), integer(bufferSize), aString(aStringValue)
    {
        allocatedArea = new unsigned char [integer];//integer allocated heap memory
        for(int i=0; i<integer; i++) allocatedArea[i]='a'+i;//put something on allocated area
        for(int i=0; i<100; i++) andVectorToo.push_back((int) aStringValue[0]);//on hundred times the first char of the string....
    }
    ~anEvent()
    {
        delete allocatedArea;
        integer=0xFFFF;
        aString.clear();
        andVectorToo.clear();
    }
    anEvent(const anEvent &obj):Event(obj)//alternativaly one can do anEvent(const anEvent &obj):Event(obj)
    {
        integer=obj.integer;
        aString=obj.aString;
        allocatedArea = new unsigned char [integer];
        for(int i=0; i<integer; i++) allocatedArea[i]=obj.allocatedArea[i];
        for(int i=0; i<100; i++) andVectorToo.push_back(obj.andVectorToo[i]);
    }
    anEvent & operator = (const anEvent &src)
    {
        //the important field to set is _label. you can both call
        //base class operator overload or set directly _label

        if(this == &src) return *this; //self assignment check...
        //Event::operator= (src);
        _label=src._label;

        if(allocatedArea) delete[] allocatedArea;//deallocate
        integer=src.integer;
        aString=src.aString;
        allocatedArea = new unsigned char [integer];
        for(int i=0; i<integer; i++) allocatedArea[i]=src.allocatedArea[i];
        for(int i=0; i<100; i++) andVectorToo.push_back(src.andVectorToo[i]);
        return *this;
    }
    anEvent* clone()const{return new anEvent(*this);}
};

//thread who fires anEvent
class Sender : public Thread {
public:
    void help()
    {
        std::cout<<"\n*** one label for more events Test!.... type ""exit"" to finish ***\n";
        std::cout<<"N.B.: all emitted events have label = "<<sharedLabel<<"\n";
        std::cout<<"Available commands:\n";
        std::cout<<"exit => terminate test and exit\n";
        std::cout<<"anEvent => emits anEvent Event\n";
        std::cout<<"rawBuffer => emits RawByteBufferData event\n";
        std::cout<<"timeout => emits TimeoutEvent event\n";
        std::cout<<"error => emits ERROR event\n";
        std::cout<<"sonOfaBitch => emits InfoMsg event with NO LABEL\n";
    }
private:
    void run()//thread loop function
    {
        std::string cmd;
	    char txBuf[256];
        help();
        for(;;){
            Event* Ev = pullOut(1000);//check thread queue for new events. We don't block on queue but we wait at least 1 sec, then we poll for user input
            if(Ev){
                //if here, there was an event!
                ziblog(LOG::INF, "received event %s", Ev->label().c_str());
                if(dynamic_cast<StopThreadEvent *>(Ev)) {
                    delete Ev;//we have extracted the event from queue and now it's our resposibility to delete it after using it
                    return;
                } else {
                    delete Ev;
                    std::cout<<"unexpected event!\n";
                }
            }
            if(std::cin.good()){//check for user input
                std::cout<<"\n=) ";
                std::cin.getline(txBuf, 256);//get input
                cmd = txBuf;
                if(!cmd.empty()) {
                    if(cmd == "exit") return;
                    else if(cmd == "anEvent") {
                        anEvent* Ev = new anEvent(1024, "stuff here...");
                        std::cout<<"\n\n\nSender is emitting event "<<typeid(*Ev).name()<<" with label \""<<Ev->label()<<"\"\n";
                        std::cout<<"allocated area: size ="<<Ev->integer<<"\ncontent is:\n";
                        for(int i=0; i<Ev->integer; i++) {
                            std::cout<<Ev->allocatedArea[i]<<" ";
                            if(!(i%20)) std::cout<<"\n";
                        }
                        std::cout<<"integer = "<<Ev->integer<<"\n";
                        std::cout<<"aString = "<<Ev->aString<<"\n";
                        std::cout<<"andVectorToo:\n";
                        for(int i=0; i<100; i++) {
                            std::cout<<Ev->andVectorToo[i]<<" ";
                            if(!(i%20)) std::cout<<"\n";
                        }
                        Ev->emitEvent();
                        std::cout<<"event emitted!!!\n";
                        delete Ev;//once the object is sent each thread will have his own copy and I delete mine.
                        std::cout<<"... and deleted local copy.....\n\n\n";
                    } else if(cmd == "rawBuffer") {
                        std::vector<unsigned char> bufferData(10, 'a');
                        RawByteBufferData rawBuffer(sharedLabel, bufferData);
                        std::cout<<"\n\n\nSender is emitting event "<<typeid(rawBuffer).name()<<" with label "<<rawBuffer.label()<<"\"\n";
                        rawBuffer.emitEvent();
                        std::cout<<"event emitted!!!\n";
                    } else if(cmd == "timeout") {
                        TimeoutEvent te(sharedLabel);
                        std::cout<<"\n\n\nSender is emitting event "<<typeid(te).name()<<" with label "<<te.label()<<"\"\n";
                        te.emitEvent();
                        std::cout<<"event emitted!!!\n";
                    } else if(cmd=="error") {
                        zibErr e(sharedLabel, "causa dell'errore = ....");
                        std::cout<<"\n\n\nSender is emitting event "<<typeid(e).name()<<" with label "<<e.label()<<"\"\n";
                        e.emitEvent();
                        std::cout<<"event emitted!!!\n";
                    } else if(cmd=="sonOfaBitch") {
                        InfoMsg sob("", "this is a InfoMsg with label empty!!!! zibaldone will treat this event as a sob!");
                        std::cout<<"\n\n\nSender is emitting event "<<typeid(sob).name()<<" with label "<<sob.label()<<"\"\n";
                        sob.emitEvent();
                        std::cout<<"event emitted!!!\n";
                    } else if(cmd=="?" || cmd=="help") help();
                    else std::cout<<"unknown command....\n";
                    cmd.clear();
                }
            }
        }
    }
};

//Receiver is a thread
class Receiver : public Thread {
    void run()
    {
        while(true) {
            Event* Ev = pullOut();//Receiver blocks on queue waiting for events. We can receive only events we have registered to (sharedLabel) and stopThread
            if(Ev){
                ziblog(LOG::INF, "received event %s", Ev->label().c_str());
                if(dynamic_cast<StopThreadEvent *>(Ev)) {
                    delete Ev;
                    return;
                } else if(dynamic_cast<anEvent *>(Ev)) {
                    //this event has no meaningful Ev->buf() adn Ev->len() (the first is NULL and the second is zero): it is the object itself that is what I want!
                    anEvent* ev = (anEvent*)Ev;
                    std::cout<<"\n\n\nReceiver RECEIVED EVENT "<<typeid(*ev).name()<<"\n";
                    std::cout<<"allocated area: size ="<<ev->integer<<"\ncontent is:\n";
                    for(int i=0; i<ev->integer; i++) {
                        std::cout<<ev->allocatedArea[i]<<" ";
                        if(!(i%20)) std::cout<<"\n";
                    }
                    std::cout<<"integer = "<<ev->integer<<"\n";
                    std::cout<<"aString = "<<ev->aString<<"\n";
                    std::cout<<"andVectorToo:\n";
                    for(int i=0; i<100; i++) {
                        std::cout<<ev->andVectorToo[i]<<" ";
                        if(!(i%20)) std::cout<<"\n";
                    }
                } else if(dynamic_cast<RawByteBufferData *>(Ev)) {
                    RawByteBufferData* ev = (RawByteBufferData*)Ev;
                    std::cout<<"\n\n\nReceiver RECEIVED EVENT "<<typeid(*ev).name()<<"\n";
                    std::cout<<"buffer Length = "<<ev->len()<<"\n";
                    std::cout<<"data:\n";
                    for(int i=0; i<ev->len(); i++)std::cout<<(ev->buf())[i];
                } else if(dynamic_cast<TimeoutEvent *>(Ev)) {
                    TimeoutEvent* ev = (TimeoutEvent*)Ev;
                    std::cout<<"\n\n\nReceiver RECEIVED EVENT "<<typeid(*ev).name()<<"\n";
                    std::cout<<"timerId="<<ev->timerId()<<"\n";
                } else if(dynamic_cast<zibErr *>(Ev)) {
                    zibErr* ev = (zibErr*)Ev;
                    std::cout<<"\n\n\nReceiver RECEIVED EVENT "<<typeid(*ev).name()<<"\n";
                    std::cout<<"error message ="<<ev->errorMsg()<<"\n";
                } else if(dynamic_cast<InfoMsg *>(Ev)) {
                    InfoMsg* ev = (InfoMsg*)Ev;
                    std::cout<<"\n\n\nReceiver RECEIVED EVENT "<<typeid(*ev).name()<<"\n";
                    std::cout<<"msg ="<<ev->msg()<<"\n";
                } else ziblog(LOG::ERR, "unexpected event!");
                delete Ev;
            }
        }
    }
public:
    Receiver(){
        register2Label(sharedLabel);//Receiver wants to receive a copy of events whose label is the string sharedLabel
        register2Label("sob");//this time Receiver also welcomes sons of bitch
    }
};

void testOneLabel4MoreEvents()
{
    Sender sender;      //prepare a Sender Thread
    Receiver receiver;  //prepare Receiver Thread

    //let's start all of them:
    sender.Start();
    receiver.Start();

    //all ends whenever sender exits
    sender.Join();
    receiver.Stop();
}
