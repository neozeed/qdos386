# R16 — the CPL3 (protected process) ABI

Applies only to kernels built with `-DQDOS_R16` (`make qdos386-smp-pc`). Everything here is opt-in per launch;
an R16 kernel runs unmarked programs exactly as before (ring 0, CPU 0).

## 1. Opt-in launch policy (versioned, explicit — no reinterpretation of old binaries)

A process is launched protected (CPL3) when either:

* **Executable metadata**: bit 24 of the a.out magic word is set — `a_magic = 0x01000107` instead of `0x00000107`.
  `tools/r16-protect-mark.py FILE` sets or clears the bit and prints both hashes; the rest of the file is
  untouched. Frozen kernels reject such a file (`APP_ERR_MAGIC`), so a marked binary cannot be mistaken for an
  ordinary one; the R16 loader accepts both values and records the bit. Marked copies live only on R16 volumes.
* **Spawn flag**: `qd_spawn(..., flags | QD_SPAWN_PROT, ...)` (`PROC_LOAD_PROT` = 0x2 in the request word;
  refused with `APP_ERR_ARGS` by older kernels, which allow only the HELD bit).

Legacy launch (no mark, no flag) stays ring 0 and is valid only with affinity CPU 0; `QT_SMP SETAFF` refuses to
put a ring-0 job on CPU 1.

## 2. What runs differently at CPL3

| Aspect | Ring-0 process (unchanged) | Protected process |
| --- | --- | --- |
| CS / SS | 0x08 / 0x10 (CPL0) | 0x2B / 0x33 (CPL3, RPL3) |
| DS/ES/FS/GS | 0x10 | 0x33; **after any interrupt or trap they read as 0x33 again** — programs must not keep custom segment values (flat compiled code never does) |
| EFLAGS | whatever the job sets | IF=1 forced at launch; IOPL=0; `popf` cannot change IF/IOPL; AC usable |
| Kernel pages, frame pool, VGA, transfer area, boot stack | readable/writable (CPL0) | supervisor-only: any touch is #PF and ends the job |
| `cli hlt in out mov %crN lgdt lidt ltr` | execute | #GP, job ended, shell survives |
| `int n` (n ≠ 0x30) | enters the gate | #GP (all gates DPL0), job ended |
| `int 0x30` services | unchanged | unchanged (gate DPL3); same registers, same numbers, same buffer rules |
| interrupt frames | pushed on the user stack (stub switches to the kernel stack) | pushed by the CPU on the job's kernel stack (TSS.ESP0 = `kx_entry`); user ESP/SS live in the frame and are restored exactly by `iret`, any residue mod 4 |
| program start | `kx_call_user` near-call, initial `[esp]` = kernel return address | `iret` transition; initial ESP = `PROC_STACK_TOP-20`, `[esp]` = address of the exit stub at `PROC_STACK_TOP-16` (top 16 stack bytes reserved) |
| program end | `ret` from `_start` back into the kernel | `ret` from `_start` lands in the 11-byte user-mode exit stub: `mov %eax,%ebx; mov $0x18,%eax; int $0x30; jmp .` — `QT_EXIT` records the status and ends the job; the parent's `qd_wait` sees the same status as today |

Nothing else in the ABI changes: argc/argv via `QT_APP_ARGS`, the private heap, channels, consoles, surfaces,
spawn/wait/kill, environment — all identical, because every service was already reached through `int 0x30` and
every user buffer was already validated against the caller's own mapped ranges (`pg_user_range_ok`: image window,
args page, heap to `brk`, stack — all mapped by construction, so kernel copies cannot fault).

Audit of the shipped programs (greps over `sdk/`, `third_party/hack103-ql` port layer, BOUNCE, ALIGNT, CATFIL,
shell, QLTerm): no `in`/`out`, no `cli`/`hlt`, no segment loads, no `int` other than `0x30` — all candidates run
at CPL3 unmodified. (`SHUTDOWN.AOU` asks the kernel via trap; the kernel still does the I/O.)

## 3. Fault containment contract

A protected job that faults (vectors 0,1,3–7,9–14,16,17,19 with EIP outside the kernel) is ended by
`pg_exception` with the usual `PGFAULT:` report and "process ended"; other jobs, the shell and the kernel
continue. `qd_wait` for a faulted child returns a negative status (no `QT_EXIT` record). Privilege-escalation
attempts — far jumps/`iret` to DPL0 selectors, loading DPL0 selectors into segment registers, `int` to DPL0
gates, writes through kernel pointers smuggled into service buffers — end as contained #GP/#PF (the buffer rules
reject non-user ranges before the kernel touches them).

## 4. Kernel-internal notes (for the next builder)

* User descriptors: GDT 0x28 code `00cffa00`, 0x30 data `00cff200`; the AP TSS descriptor is 0x38.
* `schedule` writes TSS.ESP0 (`kx_entry`) for the incoming job on the dispatching CPU; SS0=0x10.
* The exit stub and the initial ESP are written into the (already user-mapped) stack page at build time
  (`vas_copy_to`), only for protected jobs.
* `proc.c` keeps a `prot` bit per process (from the magic bit or spawn flag); `proc_entry` branches to
  `kx_user_enter(entry, esp0)` in `kernel/kctx3.s` (segments → 0x33, `iret` with CS 0x2B/SS 0x33, EFLAGS 0x202).
* The frozen `kernel/kctx.s` is untouched; `kernel/kctx3.s` (R16 target only) adds: DS/ES := 0x10 after every
  `pushal`; on every return path whose frame CS has RPL 3, DS/ES/FS/GS := 0x33 before `popal; iret` (this covers
  `resume_dispatch` and the inline IRQ returns; `kernel/uart_s3.s` is the same two-line change for IRQ3/4).
