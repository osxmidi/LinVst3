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

BUILD_FLAGS_WIN = -std=c++14 -fPIC -m64 -O2 -DVESTIGE -I/opt/wine-staging/include/wine/windows -I/opt/wine-stable/include/wine/windows -I/opt/wine-devel/include/wine/windows -I/usr/include/wine-development/windows -I/usr/include/wine-development/wine/windows -I/usr/include/wine/wine/windows -I../ -DRELEASE=1  -D__forceinline=inline -DNOMINMAX=1 -DUNICODE_OFF -Dstricmp=strcasecmp -Dstrnicmp=strncasecmp -DSMTG_RENAME_ASSERT=1 -fpermissive

LINK_FLAGS   = $(LDFLAGS)

LINK_WINE   = ../build/lib/Release/libsdk_hosting.a ../build/lib/Release/libbase.a ../build/lib/Release/libpluginterfaces.a ../build/lib/Release/libsdk.a -L/opt/wine-stable/lib64/wine -L/opt/wine-devel/lib64/wine -L/opt/wine-staging/lib64/wine -L/opt/wine-stable/lib64/wine/x86_64-unix -L/opt/wine-devel/lib64/wine/x86_64-unix -L/opt/wine-staging/lib64/wine/x86_64-unix -L/usr/lib/x86_64-linux-gnu/wine-development -lpthread -lX11 -lrt -lshell32 -lole32 $(LINK_FLAGS)

TARGETS     = do_script do_script2 testvst3.exe

# --------------------------------------------------------------

all: $(TARGETS)

do_script: 
	$(shell chmod +x ./lin-patchwin)

do_script2: 
	./lin-patchwin
	
testvst3.exe: testvst3.wine.o memorystream.wine.o basewrapper.wine.o 
	$(WINECXX) $^ $(LINK_WINE) -o $@

# --------------------------------------------------------------

testvst3.wine.o: testvst3.cpp
	$(WINECXX) $(BUILD_FLAGS_WIN) -c $^ -o $@
	
memorystream.wine.o: ../public.sdk/source/common/memorystream.cpp
	$(WINECXX) $(BUILD_FLAGS_WIN) -c $^ -o $@							
	
basewrapper.wine.o: basewrapper.cpp
	$(WINECXX) $(BUILD_FLAGS_WIN) -c $^ -o $@	
	

