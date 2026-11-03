CC ?= gcc
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -Iinclude
SOURCES := $(filter-out src/main.c,$(wildcard src/*.c))

.PHONY: all clean test

all: simulator.exe

simulator.exe: src/main.c $(wildcard src/*.c) $(wildcard include/*.h)
	$(CC) $(CFLAGS) -o $@ src/*.c

test: tests/test_runner
	./tests/test_runner

tests/test_runner: tests/test_runner.c $(SOURCES) $(wildcard include/*.h)
	$(CC) $(CFLAGS) -o $@ tests/test_runner.c $(SOURCES)

clean:
	rm -f simulator.exe tests/test_runner
