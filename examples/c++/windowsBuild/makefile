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

ifeq ($(shell uname), Linux)
	# mingw cross compiler su linux
	CC = i686-w64-mingw32-g++
	#CC = i486-mingw32-gcc
else
	# utilizzo di mingw su windows
    CC = g++
endif

#---------------------------------------------------------------------------

#MACHINE = $(shell uname -m)
#ifeq ($(MACHINE), x86_64)
#   ARCH_CFLAGS = -fPIC
#else
#   ARCH_CFLAGS =
#endif

CXXFLAGS = -Wall -Werror -O0 -g -Wl,--subsystem,windows -DDEBUG -D_ZDEBUG -D_REENTRANT

#---------------------------------------------------------------------------

ifeq ($(shell uname), Linux)
	#SYSHPATH = /usr/i486-mingw32/include
	SYSHPATH = /usr/i686-w64-mingw32/include
else
	SYSHPATH = /MinGW/include
endif

IFLAGS = -I. \
	 -I../../../zibaldone/c++/win32/Events \
	 -I../../../zibaldone/c++/win32/Thread \
	 -I../../../zibaldone/c++/win32/Log \
	 -I$(SYSHPATH) \
	 -I../../../zibaldone/c++/win32/Timer \
	 -I../../../zibaldone/c++/win32/Ipc \
	 -I../../../zibaldone/c++/win32/Utils/SerialPort \
	 -I../../../zibaldone/c++/win32/Utils/StopAndWaitOverUdp \
	 -I../../../zibaldone/c++/win32/Utils/Tools

#---------------------------------------------------------------------------


# DFLAGS for win vista (build zibaldoneWin32.dll for windows Vista, Seven, Eight
DFLAGS = -DWINVER=0x0600 -D__WINDOWS__ -DHAVE_W32API_H -D__GNUWIN32__ -DSTRICT -D__WXMSW__

# DFLAGS for win xp (build zibaldoneWin32.dll for windows Xp
#DFLAGS = -DWINVER=0x0500 -D__WINDOWS__ -DHAVE_W32API_H -D__GNUWIN32__ -DSTRICT -D__WXMSW__


#DFLAGS = -Wl,--subsystem,windows -mwindows \
#		 -DWINVER=0x0400 -D__WIN95__ -D__GNUWIN32__ \
#		 -DSTRICT -DHAVE_W32API_H -D__WXMSW__ -D__WINDOWS__ -D__i486__ -D_WIN32_WINNT=0x0500


#---------------------------------------------------------------------------

ifeq ($(shell uname), Linux)
	SYSLFLAG =
else
	SYSLFLAG = -L/MinGW/lib
endif

LFLAGS = -L../../../zibaldone/c++/win32/build/ -lzibaldoneWin32 $(SYSLFLAG) -lstdc++ -lws2_32 

#---------------------------------------------------------------------------

# dependencies
DEPS = ../../../zibaldone/c++/win32/Ipc/TcpIpc.h \
	   ../../../zibaldone/c++/win32/Ipc/UdpIpc.h \
	   ../../../zibaldone/c++/win32/Events/Events.h \
	   ../../../zibaldone/c++/win32/Thread/Thread.h \
	   ../../../zibaldone/c++/win32/Log/Log.h \
	   ../../../zibaldone/c++/win32/Timer/Timer.h \
	   ../../../zibaldone/c++/win32/Utils/SerialPort/SerialPortHandler.h \
	   ../../../zibaldone/c++/win32/Utils/SerialPort/SerialPort.h \
	   ../../../zibaldone/c++/win32/Utils/StopAndWaitOverUdp/StopAndWaitOverUdp.h \
	   ../../../zibaldone/c++/win32/Utils/Tools/Tools.h

# objects
OBJ = ../src/main.obj \
	  ../src/testOneLabel4MoreEvents.obj \
	  ../src/testSerialPort.obj \
	  ../src/testTcpIpc.obj \
	  ../src/testUdpIpc.obj \
	  ../src/testThreads.obj \
	  ../src/testAutonomousThread.obj \
	  ../src/testTimers.obj \
	  ../src/testStopAndWaitOverUdp.obj \
	  ../src/testSameThreadAddressSpaceEventReceiver.obj \
	  ../src/testZibTools.obj

# targets
%.obj: %.cpp $(DEPS)
	@echo "building $@"
	$(CC) -c -o $@ $< $(DFLAGS) $(CXXFLAGS) $(IFLAGS) 
#nota: $@ e' il nome del target (%.o), $< e' il nome della prima dipendenza (%.cpp)

zibTests.exe: $(OBJ)
	@echo "building $@"
#$(CC) -g -o $@ $^ $(LFLAGS)
	$(CC) -static -g -o $@ $^ $(LFLAGS)
	mv zibTests.exe ./build
#note: 
#	$^ e' la lista delle dipendenze ($(OBJ))
#	aggiunta opzione -static per linkare staticamente le librerie standard
#   senza -static occorre avere tutte le librerie (zibaldone e standard)
clean:
	rm -f ./build/zibTests.exe $(OBJ)
