# QDOS/386 R4 - native kernel with a managed heap and a channel manager

A native 32-bit i386 kernel, freestanding, flat address space, CPL 0, paging
off. R4 adds a native channel manager with a console driver, and a corrected
vector 0x2F (IRQ15) handler, on top of the R3 heap and job model. The R2
scheduler and the R3 memory services are kept and still pass.

Targets: the i386VM harness (synthetic timer) and the full Tiny386 PC
(SeaBIOS, PIT/PIC). Both run the same kernel sources.

## Status

Verified in the guest on both targets, three runs each:

- i386VM: `QDOS/386 R4 CHANNEL PASS`, 54 checks ok, 1 n/a.
- Tiny386 PC, known-good BIOS: `QDOS/386 R4 CHANNEL PASS`, 54 checks ok, 1 n/a.
- i386VM with the timer disabled: no PASS (instruction limit exceeded).

Details, limits and exact checks: `docs/R4-RUNTIME-REPORT.md`.
Channel ABI: `docs/R4-CHANNEL-ABI.md`. JS ROM channel audit:
`docs/JS-ROM-CHANNEL-MAP.md`. Interrupt and firmware baseline:
`docs/R3-FW-VALIDATION.md`.

## Layout

```
kernel/
  main.c                 boot, heap-only tests, R2 sequence, R3 and R4 phases, report
  jobs.h, jobs.c         job table, scheduler, traps (R2, R3 memory, R4 channel)
  memory.h, memory.c     native heap (R3)
  channel.h, channel.c   channel table, ids, ownership, console driver (R4)
  irq.h, irq.c           IRQ7 and IRQ15 classification (R4 Phase 0)
  context.s              IRQ stubs, yield, resume, IF save/restore, software int helpers
  descriptors.c          GDT, IDT (IRQ0 0x20, IRQ7 0x27, trap 0x30, IRQ15 0x2F)
tests/
  test_jobs.c/h          R2 test jobs
  test_memory.c/h        R3 heap tests and memory job phase
  test_channels.c/h      R4 channel jobs and the IRQ checks
platform/
  platform.h             platform interface (console, timer, PIC access, halt)
  pc/                    Tiny386 console (port 0x402), PIC/PIT, halt, boot sector, image
  i386vm/                console (MMIO 0x800001), synthetic timer, halt; no PIC
docs/
  R4-CHANNEL-ABI.md      native channel services, codes, errors, limits
  R4-CHANNEL-DESIGN.md   design note (implemented; differences in section 10)
  R4-RUNTIME-REPORT.md   commands, hashes, results, IRQ review, limitations
  JS-ROM-CHANNEL-MAP.md  JS ROM TRAP #2 / TRAP #3 audit (68000 ABI, for reference)
  R3-FW-VALIDATION.md    known-good firmware validation, memory map, IRQ review
  R3-MEMORY-DESIGN.md, R3-MEMORY-ABI.md, R3-RUNTIME-REPORT.md, JS-ROM-MEMORY-MAP.md,
  JS-ROM-KERNEL-MAP.md, TINY386-COMPATIBILITY.md, R1-RUNTIME-REPORT.md
scripts/
  run-all.sh             build both targets and run the matrix
logs/
  i386vm-run{1,2,3}.log, pc-run{1,2,3}.txt, i386vm-notimer.log   R4 matrix
  known-good-bios/       R3 matrix with the known-good BIOS (comparison)
  seabios-local-build/   R3 matrix with the previous local SeaBIOS build (comparison)
  r2-baseline/           R2 regression logs (comparison)
external/
  seabios-local-changes.diff, seabios-untracked.txt   from the earlier local build
patches/
  i386vm-synthetic-timer.patch   i386VM synthetic timer (applies to the harness)
```

## Build and run

External inputs are not included. Set their locations:

- `I386VM`: the i386VM harness tree (`bin/i386-gcc`, `bin/i386-ld`, `bin/i386vm`).
- `TINY386`: the `tiny386_headless` binary built from the unmodified Tiny386 tree.
- `SEABIOS`: a directory with `bios.bin` and `vgabios.bin`. The validated baseline
  is the known-good pair in `docs/R3-FW-VALIDATION.md` §2.

```
I386VM=... TINY386=... SEABIOS=... scripts/run-all.sh
```

Or build only: `make i386vm pc I386VM=... TINY386=... SEABIOS=...`.

Outputs:

- `build/i386vm/qdos386-r4.bin`: flat image for the i386VM harness.
- `build/pc/r4pc.img`: Tiny386 hard-disk image (SeaBIOS boots the boot sector).
- `build/i386vm/qdos386-r4-notimer.bin`: negative-test image.

## Services (summary)

Trap `int 0x30`, EAX = function, EBX/ECX/EDX = arguments, EAX = result.

| Code | Service |
|---|---|
| `0x08`–`0x0F` | R2 job and scheduler services |
| `0x10`–`0x13` | R3 memory: allocate, free, free bytes, validate |
| `0x14` | R4 `QT_CH_OPEN`: name in EBX, returns channel id |
| `0x15` | R4 `QT_CH_CLOSE`: channel id |
| `0x16` | R4 `QT_CH_WRITE`: channel id, buffer, length (up to 256) |
| `0x17` | R4 `QT_CH_PUTC`: channel id, one byte |

Full definitions, error codes and limits are in `docs/R4-CHANNEL-ABI.md` and
`docs/R3-MEMORY-ABI.md`. These are native i386 services. They are not the
68000 trap ABI, and they are not compatible with QDOS.

## Not in R4

- Keyboard input, files, directories, serial, VGA, SuperBASIC, ELF loading.
- Blocking I/O. The console is synchronous.
- JS ROM BASIC relocation and transient-area growth.
- Dynamic job-table size (fixed at 4) and channel table size (fixed at 8).
