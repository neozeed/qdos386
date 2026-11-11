# R16B — migration and statistics ABI (`QT_SMP`, trap 0x3E)

Applies only to the opt-in R16B kernel (`make qdos386-r16b-pc`). On an R16/R16A kernel the R16 semantics apply
(SETAFF 0/1 only, no STATS/POLICY); on a non-SMP kernel `QT_SMP` is an unknown trap. Calls are `qtrap(0x3E, op, b, c)`.
No new trap number and no general syscall framework were added: R16B extends the existing `QT_SMP` operations.

| op | name | b | c | returns |
| --- | --- | --- | --- | --- |
| 0 | INFO | – | buffer, 24 words | 0 (unchanged from R16) |
| 1 | SETAFF | job id | `0`, `1`, or `2` = AUTO | 0 or an error |
| 2 | START | – | – | SMP state (unchanged) |
| 3 | STATS | buffer size in words (≥ 10) | buffer | words written (≤ 48) or an error |
| 4 | POLICY | `0` OFF, `1` FG, `0xFF` query | – | the previous policy |

## SETAFF (R16B rules)

* **Who can move:** only a live **protected (CPL3)** job. Kernel jobs (SYSTEM, SYSTASK), the idle jobs (IDLE, IDL1) and
  legacy ring-0 processes always stay on CPU 0 → `SMP_ERR_RING0` (−92). (R16 accepted CPU 0 for any job, which would
  let the CPU-1 idle job be placed on CPU 0 while CPU 1's idle fallback could still return it — a double-run.)
* `c = 0/1` sets a **manual pin**; `c = 2` (AUTO) clears it — the job then follows the policy, or CPU 0 if the policy
  is OFF or CPU 1 is absent.
* Errors: `SMP_ERR_NOJOB` −94 (no live job with that id, incl. stale ids), `SMP_ERR_DEAD` −95 (terminating),
  `SMP_ERR_ARG` −91 (CPU not 0/1/2), `SMP_ERR_UNAVAIL` −90 (CPU 1 requested but not present), `SMP_ERR_RING0` −92.
* **The call is a request, never a direct move.** It writes the job's affinity under the BKL and returns. If the job is
  running on a CPU it may no longer use, that CPU keeps running it until its own next `schedule()` pass (its next timer
  tick, ≤ 1 tick, or sooner if the job blocks); there `pick_next` no longer selects it, the frame and the x87 state are
  saved and `running_on` becomes −1. Only then can the destination CPU's `pick_next` take it. CR3, the per-CPU entry
  row and TSS.ESP0 are re-derived on the destination at dispatch. A blocked or ready job simply becomes selectable by
  the other CPU. A job is never on two CPUs at once and is never reclaimed while a CPU still owns it.
* A job may change its own CPU (as R16's `AFFWORK` does), and any job may change another live protected job, including
  one running on the other CPU (R16 refused that case with −93).

## STATS (version 1, 48 words)

Read-only snapshot taken under the BKL. Utilisation counters are **sampled on each CPU's own timer tick** (CPU 0: PIT
IRQ 0; CPU 1: LAPIC timer): each tick classifies what was interrupted. They are tick-granular estimates, not exact time.
All counters are 32-bit and wrap; consumers should use differences (TOP does).

| word | meaning |
| --- | --- |
| 0 | version (1) |
| 1 | CPUs in use (1 or 2) |
| 2 | SMP state (0 down, 1 up, 2 failed) |
| 3 | policy (0 OFF, 1 FG) |
| 4 | focus generation (bumped by every foreground change) |
| 5 | foreground session |
| 6 | affinity changes made (manual + policy) |
| 7 | affinity changes made by the policy |
| 8 | CPU 0 timer ticks since boot |
| 9 | policy moves deferred by the residency rule |
| 10 + 16·c + 0 | CPU c: timer ticks |
| … + 1 / 2 / 3 | ticks that interrupted CPL3 code / CPL0 code (kernel or a ring-0 job) / that CPU's idle job |
| … + 4 | context switches on CPU c |
| … + 5 | completed migrations into CPU c (a dispatch whose previous CPU was the other one) |
| … + 6 / 7 / 8 | BKL acquisitions / contended acquisitions / spin iterations |
| … + 9 | BKL wait, in units of 1024 TSC cycles (contended acquisitions only) |
| … + 10 / 11 / 12 | current job id, idle job id, BKL depth (as seen inside this call) |
| 42 / 43 / 44 | BKL word, non-owner releases (`bkl_badrel`), longest spin |

BKL wait time is read from the TSC. Under QEMU TCG the guest TSC follows host time, so when the host has fewer free cores
than vCPUs (for example two emulators running at once on a two-core host) a descheduled lock holder inflates a waiter's
measured time. The contended-acquisition count is the robust figure.

## POLICY

`POLICY(1)` turns on foreground-first placement; `POLICY(0)` turns it off; `POLICY(0xFF)` queries. Changing the policy
bumps the focus generation, so every AUTO job is re-evaluated at the next CPU-0 tick. Manual pins are never overridden.
See docs/R16B-SCHEDULER-POLICY.md.

## Userland tools

* `CPUSET <jobid> 0|1|AUTO` and `CPUSET POLICY [FG|OFF]` — thin wrappers; every error code has a message.
* `TOP [samples] [ticks]` — per-CPU cpl3/cpl0/idle %, switches, migrations, BKL contention and wait, current job;
  totals first, then deltas; it sleeps between samples.

Both are built by `scripts/r16b-build-guest.sh` as CPL3-marked binaries (`TOPP.aout`, `CPUSETP.aout`).
