#!/usr/bin/make -f
# Makefile for LinVst #

#target: prerequisites
#	./lin-patchwin
	
	
#.PHONY do_script:



#prerequisites: do_script 


    
CXX     = g++
WINECXX = wineg++

CXX_FLAGS =

PREFIX  = /usr

BUILD_FLAGS_WIN = -std=c++14 -m64 -O2 -DVESTIGE -I/usr/include/wine-development/windows -I/usr/include/wine-development/wine/windows -I/usr/include/wine/wine/windows -I../ -DRELEASE=1  -D__forceinline=inline -DNOMINMAX=1 -DUNICODE_OFF -Dstricmp=strcasecmp -Dstrnicmp=strncasecmp -DSMTG_RENAME_ASSERT=1 -fpermissive

LINK_FLAGS   = $(LDFLAGS)

LINK_WINE   = ../build/lib/Release/libbase.a ../build/lib/Release/libpluginterfaces.a ../build/lib/Release/libsdk.a -lole32 -lpthread -lrt -lshell32 -L/opt/wine-stable/lib64/wine -L/opt/wine-devel/lib64/wine -L/opt/wine-staging/lib64/wine -L/usr/lib/x86_64-linux-gnu/wine-development $(LINK_FLAGS)

TARGETS     = do_script do_script2 testvst3.exe

# --------------------------------------------------------------

all: $(TARGETS)

do_script: 
	$(shell chmod +x ./lin-patchwin)

do_script2: 
	./lin-patchwin
	
testvst3.exe: testvst3.wine.o
	$(WINECXX) $^ $(LINK_WINE) -o $@

# --------------------------------------------------------------

testvst3.wine.o: testvst3.cpp
	$(WINECXX) $(BUILD_FLAGS_WIN) -c $^ -o $@

