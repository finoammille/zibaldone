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

# compiler section
SHELL=/bin/bash

TARGET=x86_64 #i386
CC=g++
ARCH_CFLAGS=-fPIC
AR:=ar
DEBUG:=-g
ifeq ($(TARGET), i386)
	CC=g++ -m32
	ARCH_CFLAGS=
endif

CFLAGS:=-Wall -Werror $(DEBUG) $(ARCH_CFLAGS)

IFLAGS = -I./Events -I./Ipc -I./Log -I./SerialPort -I./Thread -I./Timer -I./Tools
LFLAGS = -lpthread

# dependencies
DEPS = ./Ipc/TcpIpc.h \
	   ./Ipc/PfLocalIpc.h \
	   ./Ipc/UdpIpc.h \
	   ./Log/Log.h \
	   ./SerialPort/SerialPort.h \
	   ./SerialPort/SerialPortHandler.h \
	   ./Thread/Thread.h \
	   ./Events/Events.h \
	   ./Timer/Timer.h \
	   ./Tools/Tools.h

# objects
OBJ_NAMES = ./Ipc/TcpIpc \
			./Ipc/PfLocalIpc \
			./Ipc/UdpIpc \
            ./Log/Log \
			./SerialPort/SerialPort \
			./SerialPort/SerialPortHandler \
			./Thread/Thread \
			./Events/Events \
			./Timer/Timer \
			./Tools/Tools

OBJ = $(foreach OBJ_NAME, $(OBJ_NAMES), $(OBJ_NAME).o)

# targets
%.o: %.cpp $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS) $(IFLAGS)

zibaldone: zibaldoneDynamic zibaldoneStatic

zibaldoneDynamic: $(OBJ)
	@echo "building $@"
#	$(CC) -o lib$@.so $(LFLAGS) -shared $(OBJ)
	$(CC) -o lib$@.so $(LFLAGS) -shared $(OBJ) -lrt
	mv  lib$@.so ./build/libzibaldone.so

zibaldoneStatic: $(OBJ)
	@echo "building $@"
	$(AR) rc lib$@.a $(OBJ) 
	ranlib lib$@.a
	mv  lib$@.a ./build/libzibaldone.a

clean:
	rm -f $(OBJ) ./build/libzibaldone.so ./build/libzibaldone.a

#test:
#	@echo machine=$(MACHINE) additional flags=$(ARCH_CFLAGS)

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
