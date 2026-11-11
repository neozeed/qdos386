# Apollo 13 (R17) — CPUs, topology and the HT policy

Opt-in: `make qdos386-r17-pc`. One big kernel lock (BKL) is kept, as in R16B. CPU 0 (the BSP) owns every external
device interrupt (8259 PIC). No IOAPIC is programmed, and no device IRQ is routed to an AP.

## 1. Bring-up (up to 8 logical CPUs)

* **Enumeration**: ACPI MADT (enabled LAPIC entries), else the MP table, de-duplicated (`detect_cpus`). At most
  `KX_NCPU` = 8 are used. Any others are counted as enumerated and not started.
* **Per AP**:
  * a 16 KiB stack from the R17 kernel area;
  * its own TSS, selector 0x30 + 8·n (0x38…0x70), with ESP0 updated on every dispatch;
  * a halting idle job `IDLn`;
  * a LAPIC periodic timer (vector 0x40, count calibrated once against PIT channel 2);
  * per-CPU accounting.

  The CPU index is derived from TR in every assembly entry stub: (TR − 0x30)·2 = 16·cpu, the `kx_pc` row offset.
* **Sequential start with a barrier** (`start_one`): copy the 0x9000 trampoline, INIT, 10 ms, SIPI (+1 retry), then
  wait up to 200 ms for `ap_ready`. The AP sets `ap_ready` at the end of `smp_ap_main` — after loading its TSS,
  probing its own CPUID topology and arming its timer. Only then is the next AP started.
* **An AP that does not arrive** is sent INIT again, which holds it in wait-for-SIPI. Its idle job is cancelled, it is
  marked offline, and its APIC id is recorded as failed. It is never scheduled. The next AP reuses the slot.

  Tested with `R17_EXTRA=-DQDOS_R17_TEST_APFAIL=2`, where the first AP tried for slot 2 gets no SIPI. Result:
  `SMP: 3 of 4 CPU(s) online, 1 did not start (never scheduled)`, then WORK, MIG and LEAK all pass on the three
  CPUs (logs/a13/p4).

## 2. Topology detection (each CPU probes itself)

Each CPU runs CPUID on itself: the BSP in `smp_boot`, each AP in `smp_ap_main`, before `ap_ready`.

| Method | Condition | Decoding |
| --- | --- | --- |
| `CPUID leaf 0xB` | max leaf ≥ 0xB, subleaf 0 is type SMT (1) with a non-zero count, subleaf 1 is type Core (2) | SMT shift = 0xB.0 EAX[4:0], core shift = 0xB.1 EAX[4:0]; the x2APIC id from EDX must equal the 8-bit LAPIC id |
| `legacy (leaf 1 + 4)` | leaf 1 EDX.HTT and max leaf ≥ 4 | logical = 1.EBX[23:16], cores = 4.EAX[31:26] + 1; SMT shift = bits(logical/cores), core shift = bits(logical) |
| `unknown` | neither (e.g. `-cpu pentium`, HTT without leaf 4) | flat: every CPU its own core, **no sibling invented** |

`topo_finish` cross-checks all online CPUs. Each must have:

* the same method and the same shifts;
* a CPUID APIC id equal to its LAPIC id;
* an APIC id that is in the MADT/MP list.

Any mismatch gives `TOPO_BAD`, which is reported, and the policy falls back to flat. `pkg`, `core` and `thread` are
decoded from the APIC id. SMT is *proven* only if two online CPUs share a package and a core.

## 3. Policy

Foreground first, unchanged from R16B:

