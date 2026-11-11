# KCTX-FIX2 — Stage 0 audit: R14A against KCTX-FIX1, and every path that encodes a frame format

Branch `r14a-kctx-mainline-fix`, created from the frozen R14A source `7be7353` (R14A Hack: source `601ae33`, archive
`qdos386-R14A-HACK103.tar.gz` `49ac6ecc…`; R14 kernel base `43a1f448…`, archive `1dd86907…`) and merged with the
KCTX-FIX1 branch `kernel-context-fix` (`46a06e6`, package source `01a1b8c`, archive `9a2d0a25…`) as the reference.
All three archives were re-hashed and match (`logs/kctx2/stage0/inputs.txt`).

## 1. Did R14A already contain any part of the fix?

No. Diffing the actual sources of `7be7353` (not the release notes):

| File | R14A `7be7353` | KCTX-FIX1 `46a06e6` |
| --- | --- | --- |
| `kernel/kctx.s` | sha256 `5714e2db…`, `resume_dispatch`: `andl $1` / `andl $-4` | unchanged (still linked by R14 targets) |
| `kernel/kctx1.s` | absent | new: `movl %eax,%esp; cmpl $0,_kx_resume_fmt` |
| `kernel/jobs.h` | no `KX_RESUME`; sites return `sp \| 1u` | `KX_RESUME(sp, fmt)`, two forms under `-DQDOS_KCTX_FIX1` |
| `kernel/jobs.c` | 5× `return sp \| 1u;`, `schedule` returns `saved_sp \| saved_fmt` | `KX_RESUME` everywhere; `kx_resume_fmt` under the flag |
| `kernel/proc.c`, `kernel/paging.c` | one `return sp \| 1u;` each | `KX_RESUME(sp, 1)` |
| `Makefile` | `r14-pc` links `kctx.o` | + opt-in `kfix1-pc` (`kctx1.o`, `-DQDOS_KCTX_FIX1`) |

Build flags of the R14/R14A kernels: `R14_OPTS` (= R11M options, `QDOS_PAGING QDOS_DYNPROC QDOS_DYNPROC_SHELL QDOS_ENV
QDOS_TITLES QDOS_SYNC QDOS_TASKMGR QDOS_SURFACES QDOS_R13A QDOS_R13B QDOS_R14`), `PCCTX=…/kctx.o`. R14A changed no
kernel file (its additions are userland: Hack, `libc68.a`).

## 2. Which context object each target links (read from the Makefile and confirmed from the linked bytes)

| Targets | Context object | Status |
| --- | --- | --- |
| `pc` (R6), `pc-shell` (R7), R8/R9 shells, `r11h-pc`, `r11m-pc`, R11L/R11K non-paging | `kernel/context.s` (default `PCCTX`) | frozen, retired lines |
| `r11k-pc-paging`, `r11l-pc-fs-pg`, `r11m-pc-pg`, `r12-pc-pg` | `context.s` + `kernel/paging.s` (`pg_exc_common`) | frozen |
| all i386VM images (`i386vm`, `r11m-i386vm*`, R11H maps) | `kernel/context.s` (compiled in by `I386VM_SRC`) | frozen; i386VM retired |
| `r12a-pc` … `r14-pc`, `r13-pc-qdos5x9`, `r13b-epoch*` | `kernel/kctx.s` → now `kernel/frozen/kctx-r12a-r14.s` | frozen |
| **current**: `qdos386-pc` (QEMU VGA + Tiny386 headless) | **`kernel/kctx.s` (corrected)** | supported |

`tools/kctx2-resume-check.py` decodes `resume_dispatch` from each linked kernel: R12A, R12B, R12C, R13, R13A, R13B (VGA
and headless) and R14 (both) are `PTRBITS` (`movl %eax,%ecx; andl $1,%ecx; andl $-4,%eax`); `qdos386-vga.img` and
`qdos386.img` are `EXACT` (`movl %eax,%esp; cmpl $0,_kx_resume_fmt`) — `logs/kctx2/stage1/resume-convention.txt`.

**`context.s` and `paging.s`** contain the same `andl $-4` decode. They are reachable only from the frozen R6…R12
targets above (no QDOS_DYNPROC, no private stacks; their jobs run on kernel-allocated stacks that the kernel itself
keeps aligned, and none of those targets is shipped any more). They stay byte-identical (§5). No currently supported
target links them; the Makefile now also makes it impossible for a target to link them with the corrected C side
(§4), so a revived target would get the historical convention consistently, not a half-fixed one.

## 3. Every path into and out of `resume_dispatch` (current kernel)

Entries that reach `resume_dispatch` (all in `kernel/kctx.s`) and the C function whose return value it consumes:

