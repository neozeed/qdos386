# JS ROM kernel map

Source: `js-rom-gas-r2.zip` (the brief calls it `js2-gas.zip`), file `src/system_asm.s`.
Labels were checked against that file. Corrections to the brief are marked **[corr]**.
Status column: **Done** = ported and exercised by a test; **Partly** = ported
without some of the original behaviour; **Not in R1** = not ported.

## Routines

| Label | Behaviour in the JS ROM (verified) | Status | i386 implementation |
|---|---|---|---|
| `l00900` | Polled interrupt: `SV.POLLM` += 1, run the poll list, then fall into the scheduler if the interrupted context was user mode. | Done | `sched_irq_tick` (kernel/jobs.c): POLLM++, poll list, scheduler pass. Entered from the IRQ0 stub. |
| `l00936` | Scheduler entry: `jsr l009d4` (save), then the pass. **[corr]** The brief calls this the scheduler entry; it is the save-and-schedule sequence, reached by fall-through from `l00900`. | Done | `schedule()` |
| `l0093a` | Reads and clears `SV.POLLM`, increments `SV.RAND`, runs the schedule list, selects a job, runs it. | Done | `schedule()`: `pollm = sv_pollm; sv_pollm = 0; sv_rand++;` schedule list, `pick_next(pollm)`. |
| `l009d4` | Save current job: reset a non-zero accumulated priority to 1; store registers into the job header. | Done | `schedule()` stores `saved_sp`/`saved_fmt`; resets `acc_prio`. |
| `l00a0c` | Select: candidates after the current job, current job last. Skip inc = 0. Timeout countdown for status > 0. Accumulated priority, strict maximum, first found wins ties. | Done | `pick_next(pollm)` |
| `l00a78` | Restore selected job and `rte`. | Done | `resume_dispatch` in context.s, two frame formats |
| `l00a9e` | Runs a linked task list. | Done | `run_tasks()` over `poll_list` (SV.PLIST) and `sched_list` (SV.SHLST) |
| `tr1d08` | Suspend: status = timeout (word), JOB.HOLD = a1, then the scheduler. | Done | `QT_SUSPEND` → `trap_dispatch` → `schedule()` |
| `tr1d09` | Reactivate: if status ≠ 0, clear it and release the hold flag (`sf`); then the scheduler. | Done | `QT_REACTIVATE` |
| `tr1d0a` / `o9a0` | Activate: fails if increment ≠ 0, else sets the increment and START. The wait option (JOB.WJOB) is not implemented. | Partly | `QT_ACTIVATE`, no wait option |
| `tr1d0b` / `o990` | Change priority: set increment; if 0, clear accumulated priority. | Done | `QT_CHPRIO` |
| `o524` (job info) | Returns the job's status and priority. | Partly | `QT_INFO` returns increment and accumulated priority; status not included |
| `l003bc` / `l003e4` | Look up a job by id and check its TAG. | Done | `job_lookup()`: id = (tag << 16) | slot. Stale ids fail. |
| `l0058c` / `l005f0` | Create job: allocate a slot, set the TAG, link the job. | Done | `QT_CREATE` → `job_alloc()` |
| `l00408` | Current job id. | Done | `job_current` |
| `l00246` | Job table init. Job 0 is BASIC with increment `0x20`. | Partly | Job 0 is SYSTEM with increment `0x20`. There is no BASIC job. |
| Trap table | QDOS traps via the 68000 trap table. | Partly | Traps are `int 0x30` with a function code in EAX (`QT_*` in kernel/jobs.h). The numbers follow the JS ROM trap names; the 68000 vector mechanism is not reproduced. |

## Job record (native), with JS ROM fields

| Native field | JS ROM offset | Use |
|---|---|---|
| `saved_sp` / `saved_fmt` | `0x5c` (A7), `0x60` (SR), `0x62` (PC) | Saved state. The format bit selects the cooperative or interrupt frame. |
| `acc_prio` | `0x12` | Accumulated priority |
| `prio_inc` | `0x13` | Priority increment. 0 = not active. |
| `status` | `0x14` | 0 runnable; > 0 timeout ticks; < 0 indefinitely suspended |
| `hold` | `0x0c` | Release flag pointer, set to 0xFF on release |
| `tag` | `0x10` | Part of the job id |
| `in_use`, `dead` | — | Slot state, with deferred reclamation |

## System variables

| Offset | Name | Use in R1 |
|---|---|---|
| `0x2e` | SV.RAND | Incremented each scheduler pass (`sv_rand`) |
| `0x30` | SV.POLLM | Ticks since the last pass (`sv_pollm`) |
| `0x3c` | SV.PLIST | Poll list head (`poll_list`) |
| `0x40` | SV.SHLST | Schedule list head (`sched_list`) |
| `0x64` | SV.JBPNT | `job_current` |
| `0x68` | SV.JBBAS | Job table: a fixed array of four slots |

## Selection rule (l00a0c, as implemented)

For each candidate after the current job, current job last:

1. Skip if the job is not in use or `prio_inc == 0`.
2. If `status > 0`: `status -= POLLM`. Still suspended while > 0. On expiry set `status = 0` and `*hold = 0xFF`.
3. Skip if `status < 0`.
4. `acc = 1` if it was 0, else `acc + inc`, saturating at 255.
5. The strictly highest `acc` wins. The first candidate found wins ties.

Before the pass, the outgoing job's `acc` is reset to 1 if non-zero (l009d4).

## Reclamation

A terminated job keeps its slot until another job runs a scheduler pass. That
pass frees the slot, so a job's stack is never freed while it is still in use.
A freed slot gets a new tag when reused, so the old id becomes stale (`QT_REACTIVATE` returns -2).

## Remaining differences

- Job 0 is SYSTEM (increment `0x20`). The JS ROM BASIC job is not present.
- Activate's wait option and the job-info status field are not implemented.
- Slots are a fixed table of four, not the JS ROM's variable-length job area.
- Traps are `int 0x30`, not the 68000 trap vector table.
- The poll wait flag (JOB.WFLAG, checked at `l0064c`) is not modelled.
- Stacks are fixed 4 KiB per slot.
