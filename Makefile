CC ?= cc
CFLAGS ?= -std=c99 -Wall -Wextra -pedantic -O2
CPPFLAGS ?= -I.
LDLIBS ?= -lm

.PHONY: all example test clean

all: example

build/example: examples/example.c cbounce.h
	mkdir -p build
	$(CC) $(CPPFLAGS) $(CFLAGS) $< -o $@ $(LDLIBS)

example test: build/example
	./build/example

clean:
	rm -rf build
