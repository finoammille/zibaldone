########################################################################
#
# zibaldone - a C++ library for Thread, Timers and other Stuff
# http://sourceforge.net/projects/zibaldone/
#
# Copyright (C) 2012  ilant (ilant@users.sourceforge.net)
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, version 2.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
########################################################################

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
