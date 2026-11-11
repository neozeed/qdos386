# Locking, interrupts and blocking (canonical baseline)

Source of truth: `kernel/bkl.h` (the big kernel lock), `kernel/sync.h`, `kernel/r20w.h`, the interrupt notes in
`kernel/irq.h` and `kernel/vcon.h`. Design history: `docs/R16-ARCHITECTURE.md`, `docs/R16B-SCHEDULER-POLICY.md`,
`docs/A16-CACHE-DESIGN.md` (reservations and deadlock rules).

## The big kernel lock (BKL)

- One spinlock (`bkl_word`) serialises all shared-kernel execution on R16 and later. It is recursive per CPU
  (`bkl_depth[cpu]`); `bkl_leave` releases only at depth 0 and is a no-op on a CPU that does not own it.
- Held with interrupts off. Never held across an `iret` to user code (`resume_dispatch` releases it first).
- Never held while waiting for a device. A blocking service schedules another job, and that job releases the lock.
- Acquisition is a locked `xchg` spin. Deadlock is excluded because the owner releases without needing the waiter's CPU.

## Interrupts

- Every trap, every interrupt handler and boot code run with interrupts off. Virtual-console and surface code relies on this
  instead of locks (`kernel/vcon.h`).
- Device IRQs are on CPU 0 (the BSP). On the PC the IDE path is polled PIO: no IRQ14 and no DMA are used.
- Spurious IRQ7 handling is in `kernel/irq.h`.

## Blocking and scheduling

- A service that must wait sleeps the caller and returns a restart or wait code; the caller re-executes the trap when woken.
  The wake-before-sleep and completion-before-sleep orderings are fixed by the trap that does both (`r20w_trap`).
- `QT_SUSPEND` sleeps for a tick count; `QT_JOB_KILL` and `QT_PROC_KILL` are the termination paths.

## Storage worker (R20)

- The worker body runs WITHOUT the BKL. Every transition of shared worker state happens inside a trap, under the BKL with
  interrupts off.
- The storage claim is a single owner for the IDE controller. A sync trap waits for it with the BKL held, bounded by
  `R20_DEV_SPIN` polls; the worker clears the claim without the BKL.
- No lock and no BKL is held while the worker polls the disk or sleeps.

## Rules for new code

1. Take the BKL only through `bkl_enter`/`bkl_leave`; never write the lock word directly.
2. Do not block while holding the BKL. Sleep through the scheduler, and re-check state after the wake.
3. Any state shared between a trap and the worker is changed only in a trap.
