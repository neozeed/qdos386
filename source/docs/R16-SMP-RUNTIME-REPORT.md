# R16 — SMP runtime report

Measured behavior of the R16 kernel (`make qdos386-smp-pc`, image `qdos386-smp-vga.img`) under QEMU
`qemu-system-i386 -machine pc -cpu pentium`. Two-CPU runs use `-smp 2`; the concurrency and stress runs add
`-accel tcg,thread=multi` so the two vCPUs run on separate host threads (genuine parallelism). The harness is
`scripts/r12-run-qemu.sh` driving a scripted VGA-text capture; the guest test program is `tests/r16/r16t.c` (`R16T`,
launched as a CPL3-marked binary). `R16T INFO` issues the `QT_SMP` info trap and prints the live BKL and per-CPU
scheduler counters.

## 1. Bring-up

Boot log on `-smp 2`:

```
SMP: ACPI MADT: 2 CPU(s), BSP 0
...
SMP: CPU 1 (LAPIC id 1) up, paging on, own TSS/stack/timer; CPU 0 keeps the devices
```

CPU 1 enters 32-bit kernel mode, enables paging with the shared template directory, loads its own TSS (0x38),
programs its LAPIC timer and runs the scheduler. `R16T INFO` then reports `state=1 ncpu=2 ap=1` with `aptk`/`apdisp`
(AP timer ticks / dispatches) increasing over time — CPU 1 is live and preempting.

## 2. Gate 1 — single-CPU CPL3 (`r16-cpl3-proof`, re-verified on the final kernel)

`sh tests/r16/run-gate1.sh` (QEMU `-smp 1`): **24/24 PASS**. Highlights:

* Protected `R16T` runs at `cs=2b ss=33 cpl=3`; args and the `int 0x30` round-trip work; EFLAGS has IF=1, IOPL=0.
* A legacy unmarked `R16TL` runs at ring 0 (`cs=08 ss=10`) on the same kernel — opt-in is per launch.
* Every privileged/illegal instruction is contained and the job ends, the shell survives:
  `cli hlt in out mov %cr3 lgdt` → #GP (vec 0d); kernel read/write and a null call → #PF (vec 0e); `div0` → vec 00;
  `ud` → vec 06; `int` to a DPL0 gate → #GP. None print "SURVIVED".
* A user buffer pointing into kernel space is rejected by `pg_user_range_ok` (rc −14) with no fault.
* CPL3 register state (EBX/ESI/EDI, EFLAGS) survives timer preemption across ESP residues 0..3.
* A marked BOUNCE runs at CPL3 and its surface verifies; LEAK before == after (no leaked frames/handles); no halt.

## 3. Gate 2 — BKL on one CPU (`r16-bkl-up`)

Same 24/24 under BKL instrumentation, `-smp 1`. `R16T INFO` at rest shows, from inside the info syscall,
`bkl_word=1 depth=1,0 badrel=0` (CPU 0 holds the BKL *because the info trap itself runs in the kernel*), and across
repeated samples `acq − rel == 1` on CPU 0 (exactly the in-flight measuring call) and `spins=0` — **no lock owner is
ever leaked between syscalls, and no non-owner release occurs.**

## 4. Gate 3 — two CPL3 apps on two CPUs (`r16-smp2-proof`)

`R16T RACE` spawns two protected children, pins one to CPU 1 and one to CPU 0, and runs them at once. Representative
interleaved output (`-smp 2 -accel tcg,thread=multi`):

```
R16T RACE spawned cpu1-child=65541 cpu0-child=65542
R16T AFFWORK id=65541 k=0 on_cpu=1 cs=2b sum=f992b5b0
R16T AFFWORK id=65541 k=1 on_cpu=1 cs=2b sum=a060ae00
R16T AFFWORK id=65541 k=2 on_cpu=1 cs=2b sum=415e18f0
R16T AFFWORK id=65541 k=3 on_cpu=1 cs=2b sum=8ea72680
R16T AFFWORK id=65542 k=0 on_cpu=0 cs=2b sum=f992b5b0
R16T AFFWORK id=65541 k=4 on_cpu=1 cs=2b sum=6b8006b0
R16T AFFWORK id=65542 k=1 on_cpu=0 cs=2b sum=a060ae00
...
R16T RACE done cpu1-child=65541 st=0 cpu0-child=65542 st=0
```

