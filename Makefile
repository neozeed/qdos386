# QDOS/386 R6 and R7 - build both targets.
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

# Output directories and flags.  The defaults are the R6 test images; the R7
# shell boot images are built from the same sources with other values:
#   make i386vm-shell   build/i386vm-shell/qdos386-r7-shell.bin  (-DR7_SHELL_MODE)
#   make pc-shell       build/pc-shell/r7shell.img                (-DR7_SHELL_MODE)
I386D    ?= build/i386vm
I386IMG  ?= qdos386-r6.bin
I386FLAGS ?=
PCD      ?= build/pc
PCIMG    ?= r6pc.img
PCFLAGS  ?=

INC = -Ikernel -Itests -Iplatform
KCORE = kernel/main.c kernel/jobs.c kernel/memory.c kernel/channel.c kernel/irq.c kernel/descriptors.c \
        kernel/aout.c kernel/fat16.c kernel/win1.c kernel/cin.c kernel/kbd.c kernel/imgload.c kernel/shellboot.c \
        tests/test_jobs.c tests/test_memory.c tests/test_channels.c tests/test_app.c tests/test_fs.c \
        tests/test_console.c tests/test_dir.c tests/test_shell.c

all: i386vm pc

# ---- applications (independently linked; the kernel never links them) ------
APPS = build/apps/hello.aout build/apps/prep.aout build/apps/hello1.aout build/apps/shell.aout build/apps/shellt.aout

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

# hello linked for slot 1: the same object as hello.aout, a different origin.
build/apps/hello1.aout: build/apps/hello.o build/apps/app_start.o build/apps/qtrap.o apps/common/slot1.ld
	$(LD) -N -T apps/common/slot1.ld -e _start -o $@ build/apps/app_start.o build/apps/hello.o build/apps/qtrap.o

build/fat16/fixtures/big.bin build/fat16/fixtures/bad.bin build/fat16/fixtures/empty.bin: tools/make-fixtures.py
	python3 tools/make-fixtures.py build/fat16/fixtures

# R7 shell: slot 0, the same source twice (SHELLT adds the guest self-test).
build/apps/shell.o: apps/shell/shell.c apps/common/appcalls.h
	mkdir -p build/apps
	$(GCC) -c -O -Iapps/common $< -o $@
build/apps/shellt.o: apps/shell/shell.c apps/common/appcalls.h
	mkdir -p build/apps
	$(GCC) -c -O -DSHELL_SELFTEST -Iapps/common $< -o $@
build/apps/shell.aout: build/apps/shell.o build/apps/app_start.o build/apps/qtrap.o apps/common/slot0.ld
	$(LD) -N -T apps/common/slot0.ld -e _start -o $@ build/apps/app_start.o build/apps/shell.o build/apps/qtrap.o
build/apps/shellt.aout: build/apps/shellt.o build/apps/app_start.o build/apps/qtrap.o apps/common/slot0.ld
	$(LD) -N -T apps/common/slot0.ld -e _start -o $@ build/apps/app_start.o build/apps/shellt.o build/apps/qtrap.o

apps: $(APPS) build/apps/hello1.aout build/apps/shell.aout build/apps/shellt.aout

# WIN1_ volume (R6): the same bytes go on the Tiny386 hdb disk and into the
# i386VM image.  HELLO.AOU is the hello application as a file; PREP.AOU is
# the R5 prep application.
build/fat16/win1.img: $(APPS) tools/make-fat16.py fs/README.TXT build/fat16/fixtures/big.bin build/fat16/fixtures/bad.bin build/fat16/fixtures/empty.bin
	mkdir -p build/fat16
	python3 tools/make-fat16.py $@ HELLO.AOU=build/apps/hello1.aout PREP.AOU=build/apps/prep.aout \
	  SLOT0.AOU=build/apps/hello.aout README.TXT=fs/README.TXT BIG.AOU=build/fat16/fixtures/big.bin \
	  BAD.AOU=build/fat16/fixtures/bad.bin EMPTY.TXT=build/fat16/fixtures/empty.bin \
	  SHELL.AOU=build/apps/shell.aout SHELLT.AOU=build/apps/shellt.aout

fat16: build/fat16/win1.img

# Transfer area: directory plus the exact a.out bytes.  HEL2 is a second copy
# of hello (slot 0), used to test the busy-slot refusal.
build/xfer.bin: $(APPS) tools/make-xfer.py
	mkdir -p build
	python3 tools/make-xfer.py $@ HELO=build/apps/hello.aout PREP=build/apps/prep.aout HEL2=build/apps/hello.aout

# ---- i386VM ---------------------------------------------------------------
I386VM_SRC = $(KCORE) kernel/context.s platform/i386vm/platform.c platform/i386vm/disk.c platform/i386vm/halt.s

