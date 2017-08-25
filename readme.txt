kick off:

1) download and extract zibaldone_4.0.0.zip

=> windows visual studio 2010:
lib:
1) create a new empty c++ project named "zibaldone"
2) add all source files of zibaldone to the project (<extracted files>\zibaldone_4.0.0\lib\c++\win32\Events\Events.cpp, Event.h, ....)
3) edit project properties:
   Configuration Properties -> c/c++ -> General -> Additional Include Directories: add all path for zibaldone header files, i.e. 
     <extracted files>\zibaldone_4.0.0\lib\c++\win32\Events\
     <extracted files>\zibaldone_4.0.0\lib\c++\win32\Ipc\
     <extracted files>\zibaldone_4.0.0\lib\c++\win32\Log\
     <extracted files>\zibaldone_4.0.0\lib\c++\win32\Utils\SerialPort\
     <extracted files>\zibaldone_4.0.0\lib\c++\win32\Thread\
     <extracted files>\zibaldone_4.0.0\lib\c++\win32\Timer\
     <extracted files>\zibaldone_4.0.0\lib\c++\win32\Utils\Tools\
4) build a lib instead of an exe:
   Configuration Properties -> General -> Configuration Type: Static lib (lib) or dynamic lib (dll)    
5) build solution (F7). After this step you'll have a zibaldone.lib (if the name is different, you should rename it to zibaldone.lib or modify consistently the
   line "#pragma comment(lib, "zibaldone.lib")" in examples\main.cpp)

Note: zibaldone uses snprintf on linux side. In visual studio 2010 snprintf is not available 
     (it is available in visual studio 2015) so we use _snprintf and this gives an annoying 
     warning that can be disabled by defining the macro _CRT_SECURE_NO_WARNINGS

examples:
1) create a new c++ empty project
2) add all source files of examples to the project (<extracted files>/zibaldone_4.0.0/examples/src/main.cpp, ...) except for testPfLocalIpc.cpp that works on linux only.
3) edit project properties:
   Configuration Properties -> c/c++ -> General -> Additional Include Directories: add all path for zibaldone header files, i.e. 
     <extracted files>\zibaldone_4.0.0\lib\c++\win32\Events\
     <extracted files>\zibaldone_4.0.0\lib\c++\win32\Ipc\
     <extracted files>\zibaldone_4.0.0\lib\c++\win32\Log\
     <extracted files>\zibaldone_4.0.0\lib\c++\win32\Utils\SerialPort\
     <extracted files>\zibaldone_4.0.0\lib\c++\win32\Thread\
     <extracted files>\zibaldone_4.0.0\lib\c++\win32\Timer\
     <extracted files>\zibaldone_4.0.0\lib\c++\win32\Utils\Tools\
4) edit project properties:
   Configuration Properties -> Linker -> Additional lib Directories: path to zibaldone.lib built previously
   (search for where visual studio saved it: it should be in visual studio projects directory, zibaldone, Debug)
5) edit project properties:
   Configuration Properties -> Linker -> Additional Dependencies: add zibaldone.lib (or zibaldone.dll) built previously
6) build solution (F7). After this step you'll have a zibTests.exe
7) The available tests (take a look to the main.cpp file, rows 61-69) are:

   zibTests.exe --testTcp
   zibTests.exe --testUdp
   zibTests.exe --testThreads
   zibTests.exe --testAutonomousThread
   zibTests.exe --testOneLabel4MoreEvents
   zibTests.exe --testTools
   zibTests.exe --testSerialPort
   zibTests.exe --testTimers
   zibTests.exe --testSameThreadAddressSpaceEventReceiver

   you can either launch a test from a command line or use visual c++ debugger (f5). In the latter case you have to edit
   Configuration Properties -> Debugging -> Command Arguments
   specifying the argument (for example --testTimers)

   Note: if you launch a Tcp or a Udp test, windows will ask you to allow the exe across the firewall!

=> windows mingw:
1) launch msys console
2) change directory to lib source files for win32 (<extracted files>/zibaldone_4.0.0/lib/c++/win32)
3) type "make" to compile the lib. After this the build directory will contain libzibaldoneWin32.a and
   libzibaldoneWin32.dll
4) change directory to examples makefile for win32 (<extracted files>/zibaldone_4.0.0/examples/windowsMakefile)
   type "make". After this step, the directory "build" (<extracted files>/zibaldone_4.0.0/examples/windowsMakefile/build) 
   will contain "zibTests.exe".
   
   The available tests (take a look to the main.cpp file, rows 61-69) are:
   zibTests.exe --testTcp
   zibTests.exe --testUdp
   zibTests.exe --testThreads
   zibTests.exe --testAutonomousThread
   zibTests.exe --testOneLabel4MoreEvents
   zibTests.exe --testTools
   zibTests.exe --testSerialPort
   zibTests.exe --testTimers
   zibTests.exe --testSameThreadAddressSpaceEventReceiver

   you can also decide the log level and if view the log messages in the console (as well, they are written in a file)

   For example, to do Tcp test:
   - open 2 msys console (you can use cmd.exe but you have to specify the path to libgcc_s_dw2-1.dll) and change to the directory containing zibTests.exe (<extracted files>/zibaldone_4.0.0/examples/windowsMakefile/build)
   - in the first one type: zibTests.exe --testTcp and choose "1" (start listening server), choose an arbitrary port number (for example 7777), and if asked for, allow the application zibTests.exe tp access network (private network, i.e. no internet access, is sufficient since we're going to connect two consoles in the same pc via TCP socket).
   - in the second one type: zibTests.exe --testTcp and choose "2" (create client and connect to listening server), type the ip address of the pc where was opened the first console, (for example 192.168.1.100), insert the port number the server is listening to (the port you specified when you launched the server, in our example it was 7777)
   - you'll see then in both consoles the message "... connection done!SocketTest>> type exit to finish". Now you can type anything in a console and receive those data via TCP in the second one (type "enter" to view the data in the second one)
   type "exit" in both consoles to terminate the program

=> linux:
   same as windows mingw except you have to use the makefile in linux folder!
   You can also cross compile from linux to windows installing mingw in linux and compiling using windows mingw makefile

=> java:
   change directory to java lib, where it's present the build.xml ant file
   compile the jar library (ant build)
   
   to test the library, change directory to examples/java
   compile (ant build)
   change directory to examples/java/build and start the program (java -jar zibTests.jar --testXXX)

=> If you have any question, please contact me at bucc4neer@users.sourceforge.net

-----------------------------------------------------------------------------------

release notes (main differences only):

=> rev 4.0.0: + java support (now zibaldone works in Android too!)

=> rev 3.2.0: + POSIX Spurious wakeup prevention
              = fix of minor bugs

=> rev 3.1.2: = fix of minor bugs
              = removed some unnecessary copy-constructors 
                and assignment operator overload (no need for
                deep copy and default are ok!)

=> rev 3.1.1: + StopAndWaitOverUdp
              + SameThreadAddressSpaceEventReceiver
              fixed a bug in pullOut timeout

=> rev 3.0.1: fix a minor bug on stop thread

