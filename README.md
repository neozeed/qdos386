# QDOS/386 R1 — native i386 kernel on the i386VM harness

A freestanding 32-bit kernel with QDOS-inspired job scheduling, built with the
supplied GCC 1.40 / GAS 1.38 toolchain and run on the i386VM harness.

## Status

R1 checkpoint: **verified on i386VM** (cooperative switching, timer
preemption, independent stacks, termination, idle, and a validated PASS line).

Changes from the brief:
- **Target:** i386VM, not Tiny386. The supplied Tiny386 is only a CPU core
  (see docs/TINY386-COMPATIBILITY.md). No BIOS boot sector.
- **Timer:** a synthetic IRQ0 register added to the harness (patches/).
  This is not a PIT model.

## Build

```
make I386VM=/path/to/i386vm          # produces build/qdos386.bin (raw image)
make I386VM=/path/to/i386vm run      # runs it on the harness
```

The harness must include the synthetic-timer patch:
`patch -p1 < patches/i386vm-synthetic-timer.patch` in the i386VM tree, then
`bash build-emulator.sh`. Without it the timer register does not exist and
the kernel will report FAIL.

## Layout

```
kernel/context.s   descriptor loading, IRQ0 stub, cooperative yield, job start
kernel/jobs.c/h    job table, QDOS accumulated-priority scheduler
kernel/platform.c/h console, timer register, GDT/IDT
kernel/main.c      SYSTEM job, idle job, final validation and PASS line
tests/test_jobs.c  TEST-A and TEST-B (CPU-bound, canary checks)
tests/bootstrap_timer_test.s   stand-alone timer bootstrap test (pre-kernel)
docs/              kernel map, Tiny386 compatibility, runtime report
patches/           harness change (synthetic timer)
logs/              three deterministic runs and the timer-off control
```

## Limitations

See docs/R1-RUNTIME-REPORT.md.