$(I386D)/kernel-raw.bin: $(I386VM_SRC) $(wildcard kernel/*.h) $(wildcard tests/*.h) platform/platform.h
	mkdir -p $(I386D)
	$(GCC) --raw $(I386FLAGS) $(INC) $(I386VM_SRC) -o $@

$(I386D)/$(I386IMG): $(I386D)/kernel-raw.bin build/xfer.bin build/fat16/win1.img tools/make-vm-image.py
	python3 tools/make-vm-image.py $(I386D)/kernel-raw.bin build/xfer.bin $@ build/fat16/win1.img

i386vm: $(I386D)/$(I386IMG)

# Negative test: the same image with the synthetic timer never started.
build/i386vm/kernel-notimer-raw.bin: $(I386VM_SRC) $(wildcard kernel/*.h) $(wildcard tests/*.h) platform/platform.h
	mkdir -p build/i386vm
	$(GCC) --raw -DQDOS_NO_TIMER $(INC) $(I386VM_SRC) -o $@

build/i386vm/qdos386-r6-notimer.bin: build/i386vm/kernel-notimer-raw.bin build/xfer.bin build/fat16/win1.img tools/make-vm-image.py
	python3 tools/make-vm-image.py build/i386vm/kernel-notimer-raw.bin build/xfer.bin $@ build/fat16/win1.img

run-i386vm: $(I386D)/$(I386IMG)
	$(VM) $(I386D)/$(I386IMG)

# ---- Tiny386 PC -----------------------------------------------------------
PC_OBJS = $(PCD)/main.o $(PCD)/jobs.o $(PCD)/memory.o $(PCD)/channel.o $(PCD)/irq.o \
          $(PCD)/descriptors.o $(PCD)/test_jobs.o $(PCD)/test_memory.o $(PCD)/test_channels.o \
          $(PCD)/aout.o $(PCD)/test_app.o $(PCD)/context.o $(PCD)/platform.o \
          $(PCD)/fat16.o $(PCD)/win1.o $(PCD)/test_fs.o $(PCD)/disk.o \
          $(PCD)/cin.o $(PCD)/kbd.o $(PCD)/test_console.o $(PCD)/i8042.o $(PCD)/test_dir.o $(PCD)/imgload.o $(PCD)/shellboot.o $(PCD)/test_shell.o

pc: $(PCD)/$(PCIMG)

$(PCD)/%.o: kernel/%.c
	mkdir -p $(PCD)
	$(GCC) -c $(INC) $(PCFLAGS) $< -o $@
$(PCD)/%.o: tests/%.c
	mkdir -p $(PCD)
	$(GCC) -c $(INC) $(PCFLAGS) $< -o $@
$(PCD)/%.o: platform/pc/%.c
	mkdir -p $(PCD)
	$(GCC) -c $(INC) $(PCFLAGS) $< -o $@
$(PCD)/context.o: kernel/context.s
	mkdir -p $(PCD)
	$(AS) -o $@ $<
$(PCD)/entry.o: platform/pc/entry.s
	mkdir -p $(PCD)
	$(AS) -o $@ $<

$(PCD)/kernel.aout: $(PCD)/entry.o $(PC_OBJS) platform/pc/kernel.ld
	$(LD) -N -T platform/pc/kernel.ld -e kstart -o $@ $(PCD)/entry.o $(PC_OBJS)

$(PCD)/kernel.raw: $(PCD)/kernel.aout
	python3 platform/pc/extract-kernel.py $< $@ 0x100000

$(PCD)/$(PCIMG): $(PCD)/kernel.raw build/xfer.bin platform/pc/boot.bin platform/pc/make-image.py
	python3 platform/pc/make-image.py platform/pc/boot.bin $(PCD)/kernel.raw build/xfer.bin $@

platform/pc/boot.bin: platform/pc/boot.s
	cd platform/pc && $(AS) -o boot.o boot.s && $(LD) -N -T boot.ld -e start -o boot.aout boot.o && python3 $(I386VM)/tools/aout-to-raw.py boot.aout boot.bin

$(PCD)/run.ini: $(PCD)/$(PCIMG) build/fat16/win1.img
	printf '[pc]\nbios = %s/bios.bin\nvga_bios = %s/vgabios.bin\nmem_size = 8M\nhda = %s\nhdb = %s\n' \
	  "$(SEABIOS)" "$(SEABIOS)" "$(CURDIR)/$(PCD)/$(PCIMG)" "$(CURDIR)/build/fat16/win1.img" > $@

run-pc: $(PCD)/run.ini
	cd $(PCD) && $(TINY386) run.ini

notimer: build/i386vm/qdos386-r6-notimer.bin

# ---- R7 shell boot images -------------------------------------------------
i386vm-shell:
	$(MAKE) i386vm I386D=build/i386vm-shell I386FLAGS=-DR7_SHELL_MODE I386IMG=qdos386-r7-shell.bin

pc-shell:
	$(MAKE) pc PCD=build/pc-shell PCFLAGS=-DR7_SHELL_MODE PCIMG=r7shell.img

# Negative test: the shell image with the timer never started.
i386vm-shell-notimer:
	$(MAKE) i386vm I386D=build/i386vm-shell-notimer I386FLAGS="-DR7_SHELL_MODE -DQDOS_NO_TIMER" I386IMG=qdos386-r7-shell-notimer.bin

clean:
	rm -rf build platform/pc/boot.o platform/pc/boot.aout platform/pc/boot.bin

.PHONY: all apps fat16 i386vm pc run-i386vm run-pc clean notimer i386vm-shell pc-shell i386vm-shell-notimer
