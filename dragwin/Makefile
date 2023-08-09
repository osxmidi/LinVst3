#!/usr/bin/make -f
# Makefile for LinVst #

#target: prerequisites
#	./lin-patchwin
	
	
#.PHONY do_script:



#prerequisites: do_script 


    
CXX     = g++
WINECXX = wineg++ -Wl,--subsystem,windows 

CXX_FLAGS =

PREFIX  = /usr

BIN_DIR    = $(DESTDIR)$(PREFIX)/bin
VST_DIR = ./vst

BUILD_FLAGS  = -std=c++14 -fPIC -m64 -O2 -DLVRT -DEMBED -DEMBEDDRAG -DTRACKTIONWM -DVESTIGE -DNEWTIME -DINOUTMEM -DCHUNKBUF -DEMBEDRESIZE -DPCACHE -DBIN_DIR='"$(BIN_DIR)"' $(CXX_FLAGS)

BUILD_FLAGS_WIN = -std=c++14 -fPIC -m64 -O2 -DEMBED -DEMBEDDRAG -DWAVES2 -DTRACKTIONWM -DVESTIGE -DNEWTIME -DINOUTMEM -DCHUNKBUF -DEMBEDRESIZE -DPCACHE -DDRAGWIN -I/opt/wine-staging/include/wine/windows -I/opt/wine-stable/include/wine/windows -I/opt/wine-devel/include/wine/windows -I/usr/include/wine-development/windows -I/usr/include/wine-development/wine/windows -I/usr/include/wine/wine/windows -I../ -DRELEASE=1  -D__forceinline=inline -DNOMINMAX=1 -DUNICODE_OFF -Dstricmp=strcasecmp -Dstrnicmp=strncasecmp -DSMTG_RENAME_ASSERT=1 -fpermissive

#-DUNICODE_OFF
LINK_FLAGS   = $(LDFLAGS)

LINK_PLUGIN = -shared -lpthread -ldl -lX11 $(LINK_FLAGS)
LINK_WINE   = ../build/lib/Release/libsdk_hosting.a ../build/lib/Release/libbase.a ../build/lib/Release/libpluginterfaces.a ../build/lib/Release/libsdk.a -L/opt/wine-stable/lib64/wine -L/opt/wine-devel/lib64/wine -L/opt/wine-staging/lib64/wine -L/opt/wine-stable/lib64/wine/x86_64-unix -L/opt/wine-devel/lib64/wine/x86_64-unix -L/opt/wine-staging/lib64/wine/x86_64-unix -L/usr/lib/x86_64-linux-gnu/wine-development -lpthread -lX11 -lrt -lshell32 -lole32 $(LINK_FLAGS)

TARGETS     = do_script do_script2 linvst3.so lin-vst3-server.exe

PATH := $(PATH):/opt/wine-stable/bin:/opt/wine-devel/bin:/opt/wine-staging/bin

# --------------------------------------------------------------

all: $(TARGETS)

do_script: 
	$(shell chmod +x ./lin-patchwin)

do_script2: 
	./lin-patchwin

linvst3.so: linvst.unix.o remotevstclient.unix.o remotepluginclient.unix.o paths.unix.o
	$(CXX) $^ $(LINK_PLUGIN) -o $@
	
lin-vst3-server.exe: lin-vst-server.wine.o remotepluginserver.wine.o paths.wine.o memorystream.wine.o basewrapper.wine.o 
	$(WINECXX) $^ $(LINK_WINE) -o $@

# --------------------------------------------------------------

linvst.unix.o: linvst.cpp
	$(CXX) $(BUILD_FLAGS) -c $^ -o $@
	
remotevstclient.unix.o: remotevstclient.cpp
	$(CXX) $(BUILD_FLAGS) -c $^ -o $@
	
remotepluginclient.unix.o: remotepluginclient.cpp
	$(CXX) $(BUILD_FLAGS) -c $^ -o $@

paths.unix.o: paths.cpp
	$(CXX) $(BUILD_FLAGS) -c $^ -o $@


# --------------------------------------------------------------

lin-vst-server.wine.o: lin-vst-server.cpp
	$(WINECXX) $(BUILD_FLAGS_WIN) -c $^ -o $@

remotepluginserver.wine.o: remotepluginserver.cpp
	$(WINECXX) $(BUILD_FLAGS_WIN) -c $^ -o $@

paths.wine.o: paths.cpp
	$(WINECXX) $(BUILD_FLAGS_WIN) -c $^ -o $@
		
memorystream.wine.o: ../public.sdk/source/common/memorystream.cpp
	$(WINECXX) $(BUILD_FLAGS_WIN) -c $^ -o $@							
	
basewrapper.wine.o: basewrapper.cpp
	$(WINECXX) $(BUILD_FLAGS_WIN) -c $^ -o $@	

clean:
	rm -fR *.o *.exe *.so vst $(TARGETS)

install:
	install -d $(BIN_DIR)
	install -d $(VST_DIR)
	install -m 755 linvst3.so $(VST_DIR)
	install -m 755 lin-vst3-server.exe lin-vst3-server.exe.so $(BIN_DIR)
