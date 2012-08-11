########################################################################
#
# zibaldone - a C++ library for Thread, Timers and other Stuff
#
# Copyright (C) 2012  Antonio Buccino
# 
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 2 of the License, or
# (at your option) any later version.
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
CC = g++
AR = ar
DEBUG = -g

#MACHINE = $(shell uname -m)
#ifeq ($(MACHINE), x86_64)
#	ARCH_CFLAGS = -fPIC
#else
#	ARCH_CFLAGS =
#endif
ifeq ($(shell uname -m), x86_64)
	ARCH_CFLAGS = -fPIC
else
	ARCH_CFLAGS =
endif

CFLAGS = -Wall -Werror $(DEBUG) $(ARCH_CFLAGS)

IFLAGS = -I./Ipc -I./Log -I./SerialPort/LinuxSerialPort -I./SerialPort -I./Thread -I./Timer
LFLAGS = -lpthread

# dependencies
DEPS = ./Ipc/Socket.h \
	   ./Log/Log.h \
	   ./SerialPort/LinuxSerialPort/LinuxSerialPort.h \
	   ./SerialPort/SerialPortHandler.h \
	   ./Thread/Thread.h \
	   ./Timer/Timer.h

# objects
OBJ_NAMES = ./Ipc/Socket \
            ./Log/Log \
			./SerialPort/LinuxSerialPort/LinuxSerialPort \
			./SerialPort/SerialPortHandler \
			./Thread/Thread \
			./Timer/Timer
OBJ_O = $(foreach OBJ_NAME, $(OBJ_NAMES), $(OBJ_NAME).o)
OBJ_OS = $(foreach OBJ_NAME, $(OBJ_NAMES), $(OBJ_NAME).os)

# targets
%.o: %.cpp $(DEPS)
	@echo "building $@"
	$(CC) -c -o $@ $< $(CFLAGS) $(IFLAGS)

%.os: %.cpp $(DEPS)
	@echo "building $@"
	$(CC) -c -o $@ $< $(CFLAGS) $(IFLAGS)

zibaldone: $(OBJ_O) $(OBJ_OS)
	@echo "building $@"
	$(AR) rc lib$@.a $(OBJ_O) 
	ranlib lib$@.a
	mv  lib$@.a ./build
	@echo "building $@"
	$(CC) -o lib$@.so $(LFLAGS) -shared $(OBJ_OS)
	mv  lib$@.so ./build

clean:
	rm -f $(OBJ) $(OBJ_OS) ./build/libzibaldone.so ./build/libzibaldone.a

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
