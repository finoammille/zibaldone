###############################################################################
#
# zibTests - zibaldone tests and examples
#
# Copyright (C) 2012  bucc4neer (bucc4neer@users.sourceforge.net)
#
# This software is provided 'as-is', without any express or implied
# warranty.  In no event will the authors be held liable for any damages
# arising from the use of this software.
#
# Permission is granted to anyone to use this software for any purpose,
# including commercial applications, and to alter it and redistribute it
# freely, subject to the following restrictions:
#
# 1. The origin of this software must not be misrepresented; you must not
#    claim that you wrote the original software. If you use this software
#    in a product, an acknowledgment in the product documentation would be
#    appreciated but is not required.
# 2. Altered source versions must be plainly marked as such, and must not be
#    misrepresented as being the original software.
# 3. This notice may not be removed or altered from any source distribution.
#
# bucc4neer bucc4neer@users.sourceforge.net
#
###############################################################################

SHELL=/bin/bash

AR:=ar
DEBUG:=-g

#se target non specificato compilo sulla macchina locale
target=

ifeq ($(target), i386)
	CC=g++ -m32
	ARCH_CFLAGS=
else ifeq ($(target), arm)
	CC=arm-none-linux-gnueabi-g++
	AR:=arm-none-linux-gnueabi-ar
	ARCH_CFLAGS=-Darm
else ifeq ($(target), x86_64)
	CC=g++ -m64
	ARCH_CFLAGS=-fPIC
else
	CC=g++
	ifeq ($(shell uname -m), x86_64)
		ARCH_CFLAGS = -fPIC
	else
		ARCH_CFLAGS =
	endif
endif

DFLAGS= -D_ZDEBUG -D_REENTRANT

CXXFLAGS:=-Wall -Werror $(DEBUG) $(DFLAGS) $(ARCH_CFLAGS)

IFLAGS = -I../../../zibaldone/c++/linux/Events \
		 -I../../../zibaldone/c++/linux/Ipc \
		 -I../../../zibaldone/c++/linux/Log \
		 -I../../../zibaldone/c++/linux/Utils/SerialPort \
		 -I../../../zibaldone/c++/linux/Thread \
		 -I../../../zibaldone/c++/linux/Timer \
		 -I../../../zibaldone/c++/linux/Utils/StopAndWaitOverUdp \
		 -I../../../zibaldone/c++/linux/Utils/Tools
#		 -I/usr/include/cairo \
#		 -I/usr/include/pango-1.0 \
#		 -I/usr/include/glib-2.0 \
#		 -I/usr/lib/glib-2.0/include \
#		 -I/usr/lib/i386-linux-gnu/glib-2.0/include/

LFLAGS = -L../../../zibaldone/c++/linux/build \
		 -lzibaldone \
		 -lpthread \
		 -lrt

#N.B. rpath va calcolata rispetto alla posizione in cui viene messo l'exe! 
#In questo caso l'exe viene messo in ./build per cui occorre salire ulteriormente
#di uno rispetto a LFLAGS!!!
LDFLAGS = "-Wl,-rpath,../../../../zibaldone/c++/linux/build/"

# dependencies
DEPS = ../../../zibaldone/c++/linux/Events/Events.h \
       ../../../zibaldone/c++/linux/Ipc/PfLocalIpc.h \
       ../../../zibaldone/c++/linux/Ipc/TcpIpc.h \
       ../../../zibaldone/c++/linux/Ipc/UdpIpc.h \
	   ../../../zibaldone/c++/linux/Log/Log.h \
	   ../../../zibaldone/c++/linux/Utils/SerialPort/SerialPortHandler.h \
	   ../../../zibaldone/c++/linux/Utils/SerialPort/SerialPort.h \
	   ../../../zibaldone/c++/linux/Thread/Thread.h \
	   ../../../zibaldone/c++/linux/Timer/Timer.h \
	   ../../../zibaldone/c++/linux/Utils/StopAndWaitOverUdp/StopAndWaitOverUdp.h \
	   ../../../zibaldone/c++/linux/Utils/Tools/Tools.h

# objects
OBJ = ../src/main.o \
	  ../src/testOneLabel4MoreEvents.o \
	  ../src/testPfLocalIpc.o \
	  ../src/testSerialPort.o \
	  ../src/testTcpIpc.o \
	  ../src/testUdpIpc.o \
	  ../src/testThreads.o \
	  ../src/testAutonomousThread.o \
	  ../src/testTimers.o \
	  ../src/testStopAndWaitOverUdp.o \
	  ../src/testSameThreadAddressSpaceEventReceiver.o \
	  ../src/testZibTools.o

# targets
%.o: %.cpp $(DEPS)
	@echo "building $@"
	$(CC) -c -o $@ $< $(CXXFLAGS) $(IFLAGS)

zibTests: $(OBJ)
	@echo "building $@"
	$(CC) $(LDFLAGS) -o $@ $^ $(LFLAGS)
	mv zibTests ./build

clean:
	rm -f ./build/zibTests $(OBJ)

#all: clean zibTests
