# KCTX-FIX1 — independent audit of the scheduler context fault, and the corrective kernel

Branch `kernel-context-fix` from the frozen R14A tree (`7be7353`, the Hack milestone; R14 source `43a1f448`).
Inputs (hashes in `third_party/kctx-evidence/inputs-SHA256SUMS`): the GW-BASIC builder's `FINDINGS-stack-corruption.md`,
`failing-checkpoint.zip` (GWBASIC.AOU `4ae47943…`, launch volume `ed03cd27…`, kernel image `0199a72b…`) and the GW-BASIC
source/reproducer archive. The checkpoint's kernel image is byte-identical to the frozen R13B QEMU kernel
(`logs/r14/stage0/baseline-hashes.txt`). No supplied program was run outside the emulated guest; GW-BASIC was not changed.

## 1. Which context code each kernel links

| Kernels | Context object | Resume decode |
| --- | --- | --- |
| R7 … R11 default images, i386VM | `kernel/context.s` (PCCTX default) | `andl $-4` (line 48) |
| R11J … R12 paging targets | `kernel/paging.s` (+ context.s) | `andl $-4` (line 48, `pg_exc_common`) |
| R12A, R12B, R12C, R13, R13A, R13B, **R14 / R14A (current)** | `kernel/kctx.s` (PCCTX=…/kctx.o) | `andl $-4` (line 46) |
| **KFIX1** (`make kfix1-pc`, new) | `kernel/kctx1.s` + `-DQDOS_KCTX_FIX1` | none: full ESP, format in `kx_resume_fmt` |

`kernel/kctx.s` has the hash the findings quote (`5714e2db…`) and has not changed since R12A (`a9eaca9`): the current
R14/R14A kernels contain the same code as the failing R13B image. **The defect was not corrected earlier.**

## 2. All paths that leave the kernel (audited)

* Through `resume_dispatch` (the decode): IRQ0 (`sched_irq_tick` → `schedule`), the service trap (`trap_dispatch`:
  five `return sp | 1u` sites, `return schedule(…)` for blocking calls), exceptions 0–19 (`pg_exception`, including
  `job_kill_current`), the #DF recovery (`kx_df_kill` → `proc_df_kill`), the cooperative yield (`yield_asm` →
  `sched_yield_sp`), the initial frame of a job (`init_frame`, format 0, via `resume_ctx` to `job_trampoline`).
* Not through it: IRQ1, IRQ7, IRQ15 and (R14) IRQ3/IRQ4 return with `popl %esp; popal; iret` — the exact saved
  pointer, no decode, correct for any ESP.
* Kernel-stack switching (`kx_lo`/`kx_hi`/`kx_entry`), the stack canaries (at `stack_base`, not at ESP) and the
  accounting do not depend on the alignment of ESP.

## 3. Is 4-byte stack alignment part of the application ABI?

No. The only statement is an implementation comment in `context.s` (“Stacks are 4-byte aligned, so bits 0-1 are
free”); the application ABI documents (`docs/R8-APP-ABI.md`, `docs/R12A-*`) require nothing about ESP alignment. The
kernel runs programs with CR0.AM clear (CR0 = 0x80000011), so i386 code may use any ESP. A job interrupted with
ESP ≢ 0 mod 4 is therefore legal, and the kernel **silently resumes it from a shifted frame** — a kernel defect, not
an application ABI violation. (Even if alignment were made a rule, the kernel would have to refuse it explicitly.)

## 4. Independent reproduction on the current (R14) kernel — `logs/kctx/stage1/`

BASIC-independent probe `ALIGNT residue loops mode` (`tests/kctx/alignt.c`, `kalign.s`): holds ESP at the residue
while spinning (timer preemption, mode 0), making service traps (mode 1) or sleeping a tick (mode 2), with patterns
in EBX/ESI/EDI/EBP checked every iteration; plus the supplied `spin_aligned` / `spin_misaligned` assembled unchanged.

