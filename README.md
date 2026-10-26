# QDOS/386 R1 — native i386 kernel

A freestanding 32-bit kernel with a port of the JS ROM job scheduler and job
table, built with the supplied GCC 1.40 / GAS 1.38 toolchain. It runs on the
full Tiny386 PC emulator (SeaBIOS boot) and on the i386VM harness.

Status: checkpoint verified on both targets (see docs/R1-RUNTIME-REPORT.md).

## Build and run

```
make I386VM=/path/to/i386vm TINY386=/path/to/tiny386_headless SEABIOS=/path/to/seabios/out
make run-pc                  # Tiny386 PC: SeaBIOS boot, PIT/PIC timer
make run-i386vm             # i386VM harness (needs the synthetic-timer patch)
```

- **Tiny386:** build `tiny386_headless` from the full tree (`gcc -O3 *.c -lm`).
  SeaBIOS: `git clone https://github.com/coreboot/seabios.git`, apply
  `seabios/patch`, use `seabios/config`, then `make`. Copy `out/bios.bin` and `out/vgabios.bin`.
- **i386VM:** apply `patches/i386vm-synthetic-timer.patch` to the harness tree and
  run `bash build-emulator.sh`.

## Layout

```
kernel/
  context.s       cooperative and interrupt frames, IRQ0 and trap stubs, qtrap
  descriptors.c   flat GDT, IDT (IRQ0 0x20, trap 0x30)
  jobs.c/h        job table, JS ROM scheduler, service traps, reclamation
  main.c          SYSTEM job, IDLE job, validation and PASS line
platform/
  platform.h      platform interface
  pc/             boot.s, kernel.ld, entry.s, platform.c (PIT/PIC/0x402/0x8900),
                  make-image.py, extract-kernel.py
  i386vm/         platform.c (0x800001, synthetic timer), halt.s
tests/
  test_jobs.c/h   TEST-A, TEST-B (CPU-bound, canaries), TEST-C (reuses A's slot)
  bootstrap_timer_test.s   earlier i386VM timer bootstrap test
docs/
  JS-ROM-KERNEL-MAP.md     routine-by-routine port map
  TINY386-COMPATIBILITY.md
  R1-RUNTIME-REPORT.md
patches/          i386VM harness change (synthetic timer)
logs/             runs for both targets, legacy R1 logs
build/            outputs (created by make)
```

## Limitations

See docs/R1-RUNTIME-REPORT.md, "Not verified or not implemented".
