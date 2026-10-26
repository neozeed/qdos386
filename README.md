# QDOS/386 R6 - native kernel that loads a.out programs from a FAT16 disk

R6 adds read-only FAT16 files. A WIN1_ volume is mounted from the platform disk
(Tiny386 IDE primary slave on the PC; a preloaded RAM disk on i386VM), and a job
opens `WIN1_HELLO.AOU` through the R4 channel manager, reads it with
`QT_CH_READ`, loads it with the unchanged R5 a.out loader, and runs it as an
ordinary preemptive job. The guest prints the R6 lines only after those real
file operations. R2–R5 behaviour and the R5 transfer area are kept.

Targets: the i386VM harness and the Tiny386 PC. Both read the same FAT16 bytes.
See docs/R6-ABI.md (channel ABI), docs/R6-FAT16-FORMAT.md (volume),
docs/R6-RUNTIME-REPORT.md (results and limits) and docs/R6-AUDIT.md (the audit that
preceded the design).

## R6 build and run

    make i386vm pc apps fat16 I386VM=... TINY386=... SEABIOS=...
    scripts/run-all.sh        # three runs per target, plus the timer-off negative

`build/fat16/win1.img` is the 3 MiB volume (tools/make-fat16.py). It is attached
as `hdb` for the PC and embedded at 0x2C0000 in the i386VM image. Its SHA-256
is in SHA256SUMS and must not change during a run.

## Earlier releases

# QDOS/386 R5 - native kernel that loads and runs independently linked a.out programs
## Status

Verified in the guest on both targets, three runs each (`docs/R5-RUNTIME-REPORT.md`):

- i386VM and Tiny386 PC: 103 checks ok, 1 n/a, 0 FAIL; `QDOS/386 R4 CHANNEL PASS`
  and `QDOS/386 R5 A.OUT EXEC PASS`.
- i386VM with the timer disabled: no PASS (instruction limit).

Documents:

- `docs/R5-AOUT-FORMAT.md`: the a.out dialect, from bytes (Phase 0).
- `docs/R5-APP-ABI.md`: memory map, linking, startup, exit, traps, transfer area,
  loader and application contracts.
- `docs/R5-RUNTIME-REPORT.md`: results, checks, limits, external dependency hashes.
- `docs/R4-CHANNEL-ABI.md`, `docs/R3-MEMORY-ABI.md`: earlier ABIs, unchanged.

## Layout

```
kernel/
  appmap.h               R5 memory map (slots, transfer area, heap end)
  aout.h, aout.c         transfer-area reader and OMAGIC loader
  main.c                 boot, R2-R4 phases, R5 application phase, report
  jobs.h, jobs.c         job table, scheduler, traps (incl. QT_EXIT 0x18, QT_JOB_EXIT 0x19)
  memory.h, memory.c     native heap
  channel.h, channel.c   channel manager and CON driver (R4)
  irq.h, irq.c           IRQ7 and IRQ15 classification (R4)
  context.s              IRQ stubs, yield, resume, software int helpers
  descriptors.c          GDT, IDT
tests/
  test_jobs.c/h, test_memory.c/h, test_channels.c/h   R2-R4 checks
  test_app.c/h           R5 application phase and malformed-image tests
apps/
  hello/hello.c          channel output, exit status 7
  prep/prep.c            data, BSS, stack, preemption, exit status 42
  common/app_start.s     _start (calls main, QT_EXIT, returns)
  common/qtrap.s         C wrapper for int 0x30
  common/slot0.ld        link script, origin 0x200000
  common/slot1.ld        link script, origin 0x240000
  probe-layout/          Phase 0 probe (format audit evidence)
platform/
  pc/                    boot sector (INT 13h), kernel link, image builder, console, PIC/PIT
  i386vm/                console, synthetic timer, halt
tools/
  make-xfer.py           transfer area: directory + exact a.out bytes
  make-vm-image.py       i386VM image: kernel, zero padding to 0x280000, transfer area
scripts/
  run-all.sh             build both targets and run the matrix
logs/                    R5 matrix logs (logs/*.log, logs/pc-run*.txt), fresh-extraction
                         logs (logs/fresh-extraction/), R4 and R3 logs for comparison
docs/                    see Status
```

## Build and run

External inputs are not included. Set their locations:

- `I386VM`: the i386VM harness tree (`bin/i386-gcc`, `bin/as`, `bin/i386-ld`, `bin/i386vm`).
- `TINY386`: the `tiny386_headless` binary built from the unmodified Tiny386 tree.
- `SEABIOS`: a directory with `bios.bin` and `vgabios.bin` (the known-good pair in
  `docs/R5-RUNTIME-REPORT.md` section 7).

```
I386VM=... TINY386=... SEABIOS=... scripts/run-all.sh
```

Outputs:

- `build/apps/hello.aout`, `build/apps/prep.aout`: the two applications (OMAGIC).
- `build/xfer.bin`: the transfer area.
- `build/i386vm/qdos386-r5.bin`: flat image for the i386VM harness (2.6 MB).
- `build/i386vm/qdos386-r5-notimer.bin`: negative-test image.
- `build/pc/r5pc.img`: Tiny386 hard-disk image.

## Services

Trap `int 0x30`, EAX = function, EBX/ECX/EDX = arguments, EAX = result.

| Code | Service |
|---|---|
| `0x08`–`0x0F` | R2 job and scheduler services |
| `0x10`–`0x13` | R3 memory |
| `0x14`–`0x17` | R4 channel: open, close, write, putc |
| `0x18` | R5 `QT_EXIT`: record the status of the calling job |
| `0x19` | R5 `QT_JOB_EXIT`: read a job's recorded status |

## Not in R5

- Keyboard input, files, directories, serial, VGA, SuperBASIC, ELF loading.
- Relocation, dynamic linking, shared libraries, ZMAGIC/QMAGIC.
- Protection between applications; everything runs in ring 0.
- More than two application slots, or applications larger than 256 KiB.
