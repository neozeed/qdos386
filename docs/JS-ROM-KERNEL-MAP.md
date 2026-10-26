# JS ROM kernel map (R1 Phase 0)

Source: `js-rom-gas-r2.zip` (the brief calls it `js2-gas.zip`), file `src/system_asm.s`.
All addresses are the labels in that file. Offsets are byte offsets from the
named base register (A6 = system variables, A0/A2/A3 = job header).
Labels were checked against the source. Corrections to the brief are marked **[corr]**.

## Routines

| Label | What it does (verified) | R1 status | i386 replacement |
|---|---|---|---|
| `l00900` | Polled interrupt: `SV.POLLM` +1, run poll list, then if the interrupted context was user mode it falls into the scheduler; supervisor mode returns. | Replaced | `irq0_stub` (context.s) → `sched_irq_tick` |
| `l00936` | Scheduler entry: `jsr l009d4` (save), then the scheduling pass. **[corr]** The brief calls it the scheduler entry; it is the entry of the *save-and-schedule* sequence, reached by fall-through from `l00900`. | Replaced | `schedule()` in jobs.c |
| `l0093a` | Reads and clears `SV.POLLM`, increments `SV.RAND`, runs the schedule task list (`SV.SHLST`), selects a job, runs it. | Partly | Tick counter; no schedule task list |
| `l009d4` | Saves the current job: resets accumulated priority to 1 if non-zero, stores D0-D7/A0-A4, A5, A6, USP, SR, PC into the job header. | Replaced | `schedule()` stores `saved_sp`, `saved_fmt`; resets `acc_prio` |
| `l00a0c` | Selection. Round robin from current+1, current examined last. Each runnable job with increment ≠ 0: accumulated = 1 if 0, else + increment (saturating). Highest accumulated wins; first found wins ties. Returns -2 if none. | Replaced | `pick_next()` |
| `l00a78` | Restores the selected job: pushes PC and SR, sets USP, loads registers, `rte`. | Replaced | `resume_dispatch` (context.s), two formats |
| `l00a9e` | Runs a linked task list (poll, schedule, interrupt). | Not reproduced | — |
| `tr1d08` | Suspend job: stores the timeout in `JOB.STAT`, saves the job's release reference, returns to `l00936`. | Partly | `job_suspend_self()`, no timeout |
| `tr1d09` | Reactivate job: clears status, releases a hold flag, returns to `l00936`. | Not in R1 | — |
| `tr1d0a` | Activate job: sets increment, copies START, optionally waits. | Partly | `job_create()` |
| `tr1d0b` (`o990`) | Change priority: sets increment; clears accumulated priority if increment is 0. | Not in R1 | — |
| `l003bc` / `l003e4` | Look up a job by ID, checking its TAG. | Not reproduced | Index-based slots |
| `l00408` | Returns current job ID. | Replaced | `job_current` |
| `l00246` | Initialises the job table. Job 0 is BASIC, increment `0x20` (`l00246`: `move.b #0x20,0x0013`). | Not reproduced | Job 0 is SYSTEM, increment 1 |
| `l0058c` / `l005f0` | Create-job path: sets job descriptor, allocates the tag, links the job into the tree. | Partly | `job_create()` |

## Job header (offsets from the job base, from l009d4 / l00a0c / l00a78)

| Offset | Field | Used by R1 |
|---|---|---|
| `0x00` | Job base pointer (list entry points to it) | — |
| `0x04` | START address | — |
| `0x08` | Owner | — |
| `0x0c` | JOB.HOLD (release flag pointer) | No |
| `0x10` | TAG (checked by `l003e4`) | No |
| `0x12` | Accumulated priority (byte) | Yes: `acc_prio` |
| `0x13` | Priority increment (byte) | Yes: `prio_inc` |
| `0x14` | Status word: 0 runnable; > 0 suspended with timeout; < 0 suspended indefinitely | Yes, as runnable / suspended / dead |
| `0x18` | WJOB | No |
| `0x1c` | JOB.TRAPV | No |
| `0x20`-`0x5b` | D0-D7, A0-A6 | Yes: saved in the i386 frame |
| `0x5c` | A7 (USP) | Yes: `saved_sp` |
| `0x60` | SR | Yes: EFLAGS in the IRQ frame |
| `0x62` | PC | Yes: EIP in the IRQ frame |

## System variables used by the scheduler

| Offset | Name | Use |
|---|---|---|
| `0x2e` | SV.RAND | Counter; not used in R1 |
| `0x30` | SV.POLLM | Poll-tick counter; R1 uses the timer tick count |
| `0x35` | SV.PCINT | Interrupt mask; not used |
| `0x3c` | SV.PLIST | Poll task list; not used |
| `0x40` | SV.SHLST | Schedule task list; not used |
| `0x50` | SV.TRAPV | Trap vector; not used |
| `0x62` | SV.JBMAX | Highest job number; implicit (4 slots) |
| `0x64` | SV.JBPNT | Current job pointer; `job_current` |
| `0x68` | SV.JBBAS | Job table base; static `jobs[]` |
| `0x7c` | SV.JBTOP | Top of job table; not used |

## Selection rule (exact in R1, from l00a0c)

For each candidate in round-robin order starting after the current job, with
the current job last:
1. Skip if not runnable (status ≠ 0) or increment = 0.
2. Accumulated priority = 1 if it was 0; otherwise accumulated + increment, saturating at 255.
3. Choose the strictly highest accumulated priority. First candidate found wins ties.

Before the pass, the outgoing job's accumulated priority is reset to 1 if non-zero (`l009d4`).

## Divergences

- No suspend timeout countdown (`SV.POLLM` vs `JOB.STAT`). R1 uses suspended-indefinitely only.
- No poll or schedule task lists, job TAG checks, release flags, or trap-table dispatch.
- Job 0 is SYSTEM with increment 1. The JS ROM BASIC job uses increment 0x20.
- Stacks are per-job static buffers (4 KiB). The QDOS heap is not used.
- Preemption is driven by the synthetic timer (step-based), not the poll interrupt.
- Interrupts are not nested.
