# Makefile for Irrlicht Examples
# It's usually sufficient to change just the target name and source file list
# and be sure that CXX is set to a valid compiler
TARGET = test
SOURCES = $(wildcard src/*.cpp)
OBJECTS = $(SOURCES:.cpp=.o)

IRRLICHTPATH = /usr/include/irrlicht
JTHREADPATH = jthread

CPPFLAGS = -I/usr/include/X11 -I/usr/include/irrlicht -I$(HOME)/jsoncpp

#CXXFLAGS = -O3 -ffast-math -Wall
#CXXFLAGS = -O3 --fast-math -Wall -g
#CXXFLAGS = -Wall -g -O0
CXXFLAGS = -O2 --fast-math -Wall -g

#Default target

all: all_linux

ifeq ($(HOSTTYPE), x86_64)
LIBSELECT=64
endif

# Target specific settings

all_linux: LDFLAGS = -lIrrlicht -lGL -lXxf86vm -lXext -lX11 -ljthread
all_linux clean_linux: SYSTEM=Linux

# Name of the binary - only valid for targets which set SYSTEM

DESTPATH = bin/$(TARGET)

# Build commands

all_linux: $(DESTPATH)

$(DESTPATH): $(OBJECTS)
	$(CXX) -o $@ $(OBJECTS) $(LDFLAGS)

.cpp.o:
	$(CXX) -c -o $@ $< $(CPPFLAGS) $(CXXFLAGS)

clean: clean_linux

clean_linux:
	@$(RM) $(OBJECTS) $(DESTPATH)

.PHONY: all all_clean clean_linux
