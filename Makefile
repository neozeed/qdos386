# QDOS/386 R1 - build for the i386VM harness (GCC 1.40 / GAS 1.38, a.out -> raw).
# Set I386VM to the i386VM tree (default: the extracted archive).
I386VM ?= $(HOME)/i386vm
CC = $(I386VM)/bin/i386-gcc
VM = $(I386VM)/bin/i386vm

SRC = kernel/main.c kernel/jobs.c kernel/platform.c kernel/context.s tests/test_jobs.c
INC = -Ikernel -Itests

all: build/qdos386.bin

build/qdos386.bin: $(SRC) kernel/jobs.h kernel/platform.h tests/test_jobs.h
	mkdir -p build
	$(CC) --raw $(INC) $(SRC) -o $@

run: build/qdos386.bin
	$(VM) build/qdos386.bin

clean:
	rm -f build/qdos386.bin

.PHONY: all run clean
