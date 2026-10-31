# R16 — BKL / Ring 3 / two-CPU SMP experiment — HANDOVER

## What this is

The largest defensible step from R15 AMP0 toward true SMP: **two ordinary native QDOS/386 applications running
concurrently on two x86 processors, at CPL 3, in private address spaces, over one shared kernel serialized by a Big
Kernel Lock.** It is additive and opt-in — an R16 kernel runs every existing program unchanged, and an ordinary
`make` still produces the byte-identical KCTX-FIX2 kernels.

## Result against the mandate

| Required | Status |
| --- | --- |
| CPL 3 user mode (CS.RPL=3, SS.RPL=3, supervisor pages inaccessible), safe syscall + timer on one CPU | **met** — Gate 1, 24/24 |
| One serialized shared kernel under a BKL covering all shared state | **met** — Gate 2; BKL on every entry, held IF-off, never across `iret` |
| Ultimate: two existing CPL3 processes on two CPUs | **met** — Gate 3; two CPL3 apps interleaved on CPU 0 and CPU 1, both exit cleanly |
| Per-CPU current job / kernel stack / TSS.ESP0 / resume_fmt (no shared `kx_resume_fmt`) | **met** — §2 of R16-ARCHITECTURE |
| Kernel supervisor-only in every CR3; private address spaces | **met** — inherited R12A paging, audited |
| No deadlocks; no double-running jobs; safe memory lifetime/TLBs | **met** — CAS lock, atomic `running_on` ownership, deferred reclaim (KILLW) |
| Regression: frozen builds reproducible, default kernel unchanged silently | **met** — default + 25 frozen + AMP0 all byte-identical |

## Build and run

```
. scratchpad/env-r11c.sh ; export QDOS386_TOOLS        # historical toolchain (GCC 1.40 / GAS 1.38 / i386-ld)
make qdos386-smp-pc                                    # R16 kernel -> build/qdos386-smp/pc-vga/qdos386-smp-vga.img
sh scripts/r16-build-guest.sh                          # CPL3 test program -> build/r16/R16T.aout (+ R16TP.aout, marked protected)
sh tests/r16/run-gate1.sh logs/g1                      # Gate 1 (CPL3), QEMU -smp 1, expect 24/24

make qdos386-pc                                        # ordinary kernels, still f8e5d5e1… / 7c675978…
```

Two-CPU run (QEMU multi-threaded TCG for real parallelism):

```
QEMU_MEM=32 QEMU_EXTRA="-smp 2 -accel tcg,thread=multi" \
  R12_QKERNEL=build/qdos386-smp/pc-vga/qdos386-smp-vga.img \
  sh scripts/r12-run-qemu.sh VOL.img SEQ OUTDIR tag 300
```

where `VOL.img` is an R14A system volume built with `R16T.AOU` = the protected binary (`build/r16/R16TP.aout`), and
`SEQ` is a sequence file of shell commands — e.g. `R16T RACE` (two CPL3 apps on two CPUs), `R16T INFO` (live BKL and
per-CPU counters), `R16T STRESS 1000`, `R16T KILLW`, `R16T RACEPRIV`.

## Opt-in launch (how a job becomes CPL3)

Per launch, versioned, never by reinterpreting old binaries (full ABI in docs/R16-CPL3-ABI.md):

* **Executable mark**: bit 24 of the a.out magic (`0x01000107`). `tools/r16-protect-mark.py FILE [OUT]` sets/clears it.
  Frozen kernels reject such a file; the R16 loader accepts both and records the bit.
* **Spawn flag**: `qd_spawn(..., QD_SPAWN_PROT, ...)`.

Placement on CPU 1 is by the `QT_SMP` trap (`SMP_OP_SETAFF`), protected jobs only.

## The three frozen gates (git tags)

* `r16-cpl3-proof` — single-CPU CPL3 protection, 24/24.
* `r16-bkl-up` — the same under BKL instrumentation, balanced, no leaked owners.
* `r16-smp2-proof` — two CPL3 apps concurrent on two CPUs.

## Source map

New: `kernel/bkl.{h,c}`, `kernel/bkl_s.s` (the BKL + CAS primitive); `kernel/smp.{h,c}`, `kernel/smp_s.s` (CPU
discovery, bring-up, AP trampoline/timer, `QT_SMP`); `kernel/kctx3.s` (R16 entry/resume with per-CPU `str` indexing
and the CPL3 transitions); `kernel/uart_s3.s` (R16 serial IRQ stubs); `tools/r16-protect-mark.py`;
`scripts/r16-build-guest.sh`; `tests/r16/*`. Changed only under `-DQDOS_R16`: `descriptors.c` (user GDT selectors,
DPL3 gate, per-CPU + AP TSS), `proc.c` (per-CPU CR3/exc-depth, protected `prot` bit, exit stub, LAPIC map), `jobs.{c,h}`
(per-CPU current/idle, `kx_pc[2][4]`, affinity/`running_on`, schedule), `aout.c` (magic bit 24), `channel.c`/`uart.c`
(R16 `job_current` macro), `shellboot.c` (bring-up + IF-off SYSTEM poll), `Makefile` (`qdos386-smp-pc`).

## Documents in this package

1. **R16-ARCHITECTURE.md** — the two-CPU design: per-CPU state, the BKL, bring-up, scheduling/affinity, lifetime.
2. **R16-CPL3-ABI.md** — the CPL3 (protected-process) ABI: opt-in, what changes at ring 3, fault containment.
3. **R16-AUDIT.md** — the pre-existing protection foundations (U/S paging, supervisor-only kernel, buffer validation).
4. **R16-SMP-RUNTIME-REPORT.md** — the measured runs (bring-up, the three gates, the stress/fault/kill matrix, identity).
5. **R16-KNOWN-LIMITATIONS.md** — the boundaries, the one bring-up bug (the BKL acquire), and the BKL diagnostics.

## Next steps for a future builder

Finer-grained locking (split the BKL by subsystem, starting with the console and the heap), per-CPU run queues with
balancing, N > 2 CPUs (the per-CPU state is sized `[2]`; widen the arrays and the `str`→index map), and device
interrupts on CPU 1 (needs an IOAPIC routing path). None of these are attempted here; R16 stops at the first correct,
demonstrable two-CPU CPL3 result.
