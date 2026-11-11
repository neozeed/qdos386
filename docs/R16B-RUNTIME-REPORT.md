# R16B — runtime report

Platform: QEMU `qemu-system-i386 -machine pc -cpu pentium -m 32`; two CPUs with `-smp 2 -accel tcg,thread=multi`;
host with 2 cores. Kernel `build/qdos386-r16b/pc-vga/qdos386-r16b-vga.img` (sha256 `d073fbea…` for every Stage 4,
Gate 3-final and soak run). Guest tools built by `scripts/r16b-build-guest.sh`; volumes by `scripts/r16b-sysvol.sh`.
Gate 3, Stage 4 and the soak each ran a single emulator; the Gate 1 before/after runs ran two emulators at a time
(utilisation ratios are per-CPU tick samples and unaffected; BKL wait times are not — see below). Logs are under `logs/r16b/` in the build tree (selection in
`evidence/` in the package).

## Gate 0 — baseline (R16A bytes, `qdos386-smp-vga` `b9ecb69a…`)

`-smp 1` R16 Gate 1: 24/24. `-smp 2`: MBR-partitioned FAT16 boot disk mounts, `R16T RACE` both children exit 0,
`badrel = 0`, Task Manager CPU column correct. `-DQDOS_R16B` alone changes no kernel byte.

## Gate 1 — accounting and TOP

`TOP` on `-smp 2` and `-smp 1`; counters monotonic, `badrel = 0`, no fault.

| case | before (R16A idle rule) | R16B |
| --- | --- | --- |
| idle system | CPU0 ~99% idle, CPU1 100% idle | same |
| one CPL3 job pinned to CPU 1 | CPU1 ≈ 50% cpl3 / 50% idle | CPU1 **100%** cpl3, CPU0 ~99% idle |
| one job on each CPU | ≈ 50% / 50% | **100% / 100%** |
| `-smp 1`, one CPU-bound job | — | CPU0 100% |

The 50% was the idle job winning the accumulated-priority contest; R16B makes it a fallback (docs/R16B-SCHEDULER-POLICY.md).
BKL *wait time* (TSC-based) is not a trustworthy number on this host, even with one emulator: two vCPU threads, QEMU's
own threads and the capture harness share two host cores, so a descheduled lock holder inflates a waiter's measured
wait (CPU 1 accumulates millions of ×1024-cycle units per run). The contended-acquisition counts (hundreds to a few
thousand per run, e.g. 4198 / 3658 over the 8-block soak) are the figures to rely on (docs/R16B-KNOWN-LIMITATIONS.md).

## Gate 2 — safe manual migration (`-smp 2`, then `-smp 1`)

`R16T MIGW` is an unchanged computation (integer LCG, a 16 KiB memory image, an x87 accumulator held for the whole run,
and each round the callee-saved-register / ESP-residue checker); `R16T MIGTEST` runs it pinned to CPU 0 as a reference
and then again while flipping its CPU from outside every 3 ticks.

| test | flips accepted | completed migrations (kernel) | CPU changes seen by the job | result |
| --- | --- | --- | --- | --- |
| blocked mode (sleeps a tick per round) | 1000 / 1000 | 999 | 997 | `2294010e` = reference, regs OK |
| running mode (never sleeps) | 300 / 300 | 299 | 299 | `541a91a6` = reference, regs OK |

`R16T MIGKILL 40`: 40/40 workers killed while a CPU change was pending (alternately fresh, and already moved to CPU 1);
LEAK identical before and after. `R16T AFFCHK`: CPU1 `0`, AUTO `0`, CPU0 `0`, CPU 7 `−91`, after the kill `−94`.
`CPUSET`: stale id → "no such job", SYSTEM and the ring-0 shell → "not a protected (CPL3) job". `-smp 1`: every CPU-1
request `−90`, AUTO `0`, the job still completes with the correct result. `badrel = 0` throughout.

**`bkl_badrel` investigation.** An early Gate 2 run showed `badrel = 2`. Every entry stub that reaches
`resume_dispatch` takes the BKL except `_yield_asm`, used only by `job_exit`. A legacy ring-0 process ends in job
context through `job_exit`, so R16/R16A ran `schedule()` unlocked and released a lock they did not own; that run had
executed `LEAK.AOU` (ring-0) twice. Fix: `job_exit` takes the BKL. Verification: three `LEAK` exits plus a migration
test → `badrel = 0`; every later run reports 0. This also explains R16's earlier unexplained `badrel = 1`.

