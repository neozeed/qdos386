# R16 — Ring 3 / Big Kernel Lock / two-CPU SMP architecture

This is the design of the R16 experiment: run two ordinary native QDOS/386 applications concurrently on two x86
processors, at CPL 3, in private address spaces, over a single shared kernel serialized by one Big Kernel Lock (BKL).
It applies only to the kernel built with `-DQDOS_R16` (`make qdos386-smp-pc`). An R16 kernel runs every existing
program unchanged, and an ordinary `make` still produces the byte-identical KCTX-FIX2 kernels (see
docs/R16-KNOWN-LIMITATIONS.md for the preserved hashes).

The CPL3 user-mode ABI is documented separately in **docs/R16-CPL3-ABI.md**; the pre-existing protection foundations
(U/S paging, supervisor-only kernel, buffer validation) are inventoried in **docs/R16-AUDIT.md**. This document
covers the two-CPU machinery: per-CPU kernel state, the BKL, CPU bring-up, scheduling and affinity, and the lifetime
rules that keep it safe.

## 1. Model: asymmetric devices, symmetric scheduling

* **CPU 0 (BSP)** owns every device: the PIT timer/IRQ0 and wall clock, the i8042 keyboard, the serial ports, the
  VGA/presenter, the disk. All legacy IRQs are delivered to CPU 0 as before.
* **CPU 1 (AP)** owns no device. It runs a private LAPIC timer for preemption and otherwise executes only ordinary
  jobs (and its own idle loop). It never fields a device IRQ.
* **Both CPUs run the same kernel image** and share all kernel data structures (the job table, the frame pool, the
  heap, the channel and console layers, the scheduler). Every access to that shared state happens under the BKL, so
  at most one CPU is ever executing in the kernel at a time. This is the classic Big Kernel Lock: correctness first,
  with device work pinned to CPU 0 so the two CPUs contend only for the lock, never for a device.

User code runs in parallel — two CPL3 jobs genuinely execute at the same time on the two processors. Kernel code does
not; the BKL serializes it. That is the defensible first step from AMP0 (R15) toward true SMP.

## 2. Per-CPU kernel state

Every datum the entry path reads or writes while dispatching a job is per-CPU, indexed by the processor's identity.
A CPU identifies itself from its task register (`str`): CPU 0's TSS selectors are 0x18/0x20, CPU 1's is 0x38, so
`kx_this_cpu()` (kernel/kctx3.s) returns 1 iff `str == 0x38`, else 0. The assembly entry stubs compute the same
index inline and keep it in EBP (0 for CPU 0, 16 for CPU 1 — a byte offset into the per-CPU rows); EBP is callee-saved
across the C handler, so it is still valid at `resume_dispatch`.

| State | Definition | Why per-CPU |
| --- | --- | --- |
| current job | `g_jobcur[2]`, `job_current` = `g_jobcur[kx_this_cpu()]` | each CPU runs a different job |
| idle job | `g_jobidle[2]`, `job_idle` = `g_jobidle[kx_this_cpu()]` | the two CPUs must never resume one shared idle context |
| entry frame row | `kx_pc[2][4]` = `{kx_lo, kx_hi, kx_entry, resume_fmt}` | kctx3.s selects the row by `str`; **replaces KCTX-FIX2's single global `kx_resume_fmt`, which must not be shared by two CPUs** |
| TSS.ESP0 | per-CPU TSS (0x18 for CPU 0, 0x38 for CPU 1), written by `kx_set_esp0` on the dispatching CPU | CPL3→CPL0 entry must land on the running CPU's kernel stack |
| current CR3 | `g_curcr3[2]` (proc.c) | each CPU has its own active address space |
| exception depth | `g_excdepth[2]` (proc.c) | a fault on one CPU must not disturb the other's nesting guard |

`kx_pc[cpu][3]` (the resume frame format) is the critical one: KCTX-FIX2's exact-ESP resume read a single global
`kx_resume_fmt` with interrupts off between the C handler's return and `resume_dispatch`. Two CPUs resuming at once
would clobber it. R16 gives each CPU its own row; `KX_RESUME` writes `kx_pc[kx_this_cpu()][3]`, and `resume_dispatch`
reads `_kx_pc+12(%ebp)` with EBP already holding this CPU's offset.

## 3. The Big Kernel Lock

`kernel/bkl.c`, `kernel/bkl.h`, `kernel/bkl_s.s`. One lock word serializes all in-kernel execution.

* **State**: `bkl_word` — 0 when free, else `owner_cpu + 1` (1 = CPU 0, 2 = CPU 1). Recursive via a per-CPU depth
  `bkl_depth[2]`; diagnostics `bkl_acq/rel/spins[2]`, `bkl_maxspin`, `bkl_badrel`.
* **Acquire** (`bkl_enter`): if `bkl_word` already equals our id, bump our depth and return (recurse). Otherwise spin
  on a **compare-and-swap** — `bkl_cmpxchg(&bkl_word, 0, cpu+1)` — which writes our id **only when the word is 0**.
  A CPU that loses the race never writes, so it cannot clobber the winner's id. (An earlier `xchg`-based acquire wrote
  the id unconditionally; the loser overwrote the winner and the winner could never release — both CPUs livelocked.
  This was the one bug that made two-CPU bring-up hang; see docs/R16-KNOWN-LIMITATIONS.md.)
* **Release** (`bkl_leave`): if we do not own it, do nothing (a defensive no-op; counted in `bkl_badrel`). Otherwise
  drop our depth; at depth 0 write 0 with a locked xchg.
