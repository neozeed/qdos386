# QDOS/386 R16A — MBR data disks + CPU-aware Task Manager — HANDOVER

## What this is

Two small, localized, opt-in improvements on the frozen R16 BKL/SMP source:

1. The WIN1_ data disk (QEMU `hdb`, the PIIX3 primary slave) can now be an ordinary **MBR disk with one primary FAT16
   partition** (e.g. a Windows-prepared VHD with a FAT16 partition at LBA 2048), as well as the existing unpartitioned
   FAT16 superfloppy.
2. The **Task Manager shows a CPU column** grounded in the R16 scheduler's own per-job ownership.

Both are compiled only under `-DQDOS_R16` (the `qdos386-smp-pc` / R16A build). The ordinary `make` KCTX-FIX2 kernels
and every frozen R12–R16 target are **byte-identical** (`qdos386-vga.img` = `f8e5d5e1…`, `qdos386.img` = `7c675978…`).

## Baseline

Branch `r16a-mbr-taskcpu` from R16 tag `r16-bkl-smp-experiment` (commit `3f33646`). `platform/pc/disk.c` and
`kernel/fat16.c` are unchanged; the partition offset lives entirely in the block-volume adapter.

## Build and run

```
. scratchpad/env-r11c.sh ; export QDOS386_TOOLS
make qdos386-smp-pc                      # R16A kernel -> build/qdos386-smp/pc-vga/qdos386-smp-vga.img
sh scripts/r16-build-guest.sh            # CPL3 test program R16T (+ R16TP.aout, marked protected)
sh scripts/r16a-hosttest.sh              # host unit test of the MBR locator + FAT read (17/17)

make qdos386-pc                          # ordinary kernels, still f8e5d5e1… / 7c675978…
```

Fixtures:

```
sh scripts/r16a-mkdata.sh OUTDIR         # OUTDIR/data-mbr.img (FAT16 primary @ LBA 2048) + data-superfloppy.img
sh scripts/r16a-wrap-mbr.sh IN.img OUT.img [START]   # wrap a FAT16 superfloppy as an MBR partition (default LBA 2048)
```

Boot with an MBR-partitioned FAT16 system volume as `hdb` (`-smp 2`, MTTCG for the CPU column):

```
# wrap a normal system volume, then boot it as hdb
sh scripts/r14a-sysvol.sh sys.img                 # or qdos386-public-vol.sh for a public-safe volume
sh scripts/r16a-wrap-mbr.sh sys.img sys-mbr.img
QEMU_MEM=32 QEMU_EXTRA="-smp 2 -accel tcg,thread=multi" \
  R12_QKERNEL=build/qdos386-smp/pc-vga/qdos386-smp-vga.img \
  sh scripts/r12-run-qemu.sh sys-mbr.img SEQ OUTDIR tag 150
```

Windows workflow: create a VHD, format a **FAT16** partition in it (Windows may default to FAT32/NTFS — choose FAT16
deliberately), copy files **while QEMU is not using the image**, detach the VHD in Windows, then launch QEMU with
`-drive file=disk.vhd,format=vpc,if=ide,index=1,media=disk` (or `qemu-img convert` to raw and `format=raw`). Never
mount the same writable image in Windows and QEMU at once.

## What was verified (docs/R16A-RUNTIME-REPORT.md)

* Host unit test 17/17 (reject matrix + superfloppy + MBR mount+read), fixtures byte-identical afterwards.
* `-smp 2` boot from an MBR-partitioned FAT16 `hdb`: DIR/COPY/DEL, `R16T WHERE` at CPL3, reboot read-back persists;
  the MBR and the pre-partition gap are byte-identical after writes and the partition region fscks clean.
* Legacy unpartitioned superfloppy still boots and reads.
* Task Manager CPU column grounded (SYSTEM `0`, CPU-1 idle `1`, CPL3 jobs `~1`/`~0`), all controls intact.
* Regression: KCTX-FIX2 default byte-identical; R16 Gate 1 24/24; `R16T RACE` two CPUs, `badrel=0`.

## Source map (all additive / `-DQDOS_R16`-gated)

* New: `kernel/mbr.c`, `kernel/mbr.h` (pure FAT16 locator); `tests/host/r16a/test_mbr.c`; `scripts/r16a-mkdata.sh`,
  `scripts/r16a-wrap-mbr.sh`, `scripts/r16a-hosttest.sh`.
* Changed under `-DQDOS_R16`: `kernel/win1.c` (offset adapter + mount decision), `kernel/taskmgr.c` (CPU column),
  `kernel/jobs.c`/`jobs.h` (read-only `last_cpu`), `Makefile` (`mbr.o` in the R16 build).
* Bug fix: `kernel/smp.c` AP-idle `aff` now indexes by slot (was the job id — an out-of-bounds write).

## Documents in this package

1. **R16A-DESIGN.md** — the design and call sites (Gate 0).
2. **R16A-RUNTIME-REPORT.md** — the tests actually run and their output.
3. **R16A-KNOWN-LIMITATIONS.md** — boundaries, the `~N` semantics, and the AP-idle bug fix.
4. (reference) the R16 docs ride along in the source tree under `docs/`.

## Next steps for a future builder

FAT32 and large volumes; more than one data disk; an MBR/partition-aware boot disk; per-CPU usage accounting in the
Task Manager. None are attempted here — R16A stops at a readable/writable MBR FAT16 data disk and a grounded CPU column.
