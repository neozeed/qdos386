# QDOS/386 R6 - build both targets.
#   make i386vm   : flat image for the i386VM harness: kernel, transfer area,
#                   and the WIN1_ FAT16 volume as a preloaded RAM disk
#   make pc       : Tiny386 PC disk image (SeaBIOS boot sector, transfer area);
#                   the FAT16 volume is a separate disk, given as hdb in run.ini
#   make apps     : the a.out applications and the transfer area
#   make fat16    : the WIN1_ FAT16 volume (build/fat16/win1.img, 3 MiB)
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
        kernel/aout.c kernel/fat16.c kernel/win1.c \
        tests/test_jobs.c tests/test_memory.c tests/test_channels.c tests/test_app.c tests/test_fs.c

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

# WIN1_ volume (R6): the same bytes go on the Tiny386 hdb disk and into the
# i386VM image.  HELLO.AOU is the hello application as a file; PREP.AOU is
# the R5 prep application.
build/fat16/win1.img: $(APPS) tools/make-fat16.py
	mkdir -p build/fat16
	python3 tools/make-fat16.py $@ HELLO.AOU=build/apps/hello.aout PREP.AOU=build/apps/prep.aout

fat16: build/fat16/win1.img

# Transfer area: directory plus the exact a.out bytes.  HEL2 is a second copy
# of hello (slot 0), used to test the busy-slot refusal.
build/xfer.bin: $(APPS) tools/make-xfer.py
	mkdir -p build
	python3 tools/make-xfer.py $@ HELO=build/apps/hello.aout PREP=build/apps/prep.aout HEL2=build/apps/hello.aout

# ---- i386VM ---------------------------------------------------------------
I386VM_SRC = $(KCORE) kernel/context.s platform/i386vm/platform.c platform/i386vm/disk.c platform/i386vm/halt.s

build/i386vm/kernel-raw.bin: $(I386VM_SRC) $(wildcard kernel/*.h) $(wildcard tests/*.h) platform/platform.h
	mkdir -p build/i386vm
	$(GCC) --raw $(INC) $(I386VM_SRC) -o $@

build/i386vm/qdos386-r6.bin: build/i386vm/kernel-raw.bin build/xfer.bin build/fat16/win1.img tools/make-vm-image.py
	python3 tools/make-vm-image.py build/i386vm/kernel-raw.bin build/xfer.bin $@ build/fat16/win1.img

i386vm: build/i386vm/qdos386-r6.bin

# Negative test: the same image with the synthetic timer never started.
build/i386vm/kernel-notimer-raw.bin: $(I386VM_SRC) $(wildcard kernel/*.h) $(wildcard tests/*.h) platform/platform.h
	mkdir -p build/i386vm
	$(GCC) --raw -DQDOS_NO_TIMER $(INC) $(I386VM_SRC) -o $@

build/i386vm/qdos386-r6-notimer.bin: build/i386vm/kernel-notimer-raw.bin build/xfer.bin build/fat16/win1.img tools/make-vm-image.py
	python3 tools/make-vm-image.py build/i386vm/kernel-notimer-raw.bin build/xfer.bin $@ build/fat16/win1.img

run-i386vm: build/i386vm/qdos386-r6.bin
	$(VM) build/i386vm/qdos386-r6.bin

# ---- Tiny386 PC -----------------------------------------------------------
PC_OBJS = build/pc/main.o build/pc/jobs.o build/pc/memory.o build/pc/channel.o build/pc/irq.o \
          build/pc/descriptors.o build/pc/test_jobs.o build/pc/test_memory.o build/pc/test_channels.o \
          build/pc/aout.o build/pc/test_app.o build/pc/context.o build/pc/platform.o \
          build/pc/fat16.o build/pc/win1.o build/pc/test_fs.o build/pc/disk.o

pc: build/pc/r6pc.img

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

build/pc/r6pc.img: build/pc/kernel.raw build/xfer.bin platform/pc/boot.bin platform/pc/make-image.py
	python3 platform/pc/make-image.py platform/pc/boot.bin build/pc/kernel.raw build/xfer.bin $@

platform/pc/boot.bin: platform/pc/boot.s
	cd platform/pc && $(AS) -o boot.o boot.s && $(LD) -N -T boot.ld -e start -o boot.aout boot.o && python3 $(I386VM)/tools/aout-to-raw.py boot.aout boot.bin

build/pc/run.ini: build/pc/r6pc.img build/fat16/win1.img
	printf '[pc]\nbios = %s/bios.bin\nvga_bios = %s/vgabios.bin\nmem_size = 8M\nhda = %s\nhdb = %s\n' \
	  "$(SEABIOS)" "$(SEABIOS)" "$(CURDIR)/build/pc/r6pc.img" "$(CURDIR)/build/fat16/win1.img" > $@

run-pc: build/pc/run.ini
	cd build/pc && $(TINY386) run.ini

notimer: build/i386vm/qdos386-r6-notimer.bin

clean:
	rm -rf build platform/pc/boot.o platform/pc/boot.aout platform/pc/boot.bin

.PHONY: all apps fat16 i386vm pc run-i386vm run-pc clean notimer
