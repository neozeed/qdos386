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
# R8: the FAT16 volume each boot image uses (R7 volume by default).
I386WIN  ?= build/fat16/win1.img
PCWIN    ?= build/fat16/win1.img

INC = -Ikernel -Itests -Iplatform
KCORE = kernel/region.c kernel/main.c kernel/jobs.c kernel/clock.c kernel/clock_boot.c kernel/fpu.c kernel/memory.c kernel/channel.c kernel/irq.c kernel/descriptors.c \
        kernel/aout.c kernel/fat16.c kernel/win1.c kernel/cin.c kernel/kbd.c kernel/imgload.c kernel/shellboot.c \
        tests/test_jobs.c tests/test_memory.c tests/test_channels.c tests/test_app.c tests/test_fs.c \
        tests/test_console.c tests/test_dir.c tests/test_shell.c tests/test_r8.c

all: i386vm pc

# ---- applications (independently linked; the kernel never links them) ------
APPS = build/apps/hello.aout build/apps/prep.aout build/apps/hello1.aout build/apps/shell.aout build/apps/shellt.aout build/apps/args.aout

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
# R11L: the same shell with a 160 character input line (the ported tools take long command lines).
build/apps/shell-l.o: apps/shell/shell.c apps/common/appcalls.h
	mkdir -p build/apps
	$(GCC) -c -O -DSH_LINE_MAX=160 -Iapps/common $< -o $@
build/apps/shell-l.aout: build/apps/shell-l.o build/apps/app_start.o build/apps/qtrap.o apps/common/slot0.ld
	$(LD) -N -T apps/common/slot0.ld -e _start -o $@ build/apps/app_start.o build/apps/shell-l.o build/apps/qtrap.o
# R11M: the directory-aware shell (160 character line; -DR11M_SHELL adds PWD CD MD RD COPY DEL REN, and START PS WAIT KILL CON with -DR11M_JOBS).
build/apps/shell-m.o: apps/shell/shell.c apps/shell/shell_r11m.inc apps/shell/shell_jobs.inc apps/shell/shell_edit.inc apps/common/appcalls.h
	mkdir -p build/apps
	$(GCC) -c -O -DSH_LINE_MAX=160 -DR11M_SHELL -DR11M_JOBS -Iapps/common -Iapps/shell $< -o $@
build/apps/shell-m.aout: build/apps/shell-m.o build/apps/app_start.o build/apps/qtrap.o apps/common/slot0.ld
	$(LD) -N -T apps/common/slot0.ld -e _start -o $@ build/apps/app_start.o build/apps/shell-m.o build/apps/qtrap.o
build/apps/shellt.aout: build/apps/shellt.o build/apps/app_start.o build/apps/qtrap.o apps/common/slot0.ld
	$(LD) -N -T apps/common/slot0.ld -e _start -o $@ build/apps/app_start.o build/apps/shellt.o build/apps/qtrap.o

# R8: ARGS.AOU, slot 1, prints and validates argc/argv (apps/args/args.c).
build/apps/args.o: apps/args/args.c apps/common/appcalls.h
	mkdir -p build/apps
	$(GCC) -c -O -Iapps/common $< -o $@
build/apps/args.aout: build/apps/args.o build/apps/app_start.o build/apps/qtrap.o apps/common/slot1.ld
	$(LD) -N -T apps/common/slot1.ld -e _start -o $@ build/apps/app_start.o build/apps/args.o build/apps/qtrap.o

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
# R11J/R11K: I386EXTRA adds the paging sources (empty for every default image).
I386EXTRA ?=
I386VM_SRC = $(KCORE) kernel/context.s platform/i386vm/platform.c platform/i386vm/disk.c platform/i386vm/halt.s $(I386EXTRA)

