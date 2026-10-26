# R1 runtime report

Platform: i386VM harness with the synthetic timer patch. Toolchain: GCC 1.40,
GAS 1.38, a.out, `--raw`. Harness: `i386vm` built from `build-emulator.sh`
(`-O2 -DI386_ENABLE_FPU`).

## Required verification

| # | Item | Result | Evidence |
|---|---|---|---|
| 1 | Cross-compiler and linker build | PASS | `make`; `text=3056 data=44 bss=18664 entry=0`; no warnings |
| 2 | Image structural validity | PASS | Raw image 3100 bytes, entry 0, loaded at physical 0 (harness rejects bad images) |
| 3 | Tiny386 boot | **Not tested** on Tiny386 (not supplied as a machine). Run on i386VM instead | logs/run1.log |
| 4 | Protected-mode entry | PASS | Guest reads CR0: `Protected mode: OK`, `Paging: disabled` |
| 5 | Cooperative context switch | PASS | `TEST-A: resumed after cooperative yield`; `COOPERATIVE YIELDS: 4` |
| 6 | Timer interrupt delivery | PASS | `TIMER: ticks 33`; device count (low 8 bits) equals `sched_ticks` |
| 7 | Preemptive switching between CPU-bound jobs | PASS | `TEST-A: preempted 13 times`, `TEST-B: preempted 15 times` (run1) |
| 8 | Independent stack preservation | PASS | Canary check per iteration; `canary_errors == 0` for both jobs |
| 9 | Job termination and idle transition | PASS | `TEST-A: exited`, `TEST-B: exited`; `IDLE: no application jobs remain`; `IDLE DISPATCHES: 5` |
| 10 | Repeated deterministic runtime | PASS | Three runs, identical output MD5 (`logs/run1-3.log`) |

Control: with the timer disabled (`TIMER_PERIOD_UNITS 0`), the same code prints
`preempted 0 times`, `ticks 0`, and `QDOS/386 R1 MULTITASKING FAIL`, exit 1
(`build/negative-timer-off.bin`). The PASS line is therefore not printed
unconditionally.

## Sample run (logs/run1.log)

```
QDOS/386 R1
i386VM native boot

CPU: i386 protected mode
Protected mode: OK
Paging: disabled
Kernel: initialized

SYSTEM: job 0 created
TEST-A: job 1 created
TEST-B: job 2 created

TIMER: IRQ0 active
SCHEDULER: preemption enabled

TEST-A: resumed after cooperative yield
TEST-A: iteration 1
TEST-B: iteration 1
TEST-A: iteration 2
TEST-B: iteration 2
TEST-A: iteration 3
TEST-A: exited
TEST-B: iteration 3
TEST-B: exited
IDLE: no application jobs remain
TEST-A: preempted 13 times
TEST-B: preempted 15 times
TIMER: ticks 33
COOPERATIVE YIELDS: 4
IDLE DISPATCHES: 5

QDOS/386 R1 MULTITASKING PASS
```

The run ends with `HALT EAX=00000000`, exit status 0.

## Implemented

- Flat GDT (code 0x08, data 0x10), IDT with an IRQ0 interrupt gate.
- Job table with a native record and QDOS accumulated-priority selection.
- Two context formats: callee-saved (cooperative) and full interrupt frame
  (preemptive), both resumed by one dispatch routine.
- Fresh-job frames returning into a termination path (`job_trampoline` → `job_run` → `job_exit`).
- Suspend (SYSTEM), exit (test jobs), idle job, and validation before PASS.

## Not implemented or not proven

- The complete QDOS scheduler is not reproduced. Timeouts, poll and schedule
  task lists, job tags, trap dispatch, and reactivate/priority-change traps are absent.
- Tiny386 boot, BIOS boot sector, real PIC/PIT: not available in the supplied material.
- Resource reclamation: none. Stacks are static 4 KiB buffers and are not reused.
  The brief asks for deferred reclamation; it is not implemented.
- Interrupts are not nested. No EOI (no PIC).
- No stack-overflow detection; each stack is 4 KiB.
- Any CPU exception stops the harness (per the harness README). The kernel
  installs no handler for exceptions.
- Port I/O (`IN`/`OUT`) is not supported by the harness and is not used.
- Timer is step-based. Output order is deterministic for this build; it would
  change with any code change.
