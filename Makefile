# QDOS/386 R1 - build both targets.
#   make i386vm   : flat image for the i386VM harness (synthetic timer)
#   make pc       : Tiny386 PC disk image (SeaBIOS boot sector, PIT/PIC timer)
#   make run-i386vm / make run-pc : run in the emulator
#
# Paths to external trees (override on the command line):
I386VM   ?= $(HOME)/i386vm
TINY386  ?= $(HOME)/tiny386/tiny386_headless
SEABIOS  ?= $(HOME)/seabios/out
GCC  = $(I386VM)/bin/i386-gcc
AS   = $(I386VM)/bin/as
LD   = $(I386VM)/bin/i386-ld
VM   = $(I386VM)/bin/i386vm

INC = -Ikernel -Itests -Iplatform
KCORE = kernel/main.c kernel/jobs.c kernel/descriptors.c tests/test_jobs.c

all: i386vm pc

# ---- i386VM ---------------------------------------------------------------
I386VM_SRC = $(KCORE) kernel/context.s platform/i386vm/platform.c platform/i386vm/halt.s
i386vm: build/i386vm/qdos386.bin

build/i386vm/qdos386.bin: $(I386VM_SRC) $(wildcard kernel/*.h) $(wildcard tests/*.h) platform/platform.h
	mkdir -p build/i386vm
	$(GCC) --raw $(INC) $(I386VM_SRC) -o $@

run-i386vm: build/i386vm/qdos386.bin
	$(VM) build/i386vm/qdos386.bin

# ---- Tiny386 PC -----------------------------------------------------------
PC_OBJS = build/pc/main.o build/pc/jobs.o build/pc/descriptors.o \
          build/pc/test_jobs.o build/pc/context.o build/pc/platform.o

pc: build/pc/r1pc.img

build/pc/%.o: kernel/%.c
	mkdir -p build/pc
	$(GCC) -c $(INC) $< -o $@
build/pc/%.o: tests/%.c
	mkdir -p build/pc
	$(GCC) -c $(INC) $< -o $@
build/pc/%.o: platform/pc/%.c
	mkdir -p build/pc
	$(GCC) -c $(INC) $< -o $@
build/pc/context.o: kernel/context.s
	mkdir -p build/pc
	$(AS) -o $@ $<
build/pc/entry.o: platform/pc/entry.s
	mkdir -p build/pc
	$(AS) -o $@ $<

build/pc/kernel.aout: build/pc/entry.o $(PC_OBJS) platform/pc/kernel.ld
	$(LD) -N -T platform/pc/kernel.ld -e kstart -o $@ build/pc/entry.o $(PC_OBJS)

build/pc/kernel.raw: build/pc/kernel.aout
	python3 platform/pc/extract-kernel.py $< $@ 0x100000

build/pc/r1pc.img: build/pc/kernel.raw platform/pc/boot.bin
	python3 platform/pc/make-image.py platform/pc/boot.bin build/pc/kernel.raw $@

platform/pc/boot.bin: platform/pc/boot.s
	cd platform/pc && $(AS) -o boot.o boot.s && $(LD) -N -T boot.ld -e start -o boot.aout boot.o && python3 $(I386VM)/tools/aout-to-raw.py boot.aout boot.bin

build/pc/run.ini: build/pc/r1pc.img
	printf '[pc]\nbios = %s/bios.bin\nvga_bios = %s/vgabios.bin\nmem_size = 8M\nhda = %s\n' \
	  "$(SEABIOS)" "$(SEABIOS)" "$(CURDIR)/build/pc/r1pc.img" > $@

run-pc: build/pc/run.ini
	cd build/pc && $(TINY386) run.ini

clean:
	rm -rf build platform/pc/boot.o platform/pc/boot.aout platform/pc/boot.bin

.PHONY: all i386vm pc run-i386vm run-pc clean
