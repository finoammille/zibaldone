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

/*
    example of using zibaldone library. This example contains two simple
    thread: the keyLogger Thread starts polling between thread event queue and
    console input. When something is typed on keyboard, keylogger emits an event
    containing what was typed.

    A second thread parrot registers to event emitted by keylogger and when receives
    that event it prints on screen the content of that message (that is the message
    typed on keyboard by user).
*/

#include "Thread.h"
#include "Events.h"
#include "Log.h"
#include <typeinfo>
using namespace Z;//namespace for zibaldone library

const std::string userText="userText";//label that tags Events (emitted by keylogger) that contains text typed by user

//class keylogger is a thread.
class keylogger : public Thread {
    bool exit;
    void run()//thread loop function
    {
        std::string cmd;
	    char txBuf[256];
        std::cout<<"Thread Test>>\ntype ""exit"" to finish.\n";
        while(!exit){
            Event* Ev = pullOut(1000);//check thread queue for new events. We don't block on queue but we wait at least 1 sec, then we poll for user input
            if(Ev){
                //if here, there was an event!
                ziblog(LOG::INF, "received event with label %s", Ev->label().c_str());
                if(dynamic_cast<StopThreadEvent *>(Ev)) exit = true;
                delete Ev;//we have extracted the event from queue and now it's our resposibility to delete it after using it
            }
            if(std::cin.good()){//check for user input
                std::cin.getline(txBuf, 256);//get input
                cmd = txBuf;
                if(cmd == "exit") exit=true;
                else {
                    InfoMsg tx(userText, "=>"+cmd+"<=");
                    tx.emitEvent();
                }
                cmd.clear();
            }
        }
    }
public:
    keylogger(){exit=false;}
};

//parrot is a thread
class parrot : public Thread {
    void run()
    {
        while(true) {
            Event* Ev = pullOut();//parrot blocks on queue waiting for events. We can receive only events we have registered to (userText) and stopThreadEventId
            if(Ev){
                ziblog(LOG::INF, "received event %s", Ev->label().c_str());
                if(dynamic_cast<StopThreadEvent *>(Ev)) return;
                else if(dynamic_cast<InfoMsg *>(Ev)) {
                    InfoMsg* ev = (InfoMsg*)Ev;
                    std::cout<<"\n\n\nPARROT RECEIVED EVENT "<<typeid(*ev).name()<<"\n";
                    std::cout<<"payload event msg ="<<ev->msg()<<"\n";
                } else ziblog(LOG::ERR, "unexpected event!");
                delete Ev;
            }
        }
    }
public:
    parrot(){register2Label(userText);}//parrot wants to receive a copy of event whose id is the string userText
};

void testThreads()
{
    keylogger k;//prepare a keyLogger Thread
    parrot p;//prepare parrot Thread

    //let's start both of them:
    k.Start();
    p.Start();

    /*
    now we have to Join main() function to one thread.

    If we don't do that, main function will end (return) and both of klt and pt will stop
    their execution immediately. If this happens (thread killed instead of stopped in the
    right way) we cannot predict what shall happen: it may happen that the thread function
    was accessing a member data variabile that has been destroyed and so we can have a
    segmentation fault...

    what thread do we Join() to? klt or pt? At first sight it might seem that it's the same.
    but as we'll see next, it's not indifferent what thread join to.

    Suppose we join to pt, then:

    pt.Join();//programs hangs here untill pt Thread doesn't stop his execution. When this
              //line will be overcame, it means that pt has stopped and we have only to stop
              //the other thread: klt

    klt.Stop();


    Good! Ok!

    ....but it doesn't work: main blocks to pt.Join() instruction and never goes on.
    What's wrong with that?

    the problem is that pt thread (the one we have joined to) never ends! Infact pt stops execution only
    when he receives the StopThreadEvent (take a look at "void parrot::run()" method), but nobody sends
    this event to him! Klt exits when someone presses 'q" or 'Q' (look at keyLogger loop function), but
    klt doesn't send a StopThreadEvent to pt! (nor he is expected to: he neither have to know that pt exists!)
    So the right thing to do is to join to a thread that someone can stop! (here is the user who presses
    keys on the keyboard).

    Then we have to join to klt, and stop pt when klt ends!

    */


    k.Join();//programs hangs here untill klt Thread doesn't stop his execution. When this
             //line will be overcame, it means that klt has stopped and we have only to stop
             //the other thread: pt

    p.Stop();

    //Note that if you don't know what to do (but this is not a good thing!!! Especially in multithread programming...)
    //sometime you can join to both and stop both (joining to a dead thread or stopping an already stopped thread has
    //no consequences: nothing literally will be done in these cases). However, in this specific example (we have chosen
    //it not randomly!!!!) if we join pt before klt, because of what explained above, the programs hangs because pt
    //will never stop and the source line "pt.Join()" will be never overcame!
}