## Gate 3 — foreground-first policy

Three CPU-bound protected `LOOPW` jobs in text sessions 1–3 (`R16T GATE3`), focus: shell 4 s, sessions 1, 2, 3, 1 for
5 s each, then the shell; 25 s; logs read back from the disk image (`tests/r16b/run-gate3.sh`, `gate3_analyze.py`).
Two independent runs (run 1 in parentheses; host variance ~10–15 %):

| policy | fg ticks / unit | fg wake latency mean / max | bg ticks / unit | units | completed migrations |
| --- | --- | --- | --- | --- | --- |
| OFF | 21.8 (22.0) | 2.17 / 3 | 22.0 | 311 (321) | 0 |
| pins 0/1/1 | 10.3 (9.5) | 1.09 / 3 | 12.3 | 589 (646) | 1 |
| **FG** | **9.0** (7.3) | **1.00 / 1** | 18.0 | 491 (572) | 10 |
| LEND | 8.3 | 1.00 / 1 | 14.3 | 577 | 459 |

Foreground reports ran 100% on CPU 0 and background reports ≥ 99% on CPU 1 under FG. An earlier LEND variant without
borrow hysteresis made 1137 migrations in the same 25 s; the hysteresis halved that and LEND was made opt-in.

## Stage 4 — targeted verification (`tests/r16b/run-stage4.sh`)

* **a, `-smp 1`:** R16 CPL3 Gate 1 on the R16B kernel — **24/24**.
* **b, `-smp 2`, MBR-partitioned system disk:** `RACE` done (two CPL3 jobs on two CPUs); `STRESS 300` ok 300 / bad 0;
  `KILLW` (CPL3 job on CPU 1 killed from CPU 0) rc 0, wait −66; `RACEPRIV` #GP on CPU 1 contained, parent survives;
  `MIGTEST 100` MATCH (99 CPU changes); `MIGKILL 10` 10/10; Task Manager shows the background PIN job (session 3) on
  CPU `1` under FG; **`ASK` pinned to CPU 1 blocked on console input and read `hello-typed-on-cpu0-keyboard` on CPU 1**;
  partition region `fsck.fat -n` clean; LEAK identical before/after; `badrel = 0`.
* **c, `-smp 2`, text + graphics focus switching:** LOOPW (text session 1) and a CPL3-marked BOUNCE (graphics session 4),
  BOUNCE focused through the Task Manager. Task Manager CPU column: session 1 focused → LOOPW `~0`, BOUNCE `~1`; BOUNCE
  focused → **BOUNCE `~0`, LOOPW `1`**. LOOPW's own log: CPU 0 while session 1 is focused, CPU 1 while session 4 is.
  BOUNCE: 1500 frames, 482 shown / 1018 hidden, `errs=0`, `sum = model`, **`verify=PASS`**; screendump shows its live
  surface. (A first attempt focused session 2 by Alt+F3 — an empty text console, because a `/NEW` graphics surface gets a
  dynamic session; BOUNCE then ran entirely hidden. The test, not the kernel, was corrected.)
* **x87:** covered by MIGW's x87 accumulator matching across 1000 + 300 migrations and in every MIGTEST above.

## Mixed soak (`tests/r16b/run-soak.sh`, `-smp 2`)

Each block: two LOOPW jobs in sessions 1 and 2 with focus switching, `MIGTEST 40`, `STRESS 60`, `RACE`, `LEAK`; first
half policy FG, second half LEND.

* 8 blocks, 424 s wall: every MIGTEST MATCH (0 mismatches), 8/8 RACE done, STRESS 60/60 each block, LEAK identical after
  all 8 blocks, 0 faults, `badrel = 0`, 368 completed migrations. (Shorter than the 10–15 min target; not counted as such.)
* **15 blocks, 805 s wall (13.4 min) — the 10–15 minute mixed workload:** 0 MIGTEST mismatches, 15/15 RACE done,
  STRESS 60/60 in every block, **LEAK identical after all 15 blocks**, 0 faults, `badrel = 0`, 690 completed migrations
  (330 into CPU 0, 360 into CPU 1), 8447 / 7107 contended BKL acquisitions.

## Identity

`make qdos386-pc` → `f8e5d5e1…` / `7c675978…` (KCTX-FIX2) and `make qdos386-smp-pc` → `b9ecb69a…` (R16A) are unchanged
by every R16B commit; all R16B code is under `-DQDOS_R16B`, in new files, or in test/tool sources.
