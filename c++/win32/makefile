###############################################################################
#
# zibaldone - a C++/Java library for Thread, Timers and other Stuff
#
# http://sourceforge.net/projects/zibaldone/
#
# version 3.1.2, August 29th, 2015
#
# Copyright (C) 2012  ilant (ilant@users.sourceforge.net)
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
# ilant ilant@users.sourceforge.net
#
###############################################################################

SHELL=/bin/bash

ifeq ($(shell uname), Linux)
	# mingw cross compiler on linux
	CC = i686-w64-mingw32-g++
	#CC = i486-mingw32-gcc
	AR = i686-w64-mingw32-ar
	#AR = i486-mingw32-ar
else
	# use of mingw on windows
    CC = g++
	AR = ar
endif

#---------------------------------------------------------------------------

CFLAGS = -Wall -Werror -O0 -g -Wl,--subsystem,windows -DDEBUG

#---------------------------------------------------------------------------

ifeq ($(shell uname), Linux)
	#SYSHPATH = /usr/i486-mingw32/include
	SYSHPATH = /usr/i686-w64-mingw32/include
else
	SYSHPATH = /MinGW/include
endif

IFLAGS = -I. \
	 -I./Thread \
	 -I./Log \
	 -I$(SYSHPATH) \
	 -I./Timer \
	 -I./Ipc \
	 -I./Events \
	 -I./Utils/SerialPort \
	 -I./Utils/Tools

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

LFLAGS = $(SYSLFLAG) -lstdc++ -lws2_32

#---------------------------------------------------------------------------

DLLFLAGS = -Wall -O0 -g -shared -Wl,--subsystem,windows,--kill-at -DDLL

ARFLAGS = rcs

#---------------------------------------------------------------------------

# dependencies
DEPS = Thread/Thread.h \
	   Log/Log.h \
	   Ipc/TcpIpc.h \
	   Ipc/UdpIpc.h \
	   Events/Events.h \
	   Utils/SerialPort/SerialPort.h \
	   Utils/SerialPort/SerialPortHandler.h \
	   Timer/Timer.h \
	   Utils/Tools/Tools.h \
	   Utils/StopAndWaitOverUdp/StopAndWaitOverUdp.h

# objects
OBJ = Thread/Thread.o \
	  Log/Log.o \
	  Timer/Timer.o \
	  Log/Log.o \
	  Ipc/TcpIpc.o \
	  Ipc/UdpIpc.o \
	  Events/Events.o \
	  Utils/SerialPort/SerialPort.o \
	  Utils/SerialPort/SerialPortHandler.o \
	  Utils/Tools/Tools.o \
	  Utils/StopAndWaitOverUdp/StopAndWaitOverUdp.o

# targets
%.o: %.cpp $(DEPS)
	@echo "building $@"
	$(CC) -c -o $@ $< $(DFLAGS) $(CFLAGS) $(IFLAGS)

zibaldone: zibaldoneDynamic zibaldoneStatic

zibaldoneDynamic: $(OBJ)
	@echo "building $@"
	$(CC) -o $@.dll $(DLLFLAGS) $^ $(LFLAGS)
	mv $@.dll ./build/zibaldoneWin32.dll

zibaldoneStatic: $(OBJ)
	@echo "building $@"
	$(AR) $(ARFLAGS) lib$@.a $(^)
	mv lib$@.a ./build/libzibaldoneWin32.a

clean:
	rm -f ./build/zibaldoneWin32.dll ./build/libzibaldoneWin32.a $(OBJ)