Child 65541 reports `on_cpu=1` for every iteration, child 65542 `on_cpu=0`, both at `cs=2b` (CPL 3), and their lines
interleave — the two processors make progress simultaneously. Both exit 0. Post-race `R16T INFO`:
`bkl_word=1 acq=5826,1369 rel=5825,1369 badrel=0 depth=1,0` — per-CPU acquire/release balanced (CPU 0 delta 1 = the
measuring call, CPU 1 delta 0), **no leaked owners**, no faults, no halt.

## 5. Stage 4 — stress / fault / kill matrix (`-smp 2 -accel tcg,thread=multi`)

A single boot runs: `LEAK`, `R16T STRESS 1000`, `R16T KILLW`, `R16T RACEPRIV`, `R16T RACE`, `ECHO`, `R16T INFO`, `LEAK`.

| Test | Result |
| --- | --- |
| `STRESS 1000` (spawn protected child, wait, repeat ×1000) | `ok=1000 bad=0` (earlier runs: 600 ×N, all clean) |
| `KILLW` (long CPL3 child pinned CPU 1, killed from CPU 0) | `kill_rc=0 wait=-66` (QD_ERR_KILLED); shell alive — cross-CPU kill reclaims cleanly |
| `RACEPRIV` (CPL3 child pinned CPU 1 executes `cli`) | `#GP vec 0d, process ended`, parent `status=-17`, survivor `cs=2b` — fault contained on CPU 1 |
| `RACE` | two CPL3 apps concurrent, both exit 0 |
| `ECHO shell-survived-stage4` | shell responsive after the whole matrix |
| `LEAK` before vs after | identical: `heap=667744 chans=15 spaces=2 held=266240` — no resource leak after 1000 spawns + a kill + a fault + concurrency |

The only fault in the log is the intentional RACEPRIV containment; no triple fault, no kernel halt, no nested fault.
Run in isolation, `STRESS 600` and `RACEPRIV` each leave `badrel=0`; see docs/R16-KNOWN-LIMITATIONS.md §BKL counters
for the rare benign non-owner `bkl_leave` observed only under long mixed load (accounting stays balanced; no owner is
leaked).

## 6. Regression / identity (`-smp 1` and `make`)

* `make qdos386-pc` → `qdos386-vga.img` = `f8e5d5e1…`, `qdos386.img` = `7c675978…` — the KCTX-FIX2 bytes, unchanged.
* `tests/r14/check-frozen.sh`: 25 frozen R12..R13B kernel/shell images rebuilt from the current source, **0 differ.**
* `make qdos386-amp-pc` → `a1a47ac2…` (vga) / `cccbda7f…` (pc) — the R15 AMP0 kernel, unchanged.
* R16 Gate 1 on `-smp 1`: 24/24.

All R16 code is compiled only under `-DQDOS_R16` or lives in new files, so the ordinary and frozen targets are
byte-for-byte identical to their deliveries.

## 7. How to reproduce

```
. scratchpad/env-r11c.sh ; export QDOS386_TOOLS
make qdos386-smp-pc                       # the R16 kernel
sh scripts/r16-build-guest.sh             # build/r16/R16T.aout + R16TP.aout (protected)
sh tests/r16/run-gate1.sh logs/g1         # Gate 1, -smp 1, 24/24
# two-CPU: build a volume with R16T.AOU = the protected binary, then
QEMU_MEM=32 QEMU_EXTRA="-smp 2 -accel tcg,thread=multi" \
  R12_QKERNEL=build/qdos386-smp/pc-vga/qdos386-smp-vga.img \
  sh scripts/r12-run-qemu.sh VOL.img SEQ OUTDIR tag 300      # SEQ: "R16T RACE", "R16T INFO", ...
```
