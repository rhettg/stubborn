TARGET = comsock
LIBS = -lm
CC = gcc
CFLAGS = -g -Wall -Isrc
OBJDIR = ./build

.PHONY: test all clean

all: $(TARGET)

# Search the correct subdirectories for targets
VPATH = src:build:test:CiC

CICOBJECTS = $(patsubst CiC/%.c, $(OBJDIR)/%.o, $(wildcard CiC/*.c))
OBJECTS = $(patsubst src/%.c, $(OBJDIR)/%.o, $(wildcard src/*.c))
HEADERS = $(wildcard *.h)

build/%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

.PRECIOUS: $(TARGET) $(OBJECTS)

$(TARGET): main.c $(OBJECTS) $(CICOBJECTS)
	$(CC) $(CFLAGS) $< $(OBJECTS) $(CICOBJECTS) -Wall $(LIBS) -o $@

clean:
	-rm -f $(OBJDIR)/*.o $(TARGET)