* The job owning the **foreground session runs on CPU 0**.
* Every other protected (CPL3) job is a background job.
* Ring-0 jobs (the shell's built-ins, SYSTASK, legacy programs) stay on CPU 0.

Placement:

* **Background placement** (`pick_bg`): the least-loaded allowed CPU, compared by load first and then by rank. Load is
  the number of protected jobs whose affinity is that CPU.
* **A job stays put** while its CPU remains allowed.
* **Hysteresis**: a job is not moved again within 25 ticks (250 ms).
* **Policy pass**: on CPU 0's tick, at a focus change, or every 50 ticks. It re-places jobs and rebalances one job when
  two allowed CPUs differ by **two or more**.
* **Pins** (`CPUSET pin`, `QT_SMP SETAFF` 0…7) beat the policy. `SETAFF 8` (AUTO) returns a job to the policy.
* **LEND** (R16B's two-CPU borrowing) is not offered in R17.

### HT modes (`CPUSET HT AUTO|PHYSICAL|ALL`, `QT_SMP` op 7)

| Mode | Allowed background CPUs | Order |
| --- | --- | --- |
| `ALL` (0) | every online CPU | index order |
| `AUTO` (1, default when SMT is proven) | every online CPU | thread 0 of the other cores first, then their siblings, **CPU 0's own sibling last**. That sibling also counts one job heavier, so the foreground core keeps its sibling idle unless the machine is full. |
| `PHYSICAL` (2) | CPU 0 + thread 0 of every other core | siblings are **parked**: `pick_next` runs only their idle job (HLT). Pins on a parked CPU are cleared (`unpinned`). A new pin onto one is refused with **-96**. AUTO jobs on it are moved at once, without waiting out the hysteresis. |

`AUTO` or `PHYSICAL` without proven SMT is refused with **-97**. When topology is unknown or there are no siblings,
the mode is `ALL` (flat).

### CPL3 interface (`QT_SMP` = trap 0x3E)

| Op | Name | Words returned | Notes |
| --- | --- | --- | --- |
| 0 | INFO | | w23 = the calling CPU |
| 1 | SETAFF | | job, CPU 0…7, or 8 for AUTO |
| 3 | STATS v2 | 144 | w0 = 2, w1 = online, w12 = badrel, w14 = HT mode, w15 = topology \| smt<<8 \| ncpu<<16 \| unpinned<<24; per-CPU block at 16 + 16·c (see the table below) |
| 4 | POLICY | | FG \| OFF |
| 5 | MEMINFO | 192 | |
| 6 | TOPO | 96 | method, smt, MADT ids, failed ids, per-CPU apic/cpuid-apic/method/pkg/core/thread |
| 7 | HT | | b = mode (0 ALL, 1 AUTO, 2 PHYSICAL) or 0xFF to query; returns the previous mode or -96/-97 |

STATS v2 per-CPU block (word 16 + 16·c, then index):

| Index | Field |
| --- | --- |
| 0 | ticks |
| 1 | cpl3 |
| 2 | cpl0 |
| 3 | idle |
| 4 | ctxsw |
| 5 | migr |
| 6 | BKL acq |
| 7 | contended |
| 8 | spins |
| 9 | wait |
| 10 | current id |
| 11 | idle id |
| 12 | depth |
| 13 | apic |
| 14 | pkg<<16 \| core<<8 \| thread |
| 15 | flags (1 online, 2 parked, 4 parking, 8 sibling of CPU 0) \| load<<8 |

Tools:

* `/BIN/TOP` (R17) prints one line per CPU: `CPUn pPcCtT cpl3 cpl0 idle sw mig cont job load` plus `PARKED`, `(parking)` or `cpu0-sib`. It
  still understands STATS v1 from an R16B kernel.
* `/BIN/CPUSET` and `/BIN/SMPT` (INFO, HTCHK) report the topology.
* The Task Manager CPU column shows 0…7 for a running job and ~n for the CPU a job last ran on.

## 4. Measured (QEMU TCG multi-threaded, 2-core host; logs/a13/p3)

| QEMU | Detected | Default | Behaviour |
| --- | --- | --- | --- |
| `-cpu Nehalem -smp 8,sockets=1,cores=4,threads=2` | leaf 0xB, apic 0…7, core = apic>>1, thread = apic&1; SMT proven | AUTO | 3 BUSY jobs → CPUs 2, 4, 6 (distinct cores, 100% CPL3). 6 jobs → 2…7; CPU 1 (CPU 0's sibling) idle. PHYSICAL → 1, 3, 5, 7 PARKED (0% CPL3), load 2 each on 2, 4, 6. ALL → rebalanced onto 1…6. AUTO again → a job moved off CPU 1 to 7. `SMPT HTCHK`: pin on sibling CPU 3 rc 0; PHYSICAL parks it, clears the pin, refuses a new pin with -96; ALL re-admits → PASS. |
| `-cpu Nehalem -smp 4,sockets=1,cores=4,threads=1` | leaf 0xB, no SMT | ALL | AUTO/PHYSICAL refused with -97; 3 BUSY jobs on CPUs 1, 2, 3. |
| `-cpu pentium -smp 8,sockets=1,cores=4,threads=2` | **unknown** (no leaf 0xB/4) | ALL (flat) | -97 for AUTO/PHYSICAL; 2 BUSY jobs on CPUs 1, 2; `TOP` shows `-` for the topology column. |
| harness default `-cpu pentium`, `-smp 1/2/4` | unknown (flat) | ALL | Phase 2 results (logs/a13/p2) |

On every run: `badrel=0` and no unexpected PGFAULT. The only PGFAULTs are the deliberate `SMPT FAULT` CLI tests, which
are contained.
