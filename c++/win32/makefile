########################################################################
#
# zibaldone - a C++/Java library for Thread, Timers and other Stuff
#
# Copyright (C) 2012  Antonio Buccino
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
	# mingw cross compiler su linux
	#CC = i586-mingw32msvc-gcc
	CC = i486-mingw32-gcc
	AR = i486-mingw32-ar
else
	# utilizzo di mingw su windows
    CC = g++
	AR = ar
endif

#---------------------------------------------------------------------------

CFLAGS = -Wall -Werror -O0 -g -Wl,--subsystem,windows -DDEBUG

#---------------------------------------------------------------------------

ifeq ($(shell uname), Linux)
	SYSHPATH = /usr/i486-mingw32/include
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
	 -I./SerialPort \
	 -I./Tools

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
	   SerialPort/SerialPort.h \
	   SerialPort/SerialPortHandler.h \
	   Timer/Timer.h \
	   Tools/Tools.h

# objects
OBJ = Thread/Thread.o \
	  Log/Log.o \
	  Timer/Timer.o \
	  Log/Log.o \
	  Ipc/TcpIpc.o \
	  Ipc/UdpIpc.o \
	  Events/Events.o \
	  SerialPort/SerialPort.o \
	  SerialPort/SerialPortHandler.o \
	  Tools/Tools.o

# targets
%.o: %.cpp $(DEPS)
	@echo "building $@"
	$(CC) -c -o $@ $< $(DFLAGS) $(CFLAGS) $(IFLAGS) 
#nota: $@ e' il nome del target (%.o), $< e' il nome della prima dipendenza (%.cpp)

zibaldone: zibaldoneDynamic zibaldoneStatic
#nota: $^ e' la lista delle dipendenze ($(OBJ))

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


# url utili:
# http://www.gnu.org/software/make/manual/make.html#Automatic-Variables
# http://www.cs.colby.edu/maxwell/courses/tutorials/maketutor/
# http://makepp.sourceforge.net/1.19/makepp_variables.html#input
# http://makepp.sourceforge.net/1.19/makepp_cookbook.html#Tips%20for%20multiple%20directories
# http://www.gnu.org/s/hello/manual/make/Phony-Targets.html
# http://www.bo.cnr.it/corsi-di-informatica/corsoCstandard/Lezioni/17Linuxmake.html
# http://cs.acadiau.ca/~jdiamond/comp2103/beginner-tutorials/LinuxTutorialGcc.html
# http://www.gnu.org/software/make/manual/make.html#toc_Rules
# http://mrbook.org/tutorials/make/
# http://www.cs.umd.edu/class/fall2002/cmsc214/Tutorial/makefile.html
# http://www.opussoftware.com/tutorial/TutMakefile.htm
# http://www.gnu.org/s/hello/manual/make/Text-Functions.html

# mix java and c++ in same makefile!
# http://stackoverflow.com/questions/2846708/makefile-to-compile-both-c-and-java-programs-at-the-same-time

# dll
# http://ilostmynotes.blogspot.it/2011/04/mingw-example-makefile-for-console-app.html