$(I386D)/kernel-raw.bin: $(I386VM_SRC) $(wildcard kernel/*.h) $(wildcard tests/*.h) platform/platform.h
	mkdir -p $(I386D)
	$(GCC) --raw $(I386FLAGS) $(INC) $(I386VM_SRC) -o $@

# IMGENV selects the image layout for make-vm-image.py (R9: QDOS_LAYOUT=r9).
IMGENV ?=
$(I386D)/$(I386IMG): $(I386D)/kernel-raw.bin build/xfer.bin $(I386WIN) tools/make-vm-image.py
	$(IMGENV) python3 tools/make-vm-image.py $(I386D)/kernel-raw.bin build/xfer.bin $@ $(I386WIN)

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
PC_OBJS = $(PCD)/main.o $(PCD)/region.o $(PCD)/jobs.o $(PCD)/clock.o $(PCD)/clock_boot.o $(PCD)/fpu.o $(PCD)/memory.o $(PCD)/channel.o $(PCD)/irq.o \
          $(PCD)/descriptors.o $(PCD)/test_jobs.o $(PCD)/test_memory.o $(PCD)/test_channels.o \
          $(PCD)/aout.o $(PCD)/test_app.o $(PCD)/context.o $(PCD)/platform.o \
          $(PCD)/fat16.o $(PCD)/win1.o $(PCD)/test_fs.o $(PCD)/disk.o \
          $(PCD)/cin.o $(PCD)/kbd.o $(PCD)/vga.o $(PCD)/test_console.o $(PCD)/test_r8.o $(PCD)/i8042.o $(PCD)/test_dir.o $(PCD)/imgload.o $(PCD)/shellboot.o $(PCD)/test_shell.o

# R11J/R11K: PCEXTRA adds the paging objects (empty for every default image).
PCEXTRA ?=
PC_OBJS += $(PCEXTRA)

pc: $(PCD)/$(PCIMG)

# Every object depends on every header: the R8 change to a shared struct
# (struct job) must rebuild all users, or the Tiny386 image runs stale layouts.
PC_HDR = $(wildcard kernel/*.h) $(wildcard tests/*.h) $(wildcard platform/*.h) $(wildcard platform/pc/*.h)

# The compiler flags are part of the build identity: a change to PCFLAGS for an
# existing PCD must rebuild every object, not reuse objects built with other
# defines.  The stamp is rewritten only when its content changes.
FORCE:
$(PCD)/.pcflags: FORCE
	@mkdir -p $(PCD)
	@printf '%s\n' '$(PCFLAGS)' > $@.new; if cmp -s $@.new $@; then rm -f $@.new; else mv $@.new $@; fi
$(PCD)/%.o: kernel/%.c $(PC_HDR) $(PCD)/.pcflags
	mkdir -p $(PCD)
	$(GCC) -c $(INC) $(PCFLAGS) $< -o $@
$(PCD)/%.o: tests/%.c $(PC_HDR) $(PCD)/.pcflags
	mkdir -p $(PCD)
	$(GCC) -c $(INC) $(PCFLAGS) $< -o $@
$(PCD)/%.o: platform/pc/%.c $(PC_HDR) $(PCD)/.pcflags
	mkdir -p $(PCD)
	$(GCC) -c $(INC) $(PCFLAGS) $< -o $@
$(PCD)/context.o: kernel/context.s
	mkdir -p $(PCD)
	$(AS) -o $@ $<
$(PCD)/paging_s.o: kernel/paging.s
	mkdir -p $(PCD)
	$(AS) -o $@ $<
$(PCD)/entry.o: platform/pc/entry.s
	mkdir -p $(PCD)
	$(AS) -o $@ $<

$(PCD)/kernel.aout: $(PCD)/entry.o $(PC_OBJS) platform/pc/kernel.ld
	$(LD) -N -T platform/pc/kernel.ld -e kstart -o $@ $(PCD)/entry.o $(PC_OBJS)

$(PCD)/kernel.raw: $(PCD)/kernel.aout
	python3 platform/pc/extract-kernel.py $< $@ 0x100000

# PCBOOT is the boot sector: the R5-R8 one, or the R9 copy whose transfer
# address is 0x340000 (see the R9 section).
PCBOOT ?= platform/pc/boot.bin
# R11K: PCMKIMG is the image builder that matches PCBOOT.  The default pair (boot.bin + make-image.py)
# is the original loader and gives byte-identical images; the multi-transfer loader is the pair
# build/pc-r9/boot2.bin + platform/pc/make-image2.py (see the R11K section).
PCMKIMG ?= platform/pc/make-image.py
$(PCD)/$(PCIMG): $(PCD)/kernel.raw build/xfer.bin $(PCBOOT) $(PCMKIMG)
	python3 $(PCMKIMG) $(PCBOOT) $(PCD)/kernel.raw build/xfer.bin $@

platform/pc/boot.bin: platform/pc/boot.s
	cd platform/pc && $(AS) -o boot.o boot.s && $(LD) -N -T boot.ld -e start -o boot.aout boot.o && python3 $(I386VM)/tools/aout-to-raw.py boot.aout boot.bin

$(PCD)/run.ini: $(PCD)/$(PCIMG) $(PCWIN)
	printf '[pc]\nbios = %s/bios.bin\nvga_bios = %s/vgabios.bin\nmem_size = 8M\nhda = %s\nhdb = %s\n\n[cpu]\nfpu = 1\n' \
	  "$(SEABIOS)" "$(SEABIOS)" "$(CURDIR)/$(PCD)/$(PCIMG)" "$(CURDIR)/$(PCWIN)" > $@

# Canonical R11H i386VM images (release and negative-test).  Requires I386VM=<tree>.
r11h-i386vm:
	$(MAKE) i386vm I386D=build/i386vm-r11h-rel I386FLAGS="$(R9_FLAGS)" I386IMG=qdos386-r11h-rel.bin I386WIN=build/fat16/win1-r11h.img IMGENV="QDOS_LAYOUT=r9"
	$(MAKE) i386vm I386D=build/i386vm-r11h-neg I386FLAGS="$(R9_FLAGS)" I386IMG=qdos386-r11h-neg.bin I386WIN=build/fat16/win1-r11h-neg.img IMGENV="QDOS_LAYOUT=r9"

# R11I: kernel-only i386VM image for the host-backed block device (patches/i386vm-block.patch).
# No FAT16 payload is appended (I386WIN empty); the volume is passed to the harness at launch.
# The legacy RAM-disk targets above are unchanged.  Requires I386VM=<tree>.
r11i-i386vm-block:
	$(MAKE) i386vm I386D=build/r11i/block I386FLAGS="$(R9_FLAGS) -DQDOS_I386VM_BLOCK" I386IMG=qdos386-r11i-block.bin I386WIN= IMGENV="QDOS_LAYOUT=r9"

# R11K: opt-in paging (docs/R11K-PAGING-APP-ABI.md; R11J's POC flag QDOS_PAGING_POC is now QDOS_PAGING, ABI 2).
# Default images are unchanged: the paging sources enter only through I386EXTRA / PCEXTRA here.
# The multi-transfer boot sector is selected with PCBOOT=build/pc-r9/boot2.bin PCMKIMG=platform/pc/make-image2.py
# (docs/R11K-BOOT-ABI.md); the default pair is the original boot.bin + make-image.py.
R11K_PAGING = -DQDOS_PAGING
r11k-i386vm-paging:
	$(MAKE) i386vm I386D=build/r11k/vm-paging I386FLAGS="$(R9_FLAGS) -DQDOS_I386VM_BLOCK $(R11K_PAGING)" I386IMG=qdos386-r11k-paging.bin I386WIN= IMGENV="QDOS_LAYOUT=r9" I386EXTRA="kernel/paging.c kernel/paging.s"
r11k-pc-paging:
	$(MAKE) build/pc-r9/boot2.bin
	$(MAKE) pc PCD=build/r11k/pc-paging PCFLAGS="-DQDOS_LAYOUT_R9 -DR7_SHELL_MODE $(R11K_PAGING)" PCIMG=r11k-paging.img PCBOOT=build/pc-r9/boot2.bin PCMKIMG=platform/pc/make-image2.py PCEXTRA="build/r11k/pc-paging/paging.o build/r11k/pc-paging/paging_s.o"
	$(MAKE) pc PCD=build/r11k/pc-paging-vga PCFLAGS="-DQDOS_CON_VGA -DQDOS_LAYOUT_R9 -DR7_SHELL_MODE $(R11K_PAGING)" PCIMG=r11k-paging-vga.img PCBOOT=build/pc-r9/boot2.bin PCMKIMG=platform/pc/make-image2.py PCEXTRA="build/r11k/pc-paging-vga/paging.o build/r11k/pc-paging-vga/paging_s.o"

# R11L: opt-in writable FAT16 (-DQDOS_FS_WRITE; docs/R11L-FAT-WRITE-ABI.md).  Separate output
# directories; no default or R11K image is rebuilt by these targets.
#   r11l-i386vm-fs      i386VM block mode, writable volume (harness i386vm-r9-blockrw ... rw)
#   r11l-i386vm-fs-pg   the same with R11K paging
#   r11l-pc-fs / -vga   Tiny386 / QEMU PC with the R11K boot sector, writable volume
# R11L kernels: writable volume + 64 KiB application stacks (cccp alloca()s whole include files, cc1 recurses; the
# default 16 KiB stack is kept for every other image)
R11L_FS = -DQDOS_FS_WRITE -DQDOS_APP_STACK=65536u
r11l-i386vm-fs:
	$(MAKE) i386vm I386D=build/r11l/vm-fs I386FLAGS="$(R9_FLAGS) -DQDOS_I386VM_BLOCK $(R11L_FS)" I386IMG=qdos386-r11l-fs.bin I386WIN= IMGENV="QDOS_LAYOUT=r9"
r11l-i386vm-fs-pg:
	$(MAKE) i386vm I386D=build/r11l/vm-fs-pg I386FLAGS="$(R9_FLAGS) -DQDOS_I386VM_BLOCK $(R11L_FS) $(R11K_PAGING)" I386IMG=qdos386-r11l-fs-pg.bin I386WIN= IMGENV="QDOS_LAYOUT=r9" I386EXTRA="kernel/paging.c kernel/paging.s"
r11l-pc-fs:
	$(MAKE) build/pc-r9/boot2.bin
	$(MAKE) pc PCD=build/r11l/pc-fs PCFLAGS="-DQDOS_LAYOUT_R9 -DR7_SHELL_MODE $(R11L_FS)" PCIMG=r11l-fs.img PCBOOT=build/pc-r9/boot2.bin PCMKIMG=platform/pc/make-image2.py
	$(MAKE) pc PCD=build/r11l/pc-fs-vga PCFLAGS="-DQDOS_CON_VGA -DQDOS_LAYOUT_R9 -DR7_SHELL_MODE $(R11L_FS)" PCIMG=r11l-fs-vga.img PCBOOT=build/pc-r9/boot2.bin PCMKIMG=platform/pc/make-image2.py
r11l-pc-fs-pg:
	$(MAKE) build/pc-r9/boot2.bin
	$(MAKE) pc PCD=build/r11l/pc-fs-pg PCFLAGS="-DQDOS_LAYOUT_R9 -DR7_SHELL_MODE $(R11L_FS) $(R11K_PAGING)" PCIMG=r11l-fs-pg.img PCBOOT=build/pc-r9/boot2.bin PCMKIMG=platform/pc/make-image2.py PCEXTRA="build/r11l/pc-fs-pg/paging.o build/r11l/pc-fs-pg/paging_s.o"
	$(MAKE) pc PCD=build/r11l/pc-fs-pg-vga PCFLAGS="-DQDOS_CON_VGA -DQDOS_LAYOUT_R9 -DR7_SHELL_MODE $(R11L_FS) $(R11K_PAGING)" PCIMG=r11l-fs-pg-vga.img PCBOOT=build/pc-r9/boot2.bin PCMKIMG=platform/pc/make-image2.py PCEXTRA="build/r11l/pc-fs-pg-vga/paging.o build/r11l/pc-fs-pg-vga/paging_s.o"

# ---- R11M: usable-OS kernels --------------------------------------------------------------------------------
# Writable volume with directories (R11L option, R11M path layer) plus the virtual consoles (-DQDOS_VCON) and,
# from Stage 3, job control.  None of these macros is set for any earlier image, so those stay byte-identical
# (scripts/r11l-default-check.sh).
#   r11m-i386vm / -pg   i386VM harness kernels without / with R11K paging
#   r11m-pc / -pg       Tiny386 PC images (no VGA) and QEMU PC images with the VGA text console (-vga)
R11M_OPTS = $(R11L_FS) -DQDOS_VCON -DQDOS_JOBCTL -DQDOS_VFAT
r11m-i386vm:
	$(MAKE) i386vm I386D=build/r11m/vm I386FLAGS="$(R9_FLAGS) -DQDOS_I386VM_BLOCK $(R11M_OPTS)" I386IMG=qdos386-r11m.bin I386WIN= IMGENV="QDOS_LAYOUT=r9" I386EXTRA="kernel/vcon.c kernel/jobctl.c"
r11m-i386vm-pg:
	$(MAKE) i386vm I386D=build/r11m/vm-pg I386FLAGS="$(R9_FLAGS) -DQDOS_I386VM_BLOCK $(R11M_OPTS) $(R11K_PAGING)" I386IMG=qdos386-r11m-pg.bin I386WIN= IMGENV="QDOS_LAYOUT=r9" I386EXTRA="kernel/paging.c kernel/paging.s kernel/vcon.c kernel/jobctl.c"
r11m-pc:
	$(MAKE) build/pc-r9/boot2.bin
	$(MAKE) pc PCD=build/r11m/pc PCFLAGS="-DQDOS_LAYOUT_R9 -DR7_SHELL_MODE $(R11M_OPTS)" PCIMG=r11m.img PCBOOT=build/pc-r9/boot2.bin PCMKIMG=platform/pc/make-image2.py PCEXTRA="build/r11m/pc/vcon.o build/r11m/pc/jobctl.o"
	$(MAKE) pc PCD=build/r11m/pc-vga PCFLAGS="-DQDOS_CON_VGA -DQDOS_LAYOUT_R9 -DR7_SHELL_MODE $(R11M_OPTS)" PCIMG=r11m-vga.img PCBOOT=build/pc-r9/boot2.bin PCMKIMG=platform/pc/make-image2.py PCEXTRA="build/r11m/pc-vga/vcon.o build/r11m/pc-vga/jobctl.o"
r11m-pc-pg:
	$(MAKE) build/pc-r9/boot2.bin
	$(MAKE) pc PCD=build/r11m/pc-pg PCFLAGS="-DQDOS_LAYOUT_R9 -DR7_SHELL_MODE $(R11M_OPTS) $(R11K_PAGING)" PCIMG=r11m-pg.img PCBOOT=build/pc-r9/boot2.bin PCMKIMG=platform/pc/make-image2.py PCEXTRA="build/r11m/pc-pg/paging.o build/r11m/pc-pg/paging_s.o build/r11m/pc-pg/vcon.o build/r11m/pc-pg/jobctl.o"
	$(MAKE) pc PCD=build/r11m/pc-pg-vga PCFLAGS="-DQDOS_CON_VGA -DQDOS_LAYOUT_R9 -DR7_SHELL_MODE $(R11M_OPTS) $(R11K_PAGING)" PCIMG=r11m-pg-vga.img PCBOOT=build/pc-r9/boot2.bin PCMKIMG=platform/pc/make-image2.py PCEXTRA="build/r11m/pc-pg-vga/paging.o build/r11m/pc-pg-vga/paging_s.o build/r11m/pc-pg-vga/vcon.o build/r11m/pc-pg-vga/jobctl.o"
# R12: the PC kernels of the interactive stretch (QEMU VGA and Tiny386): the R11M options plus QDOS_PG_WIN3 (three paged windows).
R12_OPTS = $(R11M_OPTS) -DQDOS_PG_WIN3
r12-pc-pg:
	$(MAKE) build/pc-r9/boot2.bin
	$(MAKE) pc PCD=build/r12/pc-pg PCFLAGS="-DQDOS_LAYOUT_R9 -DR7_SHELL_MODE $(R12_OPTS) $(R11K_PAGING)" PCIMG=r12-pg.img PCBOOT=build/pc-r9/boot2.bin PCMKIMG=platform/pc/make-image2.py PCEXTRA="build/r12/pc-pg/paging.o build/r12/pc-pg/paging_s.o build/r12/pc-pg/vcon.o build/r12/pc-pg/jobctl.o"
	$(MAKE) pc PCD=build/r12/pc-pg-vga PCFLAGS="-DQDOS_CON_VGA -DQDOS_LAYOUT_R9 -DR7_SHELL_MODE $(R12_OPTS) $(R11K_PAGING)" PCIMG=r12-pg-vga.img PCBOOT=build/pc-r9/boot2.bin PCMKIMG=platform/pc/make-image2.py PCEXTRA="build/r12/pc-pg-vga/paging.o build/r12/pc-pg-vga/paging_s.o build/r12/pc-pg-vga/vcon.o build/r12/pc-pg-vga/jobctl.o"
r11m-kernels: r11m-i386vm r11m-i386vm-pg r11m-pc r11m-pc-pg

# Canonical R11H PC images (the shipped set).  Requires I386VM=<tree>.  The R9 boot
# sector (transfer address 0x340000) is built by its own rule.
r11h-pc:
	$(MAKE) build/pc-r9/boot.bin
	$(MAKE) pc PCD=build/pc-r11h PCFLAGS="-DQDOS_LAYOUT_R9 -DR7_SHELL_MODE" PCIMG=r11h-shell.img PCBOOT=build/pc-r9/boot.bin
	$(MAKE) pc PCD=build/pc-r11h-vga PCFLAGS="-DQDOS_CON_VGA -DQDOS_LAYOUT_R9 -DR7_SHELL_MODE -DQDOS_CLOCK_EPOCH=1793102400 -DQDOS_CLOCK_PINNED" PCIMG=r9shell-vga.img PCBOOT=build/pc-r9/boot.bin

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

.PHONY: all apps fat16 i386vm pc r11h-pc r11h-i386vm run-i386vm run-pc clean notimer i386vm-shell pc-shell i386vm-shell-notimer i386vm-r8-shell pc-r8-shell r8-apps r8-fat16

# ---- R8: large applications, ARGS.AOU and the R8 volume -------------------
# LARGE.AOU is generated (tools/make-large.py): about 175 KiB of text, data and
# BSS, in slot 1, with argv and heap, timer and table checks.  Its expected
# dispatcher value is written to build/apps/large-expected.txt.
build/apps/large.c build/apps/large-expected.txt: tools/make-large.py
	mkdir -p build/apps
	python3 tools/make-large.py build/apps/large.c build/apps/large-expected.txt
build/apps/large.o: build/apps/large.c apps/common/appcalls.h
	$(GCC) -c -O -Iapps/common $< -o $@
build/apps/large.aout: build/apps/large.o build/apps/app_start.o build/apps/qtrap.o apps/common/slot1.ld
	$(LD) -N -T apps/common/slot1.ld -e _start -o $@ build/apps/app_start.o build/apps/large.o build/apps/qtrap.o

# Negative fixtures derived from LARGE.AOU (read-only; see the script).
build/fat16/r8fix/OVER.AOU: build/apps/large.aout tools/make-r8-fixtures.py
	python3 tools/make-r8-fixtures.py build/apps/large.aout build/fat16/r8fix
build/fat16/r8fix/TRUNC.AOU build/fat16/r8fix/LONG.AOU build/fat16/r8fix/ENTRY.AOU build/fat16/r8fix/MAGIC.AOU: build/fat16/r8fix/OVER.AOU

r8-apps: build/apps/args.aout build/apps/large.aout build/apps/shell.aout build/apps/hello1.aout build/fat16/r8fix/OVER.AOU

# The R8 volume (separate from the R7 volume, which keeps its 10-file listing).
# The SDK examples (sdk/examples) are built with qdos386-gcc from sdk/.
sdk-examples:
	$(MAKE) -C sdk/lib QDOS386_TOOLS=$(I386VM)
	$(MAKE) -C sdk/libc QDOS386_TOOLS=$(I386VM)
	$(MAKE) -C sdk/examples QDOS386_TOOLS=$(I386VM)

build/fat16/win1-r8.img: r8-apps sdk-examples tools/make-fat16.py fs/README.TXT build/fat16/fixtures/big.bin build/fat16/fixtures/bad.bin build/fat16/fixtures/empty.bin
	mkdir -p build/fat16
	python3 tools/make-fat16.py $@ SHELL.AOU=build/apps/shell.aout ARGS.AOU=build/apps/args.aout \
	  LARGE.AOU=build/apps/large.aout HELLO.AOU=build/apps/hello1.aout README.TXT=fs/README.TXT \
	  SDKHEL.AOU=sdk/examples/SDKHEL.aout SDKARG.AOU=sdk/examples/SDKARG.aout \
	  SDKSTM.AOU=sdk/examples/SDKSTM.aout SDKSTS.AOU=sdk/examples/SDKSTS.aout \
	  BIG.AOU=build/fat16/fixtures/big.bin BAD.AOU=build/fat16/fixtures/bad.bin \
	  EMPTY.TXT=build/fat16/fixtures/empty.bin OVER.AOU=build/fat16/r8fix/OVER.AOU \
	  TRUNC.AOU=build/fat16/r8fix/TRUNC.AOU LONG.AOU=build/fat16/r8fix/LONG.AOU \
	  ENTRY.AOU=build/fat16/r8fix/ENTRY.AOU MAGIC.AOU=build/fat16/r8fix/MAGIC.AOU
r8-fat16: build/fat16/win1-r8.img

# R8 boot images: the R7 shell mode on the R8 volume.
i386vm-r8-shell:
	$(MAKE) i386vm I386D=build/i386vm-r8 I386FLAGS=-DR7_SHELL_MODE I386IMG=qdos386-r8-shell.bin I386WIN=build/fat16/win1-r8.img
# Negative test: the R8 shell image with the timer never started.
i386vm-r8-shell-notimer:
	$(MAKE) i386vm I386D=build/i386vm-r8-notimer I386FLAGS="-DR7_SHELL_MODE -DQDOS_NO_TIMER" I386IMG=qdos386-r8-shell-notimer.bin I386WIN=build/fat16/win1-r8.img
pc-r8-shell: build/fat16/win1-r8.img
	$(MAKE) pc PCD=build/pc-r8 PCFLAGS=-DR7_SHELL_MODE PCIMG=r8shell.img PCWIN=build/fat16/win1-r8.img

# ---- R9: 16 MiB i386VM guest, 1 MiB child slot (kernel/appmap.h) ----------
# Build with QDOS_LAYOUT_R9 in every kernel object and with QDOS_LAYOUT=r9 for
# the image builder.  The R8 targets above are unchanged and still build the
# R8 map.  The i386VM run needs the R9 harness (x/i386vm-r9, 16 MiB), which is
# not the 8 MiB harness; see patches/i386vm-r9-ram16.patch.
R9_FLAGS = -DR7_SHELL_MODE -DQDOS_LAYOUT_R9
I386VM_R9 ?= build/i386vm-r9/i386vm-r9

r9-i386vm-shell: build/fat16/win1-r9.img
	$(MAKE) i386vm I386D=build/i386vm-r9 I386FLAGS="$(R9_FLAGS)" I386IMG=qdos386-r9-shell.bin I386WIN=build/fat16/win1-r9.img IMGENV="QDOS_LAYOUT=r9"

r9-i386vm-shell-notimer:
	$(MAKE) i386vm I386D=build/i386vm-r9-notimer I386FLAGS="$(R9_FLAGS) -DQDOS_NO_TIMER" I386IMG=qdos386-r9-shell-notimer.bin I386WIN=build/fat16/win1-r9.img IMGENV="QDOS_LAYOUT=r9"

# The PC boot sector with the R9 transfer address.
build/pc-r9/boot.bin: platform/pc/boot.s
	mkdir -p build/pc-r9
	sed 's/movl $$0x280000,%edi/movl $$0x340000,%edi/' platform/pc/boot.s > build/pc-r9/boot.s
	$(AS) -o build/pc-r9/boot.o build/pc-r9/boot.s
	$(LD) -N -T platform/pc/boot.ld -e start -o build/pc-r9/boot.aout build/pc-r9/boot.o
	python3 $(I386VM)/tools/aout-to-raw.py build/pc-r9/boot.aout build/pc-r9/boot.bin

# R11K multi-transfer boot sector with the R9 transfer address (PCBOOT=build/pc-r9/boot2.bin
# PCMKIMG=platform/pc/make-image2.py selects it; the original loader stays the default).
build/pc-r9/boot2.bin: platform/pc/boot2.s
	mkdir -p build/pc-r9
	sed 's/movl $$0x280000,%edi/movl $$0x340000,%edi/' platform/pc/boot2.s > build/pc-r9/boot2.s
	$(AS) -o build/pc-r9/boot2.o build/pc-r9/boot2.s
	$(LD) -N -T platform/pc/boot.ld -e start -o build/pc-r9/boot2.aout build/pc-r9/boot2.o
	python3 $(I386VM)/tools/aout-to-raw.py build/pc-r9/boot2.aout build/pc-r9/boot2.bin

r9-pc-shell: build/pc-r9/boot.bin build/fat16/win1-r9.img
	$(MAKE) pc PCD=build/pc-r9 PCFLAGS="$(R9_FLAGS)" PCIMG=r9shell.img PCBOOT=build/pc-r9/boot.bin PCWIN=build/fat16/win1-r9.img

# R9 negative fixtures: OVER.AOU has BSS 1 MiB (over the slot-1 limit).
build/fat16/r9fix/OVER.AOU: build/apps/large.aout tools/make-r8-fixtures.py
	python3 tools/make-r8-fixtures.py build/apps/large.aout build/fat16/r9fix 0x100000
build/fat16/r9fix/TRUNC.AOU build/fat16/r9fix/LONG.AOU build/fat16/r9fix/ENTRY.AOU build/fat16/r9fix/MAGIC.AOU: build/fat16/r9fix/OVER.AOU

r9-apps: build/apps/args.aout build/apps/large.aout build/apps/shell.aout build/apps/hello1.aout build/fat16/r9fix/OVER.AOU

# The R9 volume: the R8 files, the R9 OVER fixture, the SDK examples and the
# C library proofs.  Its names stay 8.3-safe (10 characters with the dot).
r9-fat16: build/fat16/win1-r9.img

build/fat16/win1-r9.img: r9-apps sdk-examples tools/make-fat16.py fs/README.TXT build/fat16/fixtures/big.bin build/fat16/fixtures/bad.bin build/fat16/fixtures/empty.bin build/fat16/r9fix/TRUNC.AOU build/fat16/r9fix/LONG.AOU build/fat16/r9fix/ENTRY.AOU build/fat16/r9fix/MAGIC.AOU
	mkdir -p build/fat16
	python3 tools/make-fat16.py $@ SHELL.AOU=build/apps/shell.aout ARGS.AOU=build/apps/args.aout \
	  LARGE.AOU=build/apps/large.aout HELLO.AOU=build/apps/hello1.aout README.TXT=fs/README.TXT \
	  SDKHEL.AOU=sdk/examples/SDKHEL.aout SDKARG.AOU=sdk/examples/SDKARG.aout \
	  SDKSTM.AOU=sdk/examples/SDKSTM.aout SDKSTS.AOU=sdk/examples/SDKSTS.aout \
	  LIBHEL.AOU=sdk/examples/LIBHEL.aout CATFIL.AOU=sdk/examples/CATFIL.aout \
	  MEMSTR.AOU=sdk/examples/MEMSTR.aout BIGAPP.AOU=sdk/examples/BIGAPP.aout \
	HEAPEX.AOU=sdk/examples/HEAPEX.aout \
	  BIG.AOU=build/fat16/fixtures/big.bin BAD.AOU=build/fat16/fixtures/bad.bin \
	  EMPTY.TXT=build/fat16/fixtures/empty.bin OVER.AOU=build/fat16/r9fix/OVER.AOU \
	  TRUNC.AOU=build/fat16/r9fix/TRUNC.AOU LONG.AOU=build/fat16/r9fix/LONG.AOU \
	  ENTRY.AOU=build/fat16/r9fix/ENTRY.AOU MAGIC.AOU=build/fat16/r9fix/MAGIC.AOU

