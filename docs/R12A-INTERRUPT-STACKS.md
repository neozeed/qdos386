# R12A kernel entry, stacks and CR3 switching

Applies to `QDOS_DYNPROC` kernels (`make r12a-pc`). Code: `kernel/kctx.s` (entry stubs, resume, `kx_call_user`, #DF
task), `kernel/jobs.c` (`schedule`, `kx_*` globals), `kernel/proc.c` (`pg_exception`, `proc_double_fault`, template
sections), `kernel/descriptors.c` (TSSs, task gate).

**Privilege.** Everything runs at CPL 0. There is no hardware stack switch on interrupts (that needs a privilege change),
so the switch below is done in software, explicitly, in every entry stub. None of this is a security boundary.

## 1. Stacks every job has

| Stack | Where | Mapped in | Used for |
|---|---|---|---|
| kernel stack | kernel-heap block, 16 KiB for processes (`PROC_KSTACK_BYTES`), canary at the base | every directory (template identity) | `job_trampoline → job_run → proc_entry`, every trap/IRQ/exception handler, the scheduler, `job_exit/yield_asm` |
| user stack | 64 KiB of private frames at `0x1FFF0000..0x20000000` | the process's directory only | the application's own code |
| boot stack | `0x7FFFF0` down (SYSTEM only) | every directory | the boot job |
| #DF stack | 4 KiB in kernel BSS | every directory | the double-fault task only |

Template jobs (SYSTEM, IDLE) have no private stack: their `kx_lo..kx_hi` is the whole address space, so they never switch
and behave exactly as in R12.

## 2. The entry rule

```
 every stub:   CPU pushes eflags, cs, eip (+ error code)  on the CURRENT stack
               pushal                                     on the CURRENT stack
               eax = esp                                  (address of the frame)
               if esp < kx_lo or esp >= kx_hi:  esp = kx_entry      ← software stack switch
               push eax; call C(frame)
 IRQ1/7/15:    pop esp (back to the frame); popal; iret   (these handlers never schedule)
 IRQ0/trap/exceptions: C returns saved_sp|fmt of the job to run → resume_dispatch: esp = that; popal; iret
```

* `kx_lo, kx_hi, kx_entry` describe the **current** job. `schedule()` reloads them from the job record every time it
  returns a job (whether or not it changed).
* `kx_entry` is set once, by `kx_call_user`, just below the kernel frames of `proc_entry` (they stay live while the
  application runs). Every entry from the user stack starts there; nothing is ever live below it at that moment because
  kernel code is not running (if it were, ESP would be inside the kernel stack and no switch would happen).
* The C handler reads and writes the frame (`f[]`) while the caller's directory is still active. The only places that
  change CR3 are the scheduler (`pg_activate`, after the outgoing frame is saved and before the incoming one is used) and
  template sections, which never touch a process's memory.
* The resume path sets ESP to the incoming job's saved frame **after** its directory is loaded, so a frame on a private
  user stack is mapped when it is popped.

## 3. Why nesting is bounded

* All gates are interrupt gates (IF=0 on entry) and no kernel code executes `sti` inside a trap or ISR (audited: only the
  two resume paths do). `yield_asm` is called only by `job_exit`, which runs on the kernel stack.
* So an interrupt can arrive only (a) in application code on the user stack → switch to `kx_entry`; or (b) in job-level
  kernel code on the kernel stack (`proc_entry` before the switch, `job_exit` before its `cli`) → no switch, the frame goes
  below the current ESP. A kernel stack therefore holds at most one interrupt frame on top of its job-level frames.
* `kx_call_user` brackets its own ESP change with `cli/sti`, so no interrupt sees a half-switched state.

## 4. CR3 switching

```
   job A (process, on its user stack)            timer IRQ
   ─────────────────────────────────────────▶   frame pushed on A's user stack (A's directory active)
                                                 stub: ESP := A.kx_entry (kernel heap, shared)
   sched_irq_tick → schedule(sp):                A.saved_sp := frame address (no dereference)
                                                 pick B; FPU save/restore; pg_activate(B): CR3 := B's directory
                                                 kx_* := B's
   resume_dispatch:                              ESP := B.saved_sp (B's user stack, mapped now) → popal, iret
```

The scheduler runs entirely on A's **kernel** stack, which every directory maps; that is the property R11K obtained by
forbidding private stacks and R12A obtains by switching stacks on entry.

## 5. Deferred writes and template sections

* Hold flags (QT_SUSPEND / QT_CON_WAIT) of a process are in its private memory. A release made while another directory is
  active (timer expiry in `pick_next`, IRQ1 console wake, another job's QT_REACTIVATE) is deferred and done by
  `pg_after_switch` right after the owner's directory is loaded.
* Pool memory (process directories, tables and frames) is touched only between `tpl_enter()` and `tpl_leave()`, on a
  kernel stack, with nothing of the caller's memory referenced inside (`proc_load`, heap growth, teardown, `QT_PG_INFO`
  page-table reads, `QT_PROC_VA`). `tpl_leave` reloads the caller's CR3, which also flushes its TLB after a mapping change
  (the 386 has no INVLPG).
* Blocking QT_PROC_WAIT never writes into the waiting parent later: the parent is woken and its call answers
  PROC_ERR_AGAIN, which libqdos repeats.

## 6. Faults

* Exceptions 0..19 (except 8) use the same stub. If the current job is a process, its directory is active and the EIP is
  outside the kernel image (0x100000 .. `bss_start`), the fault is the process's own: it is reported
  (`PGFAULT: vec err cr2 eip job cr3`, then the registers) and only that job ends (`job_kill_current`); its resources are
  reclaimed like any dead job's. Every other fault halts the machine with the same report: the kernel does not try to
  recover from its own faults.
* **User-stack overflow.** A push into the unmapped page below the user stack faults; the CPU cannot push the #PF frame on
  the same stack and raises #DF. Vector 8 is a **task gate** to `kx_tss_df` (own stack, template CR3). The CPU stores the
  interrupted state in `kx_tss_main` (loaded into TR at boot). `proc_double_fault` checks that the saved CR3 is the current
  process's and the saved ESP is at or below the bottom of its user stack (within one stack size); it then rewrites the
  saved EIP/ESP/EFLAGS/segments so that the task return (`iret`, NT set) resumes in `kx_df_kill` on the job's kernel stack
  with IF off, which ends the job and schedules. Any other #DF halts. The #DF task re-arms itself (`iret` is followed by a
  jump back to its entry).
* Not covered: an overflow of a **kernel** stack (16 KiB heap block, no guard page; the canary is checked on every
  switch-out and counted as `stack_faults`), and a fault inside the #DF handler itself (would be a triple fault).

## 7. Measured

The kernel-stack high-water mark of busy processes (HEAPT: heap churn, realloc, file reads, recursion, 59-98 timer
preemptions each, three copies at once) is **1,484 bytes** of the 16 KiB kernel stack (QT_PROC_INFO, Gate 2). Exercised
paths (QEMU): timer preemption on the user stack, every trap family, IRQ1 wake-ups of processes in QT_CON_WAIT, faults
0/6/14 from user code, #DF from a user-stack overflow, deferred hold-flag writes (2 × 300).