* **Discipline**: the BKL is held **only with interrupts off**. Every kernel entry is an interrupt gate (IF cleared on
  entry) and calls `bkl_enter` right after saving state; `resume_dispatch` calls `bkl_leave` immediately before the
  `iret` back to the job. It is therefore never held across an `iret` to user, and a blocking syscall that schedules
  another job releases it (the switch runs under the lock; the resume drops it). The one place the kernel takes the
  BKL outside an interrupt gate — the SYSTEM poll loop in shellboot.c — wraps it in `irq_save`/`irq_restore` so the
  rule still holds; otherwise a timer could nest, run the scheduler under the recursed lock, and leak the lock across
  a context switch.
* **No deadlock**: there is exactly one lock and a CPU never blocks on anything else while holding it (device waits are
  CPU 0's and are expressed as scheduling, not spinning). The owner always makes progress and always releases without
  needing the waiter's CPU, so the spinner always eventually wins.

Entry points that take the BKL: `_trap_stub` (int 0x30), `_irq0_stub`, `_irq1_stub`, `_irq7_stub`, `_irq15_stub`,
`kx_exc_common` (CPU exceptions), `_smp_ap_timer_stub` (CPU 1's LAPIC timer). All reach `resume_dispatch`, which
releases once.

## 4. CPU bring-up

`kernel/smp.c`, `kernel/smp_s.s`, with GDT/TSS helpers in descriptors.c.

1. **Detect, pre-paging** (`smp_boot`, before `pg_init`): CPUID for a local APIC, read the APIC base MSR, parse the
   ACPI MADT (or the legacy MP table) for the CPU count and the AP's LAPIC id. Firmware tables are read at their
   physical addresses while paging is still off.
2. **Start, post-timer** (`smp_start_ap`, after `timer_start` and the shell process exist): map the LAPIC into every
   address space (`pg_map_lapic`, a hand-aligned per-CPU page table because GCC 1.40 ignores `__attribute__((aligned))`),
   calibrate the LAPIC timer against a 10 ms PIT channel-2 interval, install CPU 1's TSS (0x38) and its timer gate
   (vector 0x40), copy the real-mode trampoline to 0x9000, then INIT + SIPI.
3. **AP entry** (`smp_s.s`): the hand-encoded 16-bit trampoline (GAS 1.38 has no `.code16`) enables protected mode,
   far-jumps to 32-bit code, loads the kernel GDT/IDT, enables paging with the shared template directory, loads
   TR = 0x38, and calls `smp_ap_main`, which programs the LAPIC timer periodic and does `sti; hlt`. The first timer
   tick enters `_smp_ap_timer_stub` and the scheduler takes over on CPU 1.

CPU 1 gets its own idle job `IDL1` (pinned `aff = 1`), so the two CPUs never share an idle context.

## 5. Scheduling and affinity

The scheduler (`schedule`/`pick_next` in jobs.c) runs on either CPU under the BKL. Affinity is static — there is no
automatic migration:

* Each job has `aff` (0 or 1) and `running_on` (-1 when not executing, else the CPU it runs on).
* `pick_next` on CPU *c* considers only jobs with `aff == c` that are not already running on the other CPU. If none,
  it returns that CPU's idle job.
* On a switch, `schedule` sets the outgoing job's `running_on = -1` and the incoming job's `running_on = c` — the
  atomic ownership that guarantees a job is never dispatched on two CPUs at once.
* **Placement** is by the `QT_SMP` trap (`SMP_OP_SETAFF`): only a protected (CPL3) job may be placed on CPU 1 (a ring-0
  job is refused, `SMP_ERR_RING0`), and a job may move itself (the move takes effect at its next reschedule) but may
  not be moved while it runs on the *other* CPU. CPU 1's LAPIC timer then picks the newly-pinned job on its next tick;
  the job's saved CPL3 context resumes on CPU 1 exactly as KCTX-FIX2 resumes it on CPU 0.

CPU 1's timer runs `schedule_entry` → `schedule(sp, 1, 1)` with no timekeeping (no PIT ack, no wall clock, no poll
lists — those are CPU 0's); it only saves the current job and picks/resumes CPU 1's next.

## 6. Memory lifetime and address spaces

* The kernel is mapped **supervisor-only** in every process directory (unchanged from R12A); a CPL3 job touching any
  kernel page faults. Each job's private pages are user-accessible (`VAS_RW | VAS_US`).
* **CR3 per CPU**: each CPU activates the running job's directory (`pg_activate`) under the BKL; `g_curcr3[2]` tracks
  both. `pg_release_owner` checks both CPUs' active CR3 before freeing a directory.
* **Kill / exit lifetime**: a job is only ever *marked* dead (`job_mark_dead_other` / `job_kill_current`); its address
  space, stack, channels and heap are freed later by `reclaim_dead`, which **skips any job still `running_on` the other
  CPU**. So a job killed on CPU 0 while it runs on CPU 1 keeps its memory until CPU 1's next tick deschedules it
  (clearing `running_on`); only then is it reclaimed. No use-after-free, no job freed out from under a running CPU.
* The LAPIC page is mapped into the shared template, so both CPUs reach their local APIC at the same virtual address.

## 7. What is proven

Three staged gates, each frozen as a git tag:

* **r16-cpl3-proof** — single-CPU CPL3: every privileged/illegal instruction is contained as #GP/#PF, the job ends,
  the shell survives; bad user buffers are rejected by validation without faulting; preemption preserves CPL3 register
  state. 24/24.
* **r16-bkl-up** — the same 24/24 under BKL instrumentation on one CPU, with balanced acquire/release and no leaked
  owners.
* **r16-smp2-proof** — two ordinary CPL3 applications running at once on the two CPUs, each reporting the processor it
  runs on, their output interleaved; both exit cleanly; the BKL stays balanced with no leaked owners.

See docs/R16-SMP-RUNTIME-REPORT.md for the measured runs and docs/R16-KNOWN-LIMITATIONS.md for the boundaries of the
experiment.
