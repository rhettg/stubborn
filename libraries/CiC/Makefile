TARGET = build/to-test
LIBS = -lm
CC = gcc
CFLAGS = -g -Wall -Isrc
OBJDIR = ./build

.PHONY: default test all clean

default: build/to.o
all: default

# Search the correct subdirectories for targets
VPATH = src:build:test

TEST_OBJECTS = $(patsubst test/%.c, $(OBJDIR)/%.o, $(wildcard test/*.c))
OBJECTS = $(patsubst src/%.c, $(OBJDIR)/%.o, $(wildcard src/*.c))
HEADERS = $(wildcard *.h)

build/%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

.PRECIOUS: $(TARGET) $(OBJECTS)

build/test_%: test_%.c $(OBJECTS)
	$(CC) $(CFLAGS) $< $(OBJECTS) -Wall $(LIBS) -o $@

test: build/test_to build/test_evt build/test_com build/test_tmr
	./build/test_to
	./build/test_evt
	./build/test_com
	./build/test_tmr

clean:
	-rm -f $(OBJDIR)/*.o $(TARGET)