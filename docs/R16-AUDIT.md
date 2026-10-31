# R16 — Stage 0 feasibility audit (ring 3, BKL, two-CPU scheduling)

Branch `r16-ring3-bkl-smp` from frozen R15 AMP0 (`7651795`, tag `r15-amp0`, archive `35e82587…`; underlying
KCTX-FIX2 `778a3d61…`). Everything below was read from the sources named; nothing is assumed from release notes.
The R16 kernel is a separate opt-in target (`make qdos386-smp-pc`, `-DQDOS_R16`, context file `kernel/kctx3.s`);
the ordinary `make` kernels, R15 AMP0 and every frozen image keep their bytes.

## 1. Kernel entry inventory (every way the CPU enters kernel code)

| Entry | Vector / caller | Stub | C handler | Exit path |
| --- | --- | --- | --- | --- |
| timer | 0x20 (PIT IRQ0, PIC → CPU0) | `irq0_stub` | `sched_irq_tick` → `schedule` | `resume_dispatch` |
| service trap | 0x30 `int` | `trap_stub` | `trap_dispatch` | `resume_dispatch` (5 same-frame returns, blocking calls via `schedule`) |
| keyboard | 0x21 (IRQ1) | `irq1_stub` | `input_irq1` | inline `popl %esp; popal; iret` |
| spurious | 0x27 / 0x2F | `irq7_stub` / `irq15_stub` | `irq7_dispatch` / `irq15_dispatch` | inline |
| serial | 0x23 / 0x24 (R14) | `irq3_stub` / `irq4_stub` (`kernel/uart_s.s`) | `uart_irq` | inline |
| exceptions 0–19 (not 8) | CPU | `kx_exc0..19` → `kx_exc_common` | `pg_exception` | `resume_dispatch` or `halt_with` |
| #DF | task gate 0x08, TSS 0x20 | `kx_df_entry` (own TSS/stack) | `proc_double_fault` (+ `kx_df_kill` → `proc_df_kill`) | task return / `resume_dispatch` |
| cooperative yield | call from kernel-mode job | `yield_asm` | `sched_yield_sp` → `schedule` | `resume_dispatch` |
| job start | `schedule` resumes fmt-0 frame | `resume_ctx` → `job_trampoline` | `job_run` → `proc_entry` → `kx_call_user` | — |

Direct shared-kernel calls from kernel-mode jobs (not through a gate): the SYSTEM job's boot loop calls
`surface_poll()` and `taskmgr_poll()` directly (`kernel/shellboot.c`; the task manager runs inside the SYSTEM job,
`tm_slot = JOB_SYSTEM`); `boot_idle` is an empty spin and calls nothing. Everything else from jobs goes through
`int 0x30`. These two call sites are the only ones that must take the BKL explicitly (Stage 2).

## 2. Globals tied to "the current CPU" (must be per-CPU before an AP schedules)

From `kernel/jobs.c`, `kernel/proc.c`, `kernel/kctx.s`:

| Global | Where | Role |
| --- | --- | --- |
| `job_current` | jobs.c, read everywhere | the running slot |
| `kx_lo`, `kx_hi`, `kx_entry` | jobs.c, **read by the entry stubs in assembly**, written by `schedule` | the running job's kernel-entry stack |
| `kx_resume_fmt` | jobs.c (KCTX-FIX2), read by `resume_dispatch` | frame format of the pending resume |
| `cur_cr3` | proc.c | the directory loaded on this CPU (`tpl_enter`/`tpl_leave`, `pg_activate`, release guard) |
| `exc_depth` | proc.c | nested-exception guard |
| `job_ctx_override` | jobs.c (`JOB_CTX_SLOT`) | launch-context override |
| TR / TSS (`kx_tss_main`) | descriptors.c | per-CPU by definition; also the #DF outgoing state |
| FPU hardware state | fpu.c (eager FNSAVE/FRSTOR in `schedule`) | per-CPU hardware; correct with pinned jobs |

Shared state that stays shared **under the BKL**: `jobs[]`, `procs[]`, `exit_rec[]`, child records, `dead_mask`,
`sv_pollm`, poll/schedule task lists, frame pool + page tables (template sections), kernel heap, channels, FAT,
vcon/surfaces/kev, env blocks, UART, counters. The scheduler's wakeup countdown consumes `sv_pollm` under the BKL,
so timeouts decrement once regardless of which CPU's scheduler pass runs.

