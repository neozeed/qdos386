# R16 — known limitations and boundaries

R16 is an experiment that reaches a defensible first step into true SMP: two CPL3 applications on two CPUs over one
BKL-serialized kernel. It is deliberately conservative. This document is the honest boundary of what it does and
does not do, and the one bug that mattered during bring-up.

## The bug that had to be fixed for two CPUs: the BKL acquire

The first two-CPU bring-up hung hard — both CPUs spun forever in `bkl_pause` (confirmed via the QEMU monitor: both
vCPUs at the `ret` of `_bkl_pause`, IF=0). Cause: the BKL acquired with an unconditional `xchg` that wrote the owner
id into the lock word *even when losing the race*. CPU 0 could win (xchg returns 0, it enters) while CPU 1's xchg,
running just after, overwrote `bkl_word` with its own id and spun; CPU 0's `bkl_leave` then saw a foreign owner,
counted a `badrel`, and never released. Fix: acquire with a compare-and-swap (`bkl_cmpxchg`, a hand-encoded
`LOCK CMPXCHG` because GAS 1.38 has no mnemonic) that writes the owner **only when the word is 0**, so a loser never
clobbers the winner. `-smp 1` never exercised concurrent acquires, which is why Gates 1–2 passed before this surfaced.
This is fixed and verified; it is recorded here because it is the subtle, load-bearing correctness point of the whole
design.

## Scope limitations (by design)

* **Exactly two CPUs.** The code discovers all CPUs but brings up only CPU 1; per-CPU state is sized `[2]`. Extending
  to N CPUs is mechanical (arrays and the `str`→index map) but untested and not done.
* **One Big Kernel Lock.** All kernel execution is serialized. This is the point — correctness over scalability. Two
  CPL3 jobs run their *user* code in parallel; any syscall, fault or timer entry on either CPU serializes on the BKL.
  A syscall-heavy workload on CPU 1 will contend heavily (visible as large `bkl_spins[1]`). Finer-grained locking is
  explicitly out of scope.
* **Static affinity, no migration.** A job runs where it is pinned. Only a protected (CPL3) job may be placed on
  CPU 1; a ring-0 job is refused there (`SMP_ERR_RING0`). The default placement is CPU 0, so nothing moves to CPU 1
  unless asked (`QT_SMP SETAFF`, or a job pinning itself). There is no load balancing.
* **CPU 1 owns no device.** All IRQs (timer/clock, keyboard, serial, disk, VGA) are CPU 0's. CPU 1 has only its LAPIC
  timer for preemption. A job on CPU 1 reaches every device through the shared kernel under the BKL (its console
  writes, file I/O, etc. all work — they just run under the lock, physically touching the device from whichever CPU
  holds it). CPU 1 cannot service a hardware interrupt.
* **No cross-CPU TLB shootdown is needed, and none is done.** Address spaces are per-job and a job is never dispatched
  on two CPUs at once, so a mapping a CPU uses is never mutated by the other. Freeing is deferred until the owning CPU
  deschedules the job (see below), so there is no live mapping to invalidate on the other CPU. If a future change let
  two CPUs share a writable mapping, it would need a shootdown that R16 does not implement.

## BKL diagnostic counters

`bkl_badrel` counts calls to `bkl_leave` by a CPU that does not own the lock. `bkl_leave` is a safe no-op in that
case, so a non-zero `badrel` does not leak or mis-hold the lock; it is a diagnostic only. In isolation every test
leaves `badrel=0`. Under a long *mixed* load (1000 spawns interleaved with a kill, a fault and two-CPU concurrency in
one boot) a very small, non-growing count (observed: 1) can appear, while the substantive invariants still hold: per-CPU
`acq − rel` stays balanced (no lock owner is ever leaked), the lock is only ever held with interrupts off, and it is
never held across an `iret` to user. The counter is retained precisely so this is observable. Driving it to a
provable 0 under every interleaving would require either a formal argument about the few non-interrupt-gate entry
paths or removing the defensive no-op; it was left as a visible diagnostic rather than hidden.

## Memory and lifetime

* RAM is identity-mapped to 16 MiB (`-m 32` is fine; the kernel maps 16 MiB). Larger RAM is not mapped.
* A killed or exited job is only *marked* dead; its address space, stack, channels and heap are reclaimed by
  `reclaim_dead`, which skips any job still `running_on` the other CPU. So a cross-CPU kill is safe but the free is
  deferred to the target CPU's next reschedule (up to one timer quantum). Verified by `R16T KILLW`.

## Toolchain / platform constraints carried from earlier milestones

* Built only with the historical toolchain (GCC 1.40, GAS 1.38, i386-ld) at `$QDOS386_TOOLS`. GAS 1.38 has no
  `cmpxchg`, `.code16`, or `movl %cs,%eax`; these are hand-encoded or worked around in `kernel/smp_s.s`,
  `kernel/bkl_s.s`, `kernel/kctx3.s`. GCC 1.40 ignores `__attribute__((aligned))`, so page-aligned tables (the LAPIC
  page table) are over-allocated and aligned by hand.
* Tested under QEMU TCG (single- and multi-threaded). Not tested on other emulators or real hardware; the LAPIC/MADT
  path assumes a standard PC APIC as QEMU presents it.
* The `QDOS_R16_APNOTIMER` build macro (masks CPU 1's timer, leaving it dormant) exists only as a diagnostic and is
  off by default; it is not part of any shipped target.

## Not attempted

Finer-grained locking, per-CPU run queues with balancing, N > 2 CPUs, device interrupts on CPU 1, CPL3 for the shell
or system jobs, and any change to the frozen releases or the ordinary KCTX-FIX2 kernel. R16 is additive and opt-in; it
leaves every prior deliverable byte-identical.
