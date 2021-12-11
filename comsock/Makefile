TARGET = comsock
LIBS = -lm
CC = gcc
CFLAGS = -g -Wall -Isrc -I../libraries/CiC/src -I../libraries/Stubborn/src
OBJDIR = ./build
DEBUGFLAGS = -DDEBUG

.PHONY: test all clean

all: $(TARGET)

# Search the correct subdirectories for targets
VPATH = src:build:test:CiC

CICOBJECTS = $(patsubst CiC/%.c, $(OBJDIR)/%.o, $(wildcard CiC/*.c))
OBJECTS = $(patsubst src/%.c, $(OBJDIR)/%.o, $(wildcard src/*.c))
HEADERS = $(wildcard CiC/*.h) $(wildcard src/*.h)

build/%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

.PRECIOUS: $(TARGET) $(OBJECTS)

$(TARGET): main.c $(OBJECTS) $(CICOBJECTS)
	$(CC) $(CFLAGS) $< $(OBJECTS) $(CICOBJECTS) $(LIBS) -o $@

# Target to enable debug option
debug: CFLAGS += $(DEBUGFLAGS)
debug: all

clean:
	-rm -f $(OBJDIR)/*.o $(TARGET)
