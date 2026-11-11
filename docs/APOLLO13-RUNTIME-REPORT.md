# Apollo 13 (R17) — runtime report

**Environment**: QEMU `pc` with SeaBIOS, TCG `thread=multi`, on a 2-core host. One emulator ran at a time. The kernel
under test is the R17 VGA kernel.

**Volume**: the public-safe system volume with the R17 tools (`scripts/a13-sysvol.sh`).

**Harness**: `scripts/r12-run-qemu.sh`. Its base CPU model is `-cpu pentium`; QEMU_EXTRA's `-cpu` overrides it where
given.

**Logs**: summaries are in `logs/a13/p*/` and shipped as `evidence/`.

## Gate 0 — baseline (R16B kernel, `-smp 2`)

`R16T RACE` done, and `MIGTEST 20` MATCH. `badrel=0` and no PGFAULT. See `docs/APOLLO13-AUDIT.md`.

## Gate 1 — memory (`-smp 1`, logs/a13/p1)

| Check | Result |
| --- | --- |
| Boot at `-m 16/32/64/128/256/512` | all boot. Pool 3,104 / 7,200 / 15,392 / 31,776 / 64,544 frames; 512 is clamped to 256 MiB and reported |
| `MEMINFO` | raw records, `check=OK`, `bad=0` |
| `RAMCHK` (CPL3) at 256 MiB | 3 × 64 MiB PASS, frames up to 0x0FF95000; over-asking is refused cleanly and the pool recovers to baseline |
| Concurrent RAMCHK at 64 MiB | 40 + 18 MiB PASS; exhaustion refused; recovery to baseline |
| Reserved-record injection kernel | `frames_in_pool=0`, `poison_mismatch=0` → PASS (also after exhaustion) |
| `R16T STRESS`, LEAK | ok; LEAK identical before and after |

## Gate 2 — N CPUs (`-smp 1/2/4`, logs/a13/p2)

| Check | `-smp 1` | `-smp 2` | `-smp 4` |
| --- | --- | --- | --- |
| online | 1 of 1 | 2 of 2 | 4 of 4 |
| `SMPT WORK` (same work pinned on each CPU) | PASS | PASS (identical results) | PASS (identical on all 4) |
| `SMPT MIG 30` (round-robin migration vs pinned reference) | MATCH (0 migrations; one refusal for the absent CPU 1) | MATCH, 29 migrations | MATCH, 30 migrations |
| `SMPT FAULT` (CLI at CPL3 on CPU 1/3) | contained (-17) | contained | contained |
| `SMPT KILL` (kill a job running on CPU 2) | -66 PASS | PASS | PASS |
| `SMPT SPAWN 12` | PASS | PASS | PASS |
| `R16T RACE` | done | done | done |
| badrel / LEAK | 0 / identical | 0 / identical | 0 / identical |

## Gate 3 — topology and HT (logs/a13/p3)

See `docs/APOLLO13-TOPOLOGY.md` §4. Summary:

* With 4 cores × 2 threads the topology comes from leaf 0xB.
* AUTO fills distinct cores and leaves CPU 0's sibling idle.
* PHYSICAL parks siblings 1, 3, 5, 7.
* ALL uses all CPUs.
* `HTCHK` PASS.
* 4 cores × 1 thread and `-cpu pentium` both go flat, and AUTO/PHYSICAL are refused with -97.

The Task Manager shows the focused session's job on CPU 0 (`~0`).

## Gate 4 — bounded stress (logs/a13/p4)

All Gate 4 runs use `-cpu Nehalem` unless noted. The scripts are in `tests/a13`.

### AP start-up failure

Test kernel: `R17_EXTRA=-DQDOS_R17_TEST_APFAIL=2`, `-smp 4`, where the first AP tried for slot 2 gets no SIPI.

* Boot line: `SMP: 3 of 4 CPU(s) online, 1 did not start (never scheduled)`.
* `SMPT INFO` reports `failed=1`.
* `SMPT WORK` PASS on 3 CPUs.
* 3 BUSY jobs placed on CPUs 1 and 2 only; the failed CPU's slot was never used.
* `SMPT MIG 40` MATCH (41 migrations).
* LEAK equals the baseline (`spaces=2 held=274432`).

### Mixed soak

