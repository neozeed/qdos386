# QDOS/386 R1 (extended) runtime report

Two targets, one kernel:

- **PC:** Tiny386 full PC emulator (headless build), SeaBIOS boot sector,
  8259 PIC, 8254 PIT. Output via port 0x402.
- **i386VM:** the R1 harness, with the synthetic IRQ0 register. Output via 0x800001.

## Required verification (brief, section "Required verification")

| # | Item | PC | i386VM | Evidence |
|---|---|---|---|---|
| 1 | Cross-compiler and linker build | PASS | PASS | `make`; GCC 1.40 + GAS 1.38 + i386-ld |
| 2 | Boot-image structural validity | PASS | PASS | Boot sector 512 bytes, signature 55 AA; kernel image `make-image.py` checks sector count, extract checks OMAGIC, no relocations, origin, entry |
| 3 | Tiny386 boot | PASS | n/a | logs/pc-raw1-3.txt: SeaBIOS boot, then `Booting from 0000:7c00`, kernel output |
| 4 | Protected-mode entry | PASS | PASS | `Protected mode: OK`, `Paging: disabled` (CR0 read by the guest) |
| 5 | Cooperative context switch | PASS | PASS | `TEST-A: resumed after cooperative yield`; `COOPERATIVE YIELDS: 4` |
| 6 | Timer interrupt delivery | PASS | PASS | `TIMER: ticks 17`. PC: PIT on IRQ0 through the PIC. i386VM: device count matches (CHECK device ticks: ok) |
| 7 | Preemptive switching between CPU-bound jobs | PASS | PASS | `TEST-A: preempted yes`, `TEST-B: preempted yes` |
| 8 | Independent stack preservation | PASS | PASS | Canary check every iteration; `canary_errors == 0` |
| 9 | Job termination and idle transition | PASS | PASS | `TEST-A/B/C: exited`; `IDLE: no application jobs remain`; `IDLE DISPATCHES: 3` |
| 10 | Repeated deterministic runtime behaviour | PASS (3/3 identical after the boot lines) | PASS (3/3 identical) | logs/*-run*.log, logs/pc-raw*.txt |

## JS ROM job-table extension (this stretch)

| Check | PC | i386VM | What it shows |
|---|---|---|---|
| CHECK chprio | ok | ok | Change priority, then read it back |
| CHECK timeout wake | ok | ok | SYSTEM suspended for 3 ticks: woke after ≥3 ticks, release flag set |
| CHECK reactivate | ok | ok | TEST-B reactivated SYSTEM from an indefinite suspend, and the release flag was set |
| CHECK poll/sched lists | ok | ok | Poll and schedule tasks ran |
| CHECK slot reuse | ok | ok | TEST-C reused TEST-A's slot after deferred reclamation |
| CHECK stale id | ok | ok | TEST-A's old id returns -2 after the reuse |
| CHECK new id | ok | ok | New id is valid; activating an active job returns -1 |
| CHECK device ticks | n/a | ok | Device counter on i386VM only |

## Negative controls

- **i386VM, timer off (verified):** the run does not finish: `instruction limit exceeded`, because SYSTEM waits on ticks that never come.
  An earlier version of this kernel (without the job-table port) reported FAIL
  with zero preemptions. The PASS line is only printed after the checks run.

## Sample run (PC, logs/pc-raw1.txt)

```
QDOS/386 R1
Native i386 kernel

CPU: i386 protected mode
Protected mode: OK
Paging: disabled
Kernel: initialized

SYSTEM: job 0 created
TEST-A: job created by trap
TEST-B: job created by trap
SCHEDULER: preemption enabled
TIMER: IRQ0 active

TEST-A: resumed after cooperative yield
TEST-A: iteration 1
...
IDLE: no application jobs remain
TEST-A: preempted yes
TEST-B: preempted yes
CHECK chprio: ok
...
CHECK device ticks: n/a
TIMER: ticks 17
COOPERATIVE YIELDS: 4
SCHEDULER PASSES: 36
POLL RUNS: 17
SCHEDULE RUNS: 36
IDLE DISPATCHES: 3

QDOS/386 R1 MULTITASKING PASS
```

## Not verified or not implemented

- Tiny386 timing is host-clock based. Repeat runs gave identical output here, but this is
  not a guarantee under different host load.
- The PC timer test relies on ~100 Hz real time. A faster or slower host changes
  wall-clock duration, not the tick-paced flow.
- Activate's wait option and job-info status are not implemented.
- Stacks are fixed 4 KiB per slot. No stack-overflow detection.
- Interrupts are not nested. Any CPU exception has no handler; the emulator stops.
- Only the PC port I/O listed in docs/TINY386-COMPATIBILITY.md is used.
- The brief's wider QDOS API (SuperBASIC, filesystem, graphics) is out of scope.