| Test (R14 VGA kernel `8bfd4d58…`) | Result |
| --- | --- |
| `spin_aligned` (supplied control) | exit 0 |
| ALIGNT residue 0: spin (7 ticks), trap ×200000, yield ×300 | OK, OK, OK |
| `spin_misaligned` (supplied) | `#DF double fault`, kernel halt |
| ALIGNT residue 2 spin / trap / yield | `#DF`, halt (each) |
| ALIGNT residue 1 spin | `#DF`, halt |
| ALIGNT residue 3 spin | `#GP` at the `iret` of `resume_dispatch` (eip 0x109251), halt |

First divergence, traced with gdb on QEMU's stub (`tests/kctx/trace-run.sh`, `logs/kctx/stage1/trace.gdb.txt`),
ALIGNT residue 2, service-trap path, CR3 0x241000:

```
TRAP-ENTRY esp=1fffff2e eip(pushed)=00245677 cs=00000008 eflags=00000246
RESUME eax(encoded)=1fffff0f  fmt(bit0)=1  decoded esp (andl $-4)=1fffff0c  frame the job saved at 1fffff0e
words popal/iret would take at 1fffff0c: 3c3c0000 a5a53c3c ff52a5a5 ff2e1fff 00001fff 00000000 00000000 0e4c0000
                                          56770000 00080024 02460000           <- EIP 0x56770000, CS 0x0024
the frame really saved at 1fffff0e:      3c3c3c3c a5a5a5a5 1fffff52 1fffff2e 00000000 00000000 00000000 00000e4c
                                          00245677 00000008 00000246
```

The decode loses bit 1 of the saved pointer; `iret` then loads CS = 0x0024 (the high half of the pushed CS word and
the low half of EIP), which is the `IP=0024:…` of the GW-BASIC report, and the CPU double-faults. It happens on the
**first** kernel entry with a misaligned ESP — a timer tick, a service trap or a yield — not only on preemption.
For odd residues bit 0 (the format) is also the pointer's own bit, so “Option A” (`andl $-2`) would still resume
residues 1 and 3 wrongly; the findings' precondition (even ESP) would remain a silent hazard.

## 5. The fix — `kernel/kctx1.s`, `-DQDOS_KCTX_FIX1`

* Every C function that returns to `resume_dispatch` returns the **full** saved stack pointer and stores the frame
  format in `kx_resume_fmt` (`KX_RESUME(sp, fmt)` in `kernel/jobs.h`; 5 sites in `trap_dispatch`, `schedule`,
  `pg_exception` in `proc.c` and `paging.c`). `resume_dispatch` is `movl %eax,%esp; cmpl $0,_kx_resume_fmt; je
  resume_ctx; popal; iret`.
* Why a kernel global is safe here: one CPU; every entry is an interrupt gate (IF cleared) or `yield_asm`'s `cli`;
  handlers only use `irq_save`/`irq_restore` pairs, which keep IF off inside an interrupt; the value is written by the
  C function immediately before its `ret` and read by the next instruction after `movl %eax,%esp` — nothing can run
  in between. The format belongs to the transition, not to a job (a trap returning to the same frame has format 1
  whatever the job's last saved format was), so a per-job field would be wrong.
* Without `-DQDOS_KCTX_FIX1`, `KX_RESUME(sp, fmt)` is `(sp) | (fmt)`: the frozen kernels keep `kctx.s` and build
  byte-identical (`tests/r14/check-frozen.sh`: 25/25; R14 kernels and R14 binaries: see §6).
* `context.s` and `paging.s` have the same `andl $-4` but are linked only into frozen legacy images; they are left
  unchanged (changing them would change those frozen images). Any revived target that uses them needs the same change.
* All residues 0–3 are supported; nothing is rejected or rounded.

## 6. Verification

(see `docs/KCTX-FIX1-REPORT.md`)