| Entry | Gate / IF on entry | C function → return sites |
| --- | --- | --- |
| IRQ0 timer `_irq0_stub` | interrupt gate 0x8E: IF = 0 | `sched_irq_tick` → `schedule(sp, 1, 1)` |
| service trap `_trap_stub` (INT 0x30) | interrupt gate: IF = 0 | `trap_dispatch`: 5× `KX_RESUME(sp, 1)` (same frame), 7× `schedule(sp, 1, 0)` (blocking calls, exit, kill) |
| cooperative yield `_yield_asm` | `cli` first | `sched_yield_sp` → `schedule(sp, 0, 0)` |
| exceptions 0–19 except 8 `kx_exc_common` | interrupt gates | `pg_exception`: process fault → `job_kill_current` → `schedule`; otherwise `halt_with` (never returns) |
| #DF (vector 8) | task gate → `kx_df_entry` on its own TSS | `proc_double_fault`: user-stack overflow → main TSS rewritten to `kx_df_kill` with **EFLAGS 0x2**; else halt |
| `_kx_df_kill` | IF = 0 (from the TSS) | `proc_df_kill` → `job_kill_current` → `schedule` |
| start of a job | via `schedule` | `init_frame` stores `saved_fmt = 0` and an aligned frame; resumed by `resume_ctx` → `job_trampoline` |

`schedule()` is the only place that changes jobs; it saves `(sp, fmt)` of the outgoing job unmasked
(`out->saved_sp = sp; out->saved_fmt = fmt`) and returns `KX_RESUME(saved_sp, saved_fmt)` of the incoming job. No save
path masks or tags the pointer (grep of `& ~3`, `| 1u`, `andl $-4`, `$-2` over `kernel/`: only the frozen files).
IRQ1, IRQ7, IRQ15 and (R14) IRQ3/IRQ4 do not use `resume_dispatch`: they return with `popl %esp; popal; iret` from the
exact saved pointer and never schedule.

## 4. Is the single `kx_resume_fmt` variable safe here? (single CPU)

The value is written by the C function's return expression and read by the second instruction of `resume_dispatch`
(`movl %eax,%esp; cmpl $0,_kx_resume_fmt`). Nothing may run in between that writes it again:

* **Maskable interrupts**: every entry in §3 runs with IF = 0 (interrupt gates, `cli`, TSS EFLAGS 0x2). IF is set
  again only by `resume_ctx`'s `sti` and by `iret` — both after the read. Inside handlers the kernel uses only
  `irq_save()`/`irq_restore(flags)` pairs with local flags (taken with IF = 0, so `irq_restore` never sets IF);
  there is no other `sti`/`popf` in kernel C or assembly (`grep`: only the boot sectors and `kx_call_user`, which runs
  in job context before a job's first instruction, never in a handler).
* **Exceptions and NMI** in the window are kernel-mode faults: `pg_exception` halts on any fault whose EIP is in the
  kernel (and on NMI, vector 2); `proc_double_fault` halts unless the fault is a user-stack overflow of the current
  process. Nothing re-enters `resume_dispatch` from the window and returns into it.
* **Other CPUs**: none. This kernel brings up one CPU; SMP/AMP would need a per-CPU variable (or an EDX-carried
  format) — recorded in `docs/KCTX-FIX2-KNOWN-LIMITATIONS.md`.

The format is per transition, not per job: a trap returning to its own frame is format 1 whatever that job's last saved
format was, so a per-job field read in `resume_dispatch` would be wrong; the global written at the return is right.
The claim is also checked at run time by a test-only kernel (`make qdos386-check-pc`, `kx_resume_check`): IF clear at
every write and the frame shape matching the format, over the misaligned probe and GW-BASIC workloads
(`docs/KCTX-FIX2-RUNTIME-REPORT.md` §2).

## 5. What exactly has to change (Gate 0)

* `resume_dispatch` must load ESP from the returned value unchanged, and the format must come from somewhere other
  than the pointer — the KCTX-FIX1 mechanism. A one-line `andl $-2` ("Option A") is **not** a fix: bit 0 is then
  still both the format and part of the pointer, so residues 1 and 3 are still resumed wrongly.
* The C return sites (`trap_dispatch` ×5, `schedule`, `pg_exception`, `proc.c`/`paging.c` one each) must hand over
  the full pointer and set the format — `KX_RESUME`.
* The corrected path must be the source default and the default build (no opt-in flag), the frozen targets must keep
  their bytes, and the C side and the linked assembly must never disagree. Implementation:
  `docs/KCTX-FIX2-IMPLEMENTATION.md`.
