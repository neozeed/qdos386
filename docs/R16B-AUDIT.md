# R16B — Stage 0 audit: job ownership, affinity and focus (baseline = R16A)

Baseline: branch `r16b-foreground-smp` from tag `r16a-mbr-cpu-taskmanager` (`d4827f3`). R16A package
`qdos386-R16A-MBR-CPU-TASKMANAGER.tar.gz` sha256 `a81b267b…`. In this branch `make qdos386-smp-pc` reproduces the
R16A kernel exactly (`qdos386-smp-vga.img` = `b9ecb69a…`); `make qdos386-pc` = `f8e5d5e1…` / `7c675978…`. The new
opt-in target `make qdos386-r16b-pc` (R16 + `-DQDOS_R16B`, `build/qdos386-r16b/`) is where every R16B change lives.

Gate 0 (logs `logs/r16b/gate0-*`): `-smp 1` R16 Gate 1 24/24; `-smp 2` boot from an MBR-partitioned FAT16 `hdb`,
`R16T RACE` both children exit 0, `badrel=0`, Task Manager CPU column correct. The AP-idle slot fix
(`smp.c`, `ap_idle &= 0xffff`) is present.

## Ownership fields (struct job, R16/R16A)

| Field | Meaning | Written by |
| --- | --- | --- |
| `aff` | the only CPU whose `pick_next` may select the job | `job_alloc_sized` (creator's CPU), `QT_SMP SETAFF`, boot (SYSTEM 0, IDL1 1) |
| `running_on` | −1 not executing, else the CPU executing it | `schedule()` on switch, under the BKL; `smp_ap_main` for IDL1 |
| `last_cpu` | annotation: last CPU it was dispatched on | `schedule()` beside `running_on` (R16A) |

All three are read and written only under the BKL (every kernel entry holds it; `schedule()` always runs inside one).

## State transitions (per job, per CPU *c*)

```
            pick_next(c): aff==c && running_on==-1 && runnable
   READY ─────────────────────────────────────────────────────▶ RUNNING on c
 (running_on=-1)        schedule(c): running_on := c, last_cpu := c,     (running_on=c)
     ▲                   fpu_restore, pg_activate (CR3), kx_pc[c], TSS.ESP0[c]
     │                                                                │
     │   schedule(c) switching away: saved_sp/fmt := frame (at entry),│
     └───────────── fpu_save, running_on := -1  ◀─────────────────────┘
                    (also when the job blocks: status<0, or dies: dead=1)
   dead=1 + running_on==-1  ──▶ reclaim_dead() may free (skips running_on != -1)
```

`running_on := -1` is written only after the frame and the x87 state are saved, and the other CPU can only reach
`pick_next` by taking the BKL, which the source CPU still holds until `resume_dispatch`. So the hand-off is atomic:
a job's frame is never observed half-saved, and a job is never selectable by a second CPU while one CPU owns it.

## Findings that shape R16B

1. **Migration is already deferred by construction.** Changing `aff` does not touch a running job: the owning CPU keeps
   running it until its next `schedule()` pass (≤ one timer tick, or sooner at a blocking syscall), where `pick_next`
   no longer selects it, the frame and FPU are saved and `running_on` becomes −1; only then can the other CPU pick it.
   All per-CPU state (CR3, `kx_pc[c]`, TSS.ESP0) is re-derived on the destination at dispatch. R16's `AFFWORK` already
   self-migrates CPU0→CPU1 this way.
2. **SETAFF today:** a job may move itself, or a job not currently running; a job running on the *other* CPU is refused
   (`SMP_ERR_STATE`). Only CPU 1 requires a protected job; **CPU 0 has no check**, so `SETAFF(IDL1, 0)` would put the
   CPU-1 idle job on CPU 0 while CPU 1's idle fallback (`best = job_idle`, which does not check `running_on`) could
   still return it — a potential double-run. R16B must allow affinity changes only for live CPL3 jobs, never kernel or
   idle jobs.
3. **No stale per-CPU index survives a migration.** Entry stubs keep the CPU's `kx_pc` offset in EBP, and a resumed
   frame always returns through the stub of the CPU now executing it. The only cooperative (format-0, C-frame) yield
   is `job_exit` → `yield_asm`, whose frame is never resumed; blocking syscalls save the trap frame and resume by
   `iret`. A future kernel path that blocks inside C and resumes would have to re-read `kx_this_cpu()`.
4. **TLB.** CR4 = 0 (no global pages) and `pg_activate` reloads CR3 on every switch, so a CPU can hold translations
   for a job's directory only while that job is its current job. A migrated job's mapping changes made on CPU *a* are
   never stale on CPU *b*, because *b* reloaded CR3 when the job left it. No shootdown is needed under "never on two
   CPUs at once"; `pg_release_owner` already checks both CPUs' active CR3.
5. **Focus.** Every focus change goes through `vcon_switch()` (Alt+Fn in the keyboard IRQ, the channel control, the
   surface layer, the Task Manager). `vcon_fg()` is the foreground session; a job's session is `vcon_job_con(slot)`.
   The Task Manager overlay leaves `fg` at the underlying session.
6. **Accounting hooks.** CPU 0's tick is `sched_irq_tick(sp)` (IRQ0), CPU 1's is `smp_ap_timer_resume(sp)` (LAPIC);
   both run under the BKL with the interrupted frame at `sp` (CS at `f[9]`).