Command: `tests/a13/run-soak.sh`, `-smp 8,cores=4,threads=2`, `-m 128`, 5 blocks, 928 s wall time.

Each block runs:

* two LOOPW jobs (compute plus a FAT16 log write) in sessions 1 and 2, with Alt+F2 / Alt+F3 / Alt+F1 focus switching;
* `SMPT MIG 220` (round-robin pin over all CPUs vs a pinned reference);
* `R16T STRESS 40`;
* `R16T RACE`;
* `SMPT SPAWN 8` (one worker pinned to each CPU);
* LEAK.

The HT mode cycles AUTO → PHYSICAL → ALL → AUTO → PHYSICAL.

| Check | Result |
| --- | --- |
| `SMPT MIG` | 5 / 5 MATCH, **1,088 migrations** in total. Under PHYSICAL the parked siblings refused 220 pins per block (-96, counted as `parked`, not as errors). |
| `R16T RACE` | 5 / 5 done |
| `R16T STRESS` | bad=0 |
| `SMPT SPAWN` | PASS (under PHYSICAL, workers aimed at parked siblings run under the policy) |
| LEAK | one value throughout (`heap=613488 chans=15 spaces=2 held=274432`) |
| badrel / PGFAULT | 0 / 0 |
| FAT16 | 10 LOOPW logs written (112–133 lines each) and read back on the host with mtools; `fsck.fat -n` clean |
| shutdown | `SHUTDOWN /N` → `ready` (no job left). `SHUTDOWN` → `no response` is expected on QEMU `pc`, which does not decode port 0x8900. The harness then stops QEMU. |

The first soak run found one test defect, which has been fixed. The old `SMPT SPAWN` treated a refused pin onto a
parked sibling as a failure (`ok=4 bad=4`), although the kernel behaved correctly and every result matched. SMPT now
records such refusals separately. Both runs are kept in `evidence/` (`p4-soak8-run1.summary`, `p4-soak8.summary`).

### Memory size × CPU count

Command: `tests/a13/run-memsmp.sh`. Each boot runs MEMINFO, two concurrent `RAMCHK 6 2` in background sessions,
`SMPT WORK 30` on every CPU, `SMPT SPAWN 8` and LEAK.

| `-m` / `-smp` | Online / topology | Pool (high) | WORK | SPAWN | Pool after |
| --- | --- | --- | --- | --- | --- |
| 16 / 1 | 1 / flat | 3,104 (0) | PASS | PASS | free back to 3,037, used 67 |
| 16 / 8 | 8 / leaf 0xB SMT, AUTO | 3,104 (0) | PASS (8 identical) | PASS | free back to baseline |
| 32 / 2 (1 core × 2 threads) | 2 / SMT, AUTO | 7,200 (4,064) | PASS | PASS | free back to baseline; high frames were used |
| 64 / 4 | 4 / SMT, AUTO | 15,392 (12,256) | PASS | PASS | free back to baseline |
| 128 / 8 | 8 / SMT, AUTO | 31,776 (28,640) | PASS | PASS | free back to baseline |
| 256 / 8 | 8 / SMT, AUTO | 64,544 (61,408) | PASS | PASS | free back to baseline |

### Graphics and foreground migration

Setup: `tests/a13/bounce.seq`, `-smp 8,cores=4,threads=2`, `-m 128`, AUTO. One LOOPW job plus **two protected BOUNCE
instances** (surface sessions 4 and 5).

* **Background placement** (Task Manager): R16T on `~2`, BOUNCE #1 on `~4`, BOUNCE #2 on `~6`. These are three
  distinct physical cores; CPU 0's sibling (CPU 1) stays idle.
* **Foreground migration**: after BOUNCE #2 is focused through the Task Manager, it shows `~0`. It migrated to CPU 0
  while the other two stayed on 2 and 4.
* **Correctness**: `BOUNCE frames=1500 … errs=0 sum=model → verify=PASS`. LEAK is back to baseline afterwards, with no
  PGFAULT.
* Screendump of the focused instance: `evidence/p4-a13-bounce-focused.png`.

EliteQL (separate private package) was not re-run for R17. It is optional per the handoff, and its source is not
part of this tree.

### Frozen outputs

The frozen images were rechecked after every phase: `f8e5d5e1`, `7c675978`, `b9ecb69a`, `d073fbea`, all unchanged.
