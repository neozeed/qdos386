# R16A — runtime report

Platform: `qemu-system-i386 -machine pc -cpu pentium -m 32`, two CPUs with `-smp 2 -accel tcg,thread=multi`. Kernel
`build/qdos386-smp/pc-vga/qdos386-smp-vga.img` (the R16A/`-DQDOS_R16` build). Guest test program `tests/r16/r16t.c`
launched CPL3-marked. Fixtures built by `scripts/r16a-mkdata.sh` (bare data disk) and `scripts/r16a-wrap-mbr.sh`
(wrap a system volume as an MBR FAT16 partition at LBA 2048). Tests actually run are listed below; nothing longer than
a few minutes.

## Host unit test (`scripts/r16a-hosttest.sh`)

`kernel/mbr.c` + `kernel/fat16.c` compiled on the host, exercised by `tests/host/r16a/test_mbr.c`:

```
R16A MBR host test: 17 passed, 0 failed
fixtures byte-identical after the test (read-only)
```

Covered: superfloppy VBR → offset 0; MBR FAT16 type 0x06 and 0x0E at LBA 2048 → that partition; two FAT16 primaries →
lowest-LBA chosen (ncand=2); and the reject matrix — bad boot signature, unsupported type (0x07), zero-length
partition, start out of range, start+size past the device, 32-bit `start+size` overflow, blank sector 0. The fixture
images are opened read-only and are byte-identical afterwards (sha256 before == after).

## QEMU — MBR-partitioned FAT16 data disk (`-smp 2`)

System volume wrapped as a FAT16 primary (type 0x06) at LBA 2048 of an MBR disk, booted as `hdb`:

```
WIN1_: mounted
SHELL: started as a process
SHELL: running
/> DIR                      -> 76 files, 13 directories  (read from the partition)
/> R16T WHERE               -> cs=2b ss=33 cpl=3          (a program loads and runs at CPL3 from the partition)
/> COPY BIN/R16T.AOU TESTCOPY.BIN
/> DIR                      -> 77 files                   (write created the file)
/> DEL TESTCOPY.BIN
```

Post-run checks on the resulting `hdb` image:

* **MBR (sector 0) byte-identical** before and after the writes.
* **Pre-partition gap (sectors 1..2047) byte-identical** — writes never addressed anything outside the partition.
* Extracted partition region (sectors 2048..) is **`fsck.fat -n` clean** (both FAT copies consistent).

## QEMU — reboot and read back (persistence)

Run A created `KEEPME.BIN` on the MBR partition and did not delete it; Run B booted the image left by Run A:

```
Run B /> DIR -> KEEPME.BIN    36435
```

The file written through the partition offset survived a reboot and a fresh mount; the partition region fscks clean.

## QEMU — legacy superfloppy (`-smp 2`)

An ordinary unpartitioned FAT16 superfloppy system volume boots unchanged: `WIN1_: mounted`, `SHELL: running`, `DIR`
works, `ECHO legacy-superfloppy-ok` responds. The superfloppy path is byte-for-byte the prior behaviour.

## QEMU — Task Manager CPU column (`-smp 2`)

Two CPL3 jobs started on separate CPUs (`START R16T PIN 1 … /NEW`, `START R16T PIN 0 … /NEW`), then Ctrl+Esc:

```
 ID       TITLE                    CPU PROGRAM        STATE   CON  PARENT
 0        SYSTEM                   0   SYSTEM         RUN     0    -        K
 65537    SHELL.AOU                ~0  SHELL.AOU      INPUT   0    -        P
 65538    IDLE                     ~0  IDLE           RUN     0    -        K
 65539    IDL1                     1   IDL1           RUN     0    -        K
 65540    R16T.AOU                 ~1  R16T.AOU       RUN     4    65537
 65541    R16T.AOU                 ~0  R16T.AOU       RUN     5    65537
```

`SYSTEM` runs on CPU 0 (`0`); the CPU-1 idle `IDL1` is on CPU 1 (`1`); the job pinned to CPU 1 shows `~1` and the one
pinned to CPU 0 shows `~0` (the snapshot runs in SYSTASK on CPU 0, so whichever CPL3 job was on CPU 0 is momentarily
displaced and shows its last CPU; the CPU-1 job's ownership is observed live). Running jobs with confirmed ownership
show `0`/`1`; others show `~0`/`~1` (last CPU) or `-` (never dispatched). All other columns and the F1/F2/Enter/Esc
controls are intact; switching and ending tasks still work. `R16T RACE` likewise shows its two children at `on_cpu=1`
and `on_cpu=0`, matching the IDs the jobs report themselves.

## Regression / identity

* `make qdos386-pc` → `f8e5d5e1…` / `7c675978…` — the KCTX-FIX2 bytes, unchanged (all R16A code is under `-DQDOS_R16`).
* R16 Gate 1 (`-smp 1`) on the R16A kernel: **24/24**. `R16T RACE` (`-smp 2`): two CPL3 apps concurrent, `badrel=0`.
* A bug the CPU column exposed — `smp.c` wrote `jobs[<job id>].aff` instead of `jobs[<slot>].aff`, an out-of-bounds
  write that left the CPU-1 idle at `aff=0` — was fixed (index by slot). After the fix the idle shows CPU 1 correctly
  and RACE/Gate 1 still pass.
