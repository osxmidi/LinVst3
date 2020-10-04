#!/usr/bin/make -f
# Makefile for LinVst #

CXX     = g++

CXX_FLAGS =

PREFIX  = /usr

BIN_DIR    = $(DESTDIR)$(PREFIX)/bin

BUILD_FLAGS  = `pkg-config --cflags gtk+-3.0` $(CXX_FLAGS)

LINK_FLAGS   = $(LDFLAGS)

LINK_APP = -no-pie `pkg-config --libs gtk+-3.0` $(LINK_FLAGS)

TARGETS     = testvst3gui

# --------------------------------------------------------------

all: $(TARGETS)

testvst3gui: testvst3gui.o
	$(CXX) $^ $(LINK_APP) -o $@
	
testvst3gui.o: testvst3gui.cpp
	$(CXX) $(BUILD_FLAGS) -c $^ -o $@	
	
clean:
	rm -fR *.o $(TARGETS)

install:

