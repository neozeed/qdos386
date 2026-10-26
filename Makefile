# QDOS/386 R5 - build both targets.
#   make i386vm   : flat image for the i386VM harness, with the transfer area
#   make pc       : Tiny386 PC disk image (SeaBIOS boot sector, transfer area)
#   make apps     : the two a.out applications and the transfer area
#   make notimer  : i386VM negative test (timer never started)
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
KCORE = kernel/main.c kernel/jobs.c kernel/memory.c kernel/channel.c kernel/irq.c kernel/descriptors.c \
        kernel/aout.c tests/test_jobs.c tests/test_memory.c tests/test_channels.c tests/test_app.c

all: i386vm pc

# ---- applications (independently linked; the kernel never links them) ------
APPS = build/apps/hello.aout build/apps/prep.aout

build/apps/%.o: apps/hello/%.c apps/common/appcalls.h
	mkdir -p build/apps
	$(GCC) -c -Iapps/common $< -o $@
build/apps/prep.o: apps/prep/prep.c apps/common/appcalls.h
	mkdir -p build/apps
	$(GCC) -c -Iapps/common $< -o $@
build/apps/app_start.o: apps/common/app_start.s
	mkdir -p build/apps
	$(AS) -o $@ $<
build/apps/qtrap.o: apps/common/qtrap.s
	mkdir -p build/apps
	$(AS) -o $@ $<

build/apps/hello.aout: build/apps/hello.o build/apps/app_start.o build/apps/qtrap.o apps/common/slot0.ld
	$(LD) -N -T apps/common/slot0.ld -e _start -o $@ build/apps/app_start.o build/apps/hello.o build/apps/qtrap.o
build/apps/prep.aout: build/apps/prep.o build/apps/app_start.o build/apps/qtrap.o apps/common/slot1.ld
	$(LD) -N -T apps/common/slot1.ld -e _start -o $@ build/apps/app_start.o build/apps/prep.o build/apps/qtrap.o

apps: $(APPS)

# Transfer area: directory plus the exact a.out bytes.  HEL2 is a second copy
# of hello (slot 0), used to test the busy-slot refusal.
build/xfer.bin: $(APPS) tools/make-xfer.py
	mkdir -p build
	python3 tools/make-xfer.py $@ HELO=build/apps/hello.aout PREP=build/apps/prep.aout HEL2=build/apps/hello.aout

# ---- i386VM ---------------------------------------------------------------
I386VM_SRC = $(KCORE) kernel/context.s platform/i386vm/platform.c platform/i386vm/halt.s

build/i386vm/kernel-raw.bin: $(I386VM_SRC) $(wildcard kernel/*.h) $(wildcard tests/*.h) platform/platform.h
	mkdir -p build/i386vm
	$(GCC) --raw $(INC) $(I386VM_SRC) -o $@

build/i386vm/qdos386-r5.bin: build/i386vm/kernel-raw.bin build/xfer.bin tools/make-vm-image.py
	python3 tools/make-vm-image.py build/i386vm/kernel-raw.bin build/xfer.bin $@

i386vm: build/i386vm/qdos386-r5.bin

# Negative test: the same image with the synthetic timer never started.
build/i386vm/kernel-notimer-raw.bin: $(I386VM_SRC) $(wildcard kernel/*.h) $(wildcard tests/*.h) platform/platform.h
	mkdir -p build/i386vm
	$(GCC) --raw -DQDOS_NO_TIMER $(INC) $(I386VM_SRC) -o $@

build/i386vm/qdos386-r5-notimer.bin: build/i386vm/kernel-notimer-raw.bin build/xfer.bin tools/make-vm-image.py
	python3 tools/make-vm-image.py build/i386vm/kernel-notimer-raw.bin build/xfer.bin $@

run-i386vm: build/i386vm/qdos386-r5.bin
	$(VM) build/i386vm/qdos386-r5.bin

# ---- Tiny386 PC -----------------------------------------------------------
PC_OBJS = build/pc/main.o build/pc/jobs.o build/pc/memory.o build/pc/channel.o build/pc/irq.o \
          build/pc/descriptors.o build/pc/test_jobs.o build/pc/test_memory.o build/pc/test_channels.o \
          build/pc/aout.o build/pc/test_app.o build/pc/context.o build/pc/platform.o

pc: build/pc/r5pc.img

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

build/pc/r5pc.img: build/pc/kernel.raw build/xfer.bin platform/pc/boot.bin platform/pc/make-image.py
	python3 platform/pc/make-image.py platform/pc/boot.bin build/pc/kernel.raw build/xfer.bin $@

platform/pc/boot.bin: platform/pc/boot.s
	cd platform/pc && $(AS) -o boot.o boot.s && $(LD) -N -T boot.ld -e start -o boot.aout boot.o && python3 $(I386VM)/tools/aout-to-raw.py boot.aout boot.bin

build/pc/run.ini: build/pc/r5pc.img
	printf '[pc]\nbios = %s/bios.bin\nvga_bios = %s/vgabios.bin\nmem_size = 8M\nhda = %s\n' \
	  "$(SEABIOS)" "$(SEABIOS)" "$(CURDIR)/build/pc/r5pc.img" > $@

run-pc: build/pc/run.ini
	cd build/pc && $(TINY386) run.ini

notimer: build/i386vm/qdos386-r5-notimer.bin

clean:
	rm -rf build platform/pc/boot.o platform/pc/boot.aout platform/pc/boot.bin

.PHONY: all apps i386vm pc run-i386vm run-pc clean notimer
