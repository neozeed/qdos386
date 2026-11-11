# QDOS/386 R16B — foreground-first SMP scheduling — HANDOVER

## What this is

An opt-in R16B kernel (`make qdos386-r16b-pc`) on top of frozen R16A: per-CPU CPU-usage and BKL-contention accounting
with a `TOP` tool, safe migration of protected (CPL3) jobs between the two CPUs with a `CPUSET` tool, and a QDOS-style
**foreground-first** policy — CPU 0 keeps the devices and favours the focused session's protected jobs, CPU 1 runs
background protected jobs — with an optional work-conserving `LEND` mode. The Big Kernel Lock is kept. KCTX-FIX2, R15,
R16 and R16A builds are byte-identical (`make qdos386-pc` `f8e5d5e1…`/`7c675978…`, `make qdos386-smp-pc` `b9ecb69a…`).

**Both milestones were reached:** the minimum (accounting + safe manual migration) and the stretch (automatic
foreground-first migration with verified background progress). See docs/R16B-RUNTIME-REPORT.md for the numbers.

## Baseline

Branch `r16b-foreground-smp` from tag `r16a-mbr-cpu-taskmanager` (`d4827f3`; R16A package sha256 `a81b267b…`).
Gate tags: `r16b-gate0`, `r16b-gate1`, `r16b-gate2`, `r16b-gate3`, final `r16b-foreground-smp`.

## Build and run

```
. scratchpad/env-r11c.sh ; export QDOS386_TOOLS        # GCC 1.40 / GAS 1.38 / i386-ld at $QDOS386_TOOLS
make qdos386-r16b-pc                                   # build/qdos386-r16b/pc-vga/qdos386-r16b-vga.img (QEMU)
sh scripts/r16b-build-guest.sh                         # R16TP, TOPP, CPUSETP, BOUNCEP (CPL3-marked)
sh scripts/r16b-sysvol.sh vol.img [mbr]                # test volume with the tools (optionally MBR-partitioned)
QEMU_MEM=32 QEMU_EXTRA="-smp 2 -accel tcg,thread=multi" \
  R12_QKERNEL=build/qdos386-r16b/pc-vga/qdos386-r16b-vga.img sh scripts/r12-run-qemu.sh vol.img SEQ OUT tag 300
```

Or boot the package's `examples/system-r16b-*.img` (public-safe) as `hdb` with `kernels/qdos386-r16b-vga.img` as `hda`.

At the shell: `TOP [samples] [ticks]`, `CPUSET <jobid> 0|1|AUTO`, `CPUSET POLICY [FG|LEND|OFF]` (FG is the boot default;
OFF = R16A placement). Only protected programs move; ordinary `.AOU` programs run on CPU 0 exactly as before.

## Tests (bounded; one emulator at a time)

| script | what |
| --- | --- |
| `tests/r16/run-gate1.sh` (with `R16_GATE1_KERNEL=…r16b…`) | CPL3 protection, 24 checks, `-smp 1` |
| `R16T MIGTEST <cycles> <sleepy> [rounds] [interval]` | exact-result migration test of an unchanged computation |
| `R16T MIGKILL n`, `R16T AFFCHK`, `R16T ASK` | kills during pending migration; SETAFF error contract; CPU-1 job blocked on input |
| `tests/r16b/run-gate3.sh MODE OUT` (0 OFF, 1 pins, 2 FG, 3 LEND) | three sessions under one focus timeline; `gate3_analyze.py` |
| `tests/r16b/run-stage4.sh OUT a b c` | Gate 1 on R16B; MBR-disk matrix; text + graphics focus switching |
| `tests/r16b/run-soak.sh OUT [blocks]` | mixed soak, FG then LEND |

## Key results

* One CPL3 job alone on a CPU: 50% → **100%** (the idle job no longer competes).
* 1000 + 300 external CPU flips of an x87/memory/register-checked computation: exact results match the never-moved
  reference; 1298 completed migrations; 40/40 kills during pending migration; no leaks; `badrel = 0`.
* Gate 3 (three sessions, focus cycling): foreground work unit 9.0 / 7.3 ticks with FG vs 21.8 / 22.0 with R16A
  placement and 10.3 / 9.5 with hand pins; foreground wake latency bounded at 1 tick; 10 event-driven migrations.
* Graphics: a CPL3 BOUNCE followed focus between CPUs, 1500 frames, `verify=PASS`.
* **Defect found and fixed:** ring-0 process exit ran `schedule()` without the BKL in R16/R16A — the true source of
  their nonzero `bkl_badrel`. Plus the idle-job competition and two placement holes (docs/R16B-KNOWN-LIMITATIONS.md).

## Source map (all R16B code is gated by `-DQDOS_R16B` or in new files)

* `kernel/smp.c` / `smp.h` — accounting, STATS/POLICY ops, R16B SETAFF rules, FG policy, LEND.
* `kernel/jobs.c` / `jobs.h` — `pin`, `moved_tick`, `lent_tick`; dispatch note; idle job as fallback; lending hooks;
  `job_exit` takes the BKL.
* `kernel/bkl.c`, new `kernel/r16b_s.s` — contended-acquisition count and TSC wait time.
* `kernel/proc.c` — ring-0 processes on CPU 0 at load. `kernel/vcon.c` — focus generation in `vcon_switch`.
* `tests/r16b/top.c`, `cpuset.c`; R16T additions in `tests/r16/r16t.c`; scripts `scripts/r16b-*.sh`, `tests/r16b/*`.

## Documents

`docs-r16b/R16B-SCHEDULER-POLICY.md`, `R16B-MIGRATION-ABI.md`, `R16B-RUNTIME-REPORT.md`, `R16B-KNOWN-LIMITATIONS.md`
(including the supported / unsupported application modes), `R16B-AUDIT.md` (Stage 0 ownership audit).

## Next steps

Per-CPU run queues; a reschedule IPI to cut migration latency below one tick; splitting the BKL (console and heap first);
policy granularity finer than a whole session.
