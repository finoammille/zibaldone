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
