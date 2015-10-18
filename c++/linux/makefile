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

# compiler section
SHELL=/bin/bash

AR:=ar
DEBUG:=-g

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

CFLAGS:=-Wall -Werror $(DEBUG) $(ARCH_CFLAGS)

IFLAGS = -I./Events -I./Ipc -I./Log -I./Utils/SerialPort -I./Thread -I./Timer -I./Utils/Tools
LFLAGS = -lpthread

# dependencies
DEPS = ./Ipc/TcpIpc.h \
	   ./Ipc/PfLocalIpc.h \
	   ./Ipc/UdpIpc.h \
	   ./Log/Log.h \
	   ./Utils/SerialPort/SerialPort.h \
	   ./Utils/SerialPort/SerialPortHandler.h \
	   ./Thread/Thread.h \
	   ./Events/Events.h \
	   ./Timer/Timer.h \
	   ./Utils/Tools/Tools.h \
	   ./Utils/StopAndWaitOverUdp/StopAndWaitOverUdp.h

# objects
OBJ_NAMES = ./Ipc/TcpIpc \
			./Ipc/PfLocalIpc \
			./Ipc/UdpIpc \
            ./Log/Log \
			./Utils/SerialPort/SerialPort \
			./Utils/SerialPort/SerialPortHandler \
			./Thread/Thread \
			./Events/Events \
			./Timer/Timer \
			./Utils/Tools/Tools \
			./Utils/StopAndWaitOverUdp/StopAndWaitOverUdp

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