## 3. What protection already exists (and what is missing) for CPL 3

Already in place (R12A dynamic processes, read from `proc.c`/`vas.c`):

* Process private pages (image, args page, heap, stack) are mapped `VAS_RW | VAS_US` — user at PTE level; the
  template's identity PTEs have no U/S bit, so **kernel, frame pool, transfer area, boot stack and VGA are
  supervisor-only in every process directory already**. Page 0 is hidden (NULL faults).
* `pg_user_range_ok` admits only the caller's image window, args page, heap `[base, brk)` and stack — all of which
  are mapped-present by construction (heap only up to `brk`), so a validated copy can never fault in kernel mode;
  `chan_user_range_ok` uses it for every paged caller and never admits kernel memory to a paged job.
* `pg_exception` already ends (only) the faulting process for vectors 0,1,3–7,9–14,16,17,19 when EIP is outside the
  kernel and the process's own directory is active; the #DF task gate covers ring-0 user-stack overflow (at CPL3 a
  stack overflow is an ordinary #PF on the guard, handled the same way).
* Eager per-switch x87 save/restore in `schedule` (`fpu_save`/`fpu_restore`).

Missing for CPL3 (Stage 1 work):

1. **GDT**: only 0x08/0x10 (DPL0) + TSS 0x18 (main, in TR) + 0x20 (#DF). Add user code 0x28 (sel 0x2B) and user
   data 0x30 (sel 0x33); later the AP TSS at 0x38.
2. **Gate DPL**: every IDT gate is type 0x8E (DPL0). `int 0x30` from CPL3 needs DPL3 (0xEE); all other vectors stay
   DPL0 so a user `int n` gets #GP and ends the job.
3. **TSS.SS0/ESP0** are never written (no privilege change ever happens today). `schedule` must set
   ESP0 = the incoming job's `kx_entry` (SS0 = 0x10 once). The entry stubs' software stack switch then never
   triggers for CPL3 entries (the CPU is already inside `[kx_lo, kx_hi)`).
4. **Frame layout** is compatible: a CPL3 entry pushes SS:ESP ahead of EFLAGS/CS/EIP; all handler offsets
   (`f[0..7]` pushal, `f[8]` vector, `f[10]` EIP) are unchanged, and `popal; iret` restores the user ESP/SS
   automatically, at any ESP residue. `saved_sp` for a CPL3 job points into its kernel stack instead of its user
   stack; nothing reads it except the resume.
5. **Segment registers**: the kernel never reloads DS/ES today (they have been 0x10 since boot, and no other value
   can occur). At CPL3 a job can leave DS/ES/FS/GS null or user-valued; a kernel entry with DS=null would #GP
   inside the kernel. Every R16 stub therefore loads DS/ES=0x10 after `pushal`, and every return to a frame whose
   CS has RPL 3 loads DS/ES/FS/GS=0x33 before `popal; iret` (an `iret` to CPL3 nulls any DPL0 data segment, so
   they cannot simply be left). Consequence, documented in the ABI: CPL3 segment registers read as the flat user
   selectors after any interrupt; flat C programs never notice.
6. **Launch and exit**: `kx_call_user` near-calls the image and the program *returns* to the kernel
   (`crt0` → `qd_end_job` → `ret` from `_start`); `QT_EXIT` only records `exit_rec[]`. At CPL3 there is no return
   path, so the kernel plants an 11-byte exit stub at the top of the user stack
   (`mov %eax,%ebx; mov $QT_EXIT,%eax; int 0x30; jmp .`), points `_start`'s return address at it, and `QT_EXIT`
   from a protected job records the status and ends the job (`job_kill_current` path); the waiting parent then
   reads the recorded status exactly as today (`exit_rec` is checked before the killed/no-status cases in
   `job_exit_status`).
7. **EFLAGS**: user frames get IF=1, IOPL=0; `CLI`/`STI`/`HLT`/`IN`/`OUT`/`MOV CRn` and DPL0 gates all #GP and the
   job is ended by the existing `pg_exception` path. `popf` at CPL3 cannot set IF or IOPL (silently masked);
   EFLAGS.AC is user-settable, so the existing ALIGNT register/EFLAGS checks run unchanged at CPL3.

What an unchanged `.AOU` does at startup (read from `sdk/lib/crt0_libc.s`): records `qd_entry_frame`, sets the
program title and reads argc/argv by trap, calls `main`, funnels every exit (including `exit()`) through a single
`ret` from `_start`. It touches no segment register and no port; all services are `int 0x30`. The same holds for
the SDK programs used in the gates (BOUNCE, ALIGNT, CATFIL, Hack via `libc68`): no `in/out`, no `cli`, no far
calls — so they can run at CPL3 unmodified (the audit greps are in the ABI doc).

## 4. Memory limit audit (for `-smp 2 -m 32`)

`proc_boot` caps `ram_top` at `FR_LIMIT` = 16 MiB — the extent of the identity map (4 template tables). `-m 32`
therefore changes nothing physical for the kernel (same 3,264-frame pool); the ACPI tables that SMP discovery
reads sit above 16 MiB and are read before paging is enabled, exactly as R15 AMP0 does. R16 keeps the 16 MiB map
(extending it is unrelated risk) and runs QEMU with `-m 32`. One addition: the template gains a supervisor, PCD
mapping of the local APIC page (virtual = physical 0xFEE00000, PDE 0x3FB, a dedicated kernel-BSS page table) so
that the AP can program its timer and EOI with paging on; processes inherit it supervisor-only.

## 5. SMP plan grounded in the sources (Stage 3)

* Discovery + INIT/SIPI: the R15 `amp.c` sequence (MADT→MP fallback, checksums, ICR delivery polls, PIT ch.2
  delays, bounded READY wait) is reused as a copy in `kernel/smp.c` — `amp.c` itself is left byte-identical for
  the R15 target. Split: `smp_boot()` (pre-paging: detect CPUs, calibrate the LAPIC timer against PIT ch.2) and
  `smp_start_ap()` (after `pg_init` + `timer_start`: INIT/SIPI through the now-mapped LAPIC; the AP enables
  paging itself with the template CR3).
* The AP is **not** the AMP worker: it loads the kernel GDT/IDT, its own TSS (0x38) and 16 KiB kernel stack,
  enables paging, `fninit`, programs its LAPIC timer (vector 0x40, periodic, PIT-calibrated) and enters the
  scheduler by resuming its own idle job under the BKL. External IRQs stay on CPU0 (PIC); the AP takes only its
  LAPIC timer and exceptions.
* `str`-based CPU indexing: TR is 0x18 on CPU0 (and 0x20 only inside the #DF task, which maps to index 0) and
  0x38 on CPU1, so the stubs and the C accessor derive the per-CPU block from TR with no fragile globals.
* Job placement: `aff` (0/1) and `running_on` (−1/0/1) fields under `QDOS_R16`; `pick_next` filters
  `aff == cpu && (running_on == −1 || slot == current)`; per-CPU idle jobs; no migration. `reclaim_dead` skips
  jobs with `running_on != −1` (a KILLed job running in user mode on the other CPU is reclaimed only after that
  CPU's next scheduler pass saves it off), and `pg_release_owner` refuses while the directory is loaded on either
  CPU — CR3 loads flush the whole TLB (no global pages are used, CR4 is never touched), so after both checks pass
  there are no stale translations and no shootdown IPI is needed. Heap growth and hold-flag writes always run on
  the CPU that owns the affected directory state under the BKL (growth in the owner's own trap; hold writes on a
  blocked — not running — job).
* CPU1 scheduler passes skip timekeeping: no `sched_ticks`/`clock_tick`/polls/`RUN_TASKS` (CPU0's PIT remains the
  timebase and device poller); they only save/pick/resume.

## 6. Risks accepted and declared up front

* A kernel-mode fault on either CPU halts that CPU while it may hold the BKL: the system fail-stops (honest,
  reported in KNOWN-LIMITATIONS). User faults are contained per process on both CPUs.
* The #DF task gate and its TSS are CPU0 state; a #DF on CPU1 (kernel bug; user stack overflow is #PF at CPL3)
  is a fail-stop.
* Everything still trusts ring-0 *kernel* code; CPL3 is protection for processes, not between the kernels.
* Legacy ring-0 process launch stays available, CPU0 only; CPL3 is opt-in per launch (ABI doc §1).
