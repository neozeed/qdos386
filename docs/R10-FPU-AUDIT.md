# R10 FPU audit (Phase 0)

Status: Phase 0 only. No kernel, harness or libc changes are made in this
document's round. Source-reviewed conclusions are marked as such; nothing here
is guest-tested yet.

## 0. Baseline

R9 was checked before any R10 work. The R9 package (`qdos386-R9.tar.gz`,
SHA-256 `b6ea266d…`) was extracted cleanly, all 352 files matched the manifest,
and the R9 volume and shell image rebuilt byte-identically from the extracted
source. The R9 sessions (29/29 on both targets) and kernel suites (211 CHECK,
0 FAIL) were rerun from the extracted package. R9 is preserved unchanged on
branch `r9` at `ee38a8b`.

## 1. Sources and hashes

Two Tiny386 lineages are present. They are not the same revision.

| Role | File | SHA-256 (first 16) |
|---|---|---|
| PC emulator (working) | `tiny-build/fpu.c` | `da5dc17d…` |
| PC emulator (working) | `tiny-build/i386.c` | `e0febcbb…` |
| PC emulator (working) | `tiny-build/i386.h` | `9703ef6a…` |
| PC emulator (working) | `tiny-build/fpu.h` | `2f489bf2…` |
| PC binary | `tiny-build/tiny386_headless` | `fb63b405…` |
| Standalone i386VM core | `x/i386vm-r7/emulator-tiny386/fpu.c` | `32345123…` |
| Standalone i386VM core | `x/i386vm-r7/emulator-tiny386/i386.c` | `17bf8cc6…` |
| Standalone i386VM core | `x/i386vm-r7/emulator-tiny386/i386.h` | `859140de…` |
| Standalone i386VM core | `x/i386vm-r7/emulator-tiny386/fpu.h` | `cd429154…` |
| Standalone i386VM R9 harness | `x/i386vm-r9/i386vm-r9` | `a4724f79…` |
| Toolchain | `x/i386vm/i386vm/bin/i386-gcc`, `as`, `i386-ld` | `3567f3be…`, `47369dd5…`, `dfdb3965…` |

`x/tiny386full/tiny386-master` is byte-identical to `tiny-build` for the
FPU and CPU files. The PC tree is therefore the lineage that the plan names as
the reference.

## 2. Differences between the two cores (CR/LF-insensitive)

Line counts from `diff --strip-trailing-cr`:

- `fpu.c`: 131 changed lines.
- `i386.c`: 254 changed lines.
- `i386.h`: 20. `fpu.h`: 8.

Findings:

1. **The standalone FPU is an older revision.** The PC `fpu.c` has the exact
   64-bit integer path (`fppushi64`, `fpgeti64`), used by FILD m64 and FISTP
   m64. The standalone copy does not have it, so FILD/FISTP of 64-bit values
   can lose low bits there. The standalone copy adds `fpu_state_size`,
   `fpu_save_state` and `fpu_load_state`, which the PC copy does not have.
2. **The standalone CPU core is also older.** The PC `i386.c` clears
   `cc.mask` on task switch (fixes stale lazy flags), passes the task-switch
   error code on the new stack, and has a different memory-mapped I/O rule.
   The standalone `in_iomem()` is `addr >= 0x800000` (the R9 device page
   rule). The PC rule is the VGA range or `addr >= 0xe0000000`. This is the
   only core difference that must stay in the standalone build.
3. **The standalone harness adds `cpui386_reset_pm_flat` and a guest-exception
   print that exits.** These are harness code and stay.

## 3. Does the standalone harness initialise the FPU?

Yes, by source review. The standalone core:

- defines `I386_ENABLE_FPU` in the build (R9 build flags);
- creates the FPU on reset with `cpu->fpu = fpu_new()` when that define is set;
- sets `CR0 = 0x10` (ET) when an FPU is present, so the reset state has
  EM = 0 and TS = 0;
- dispatches ESC opcodes (`0xd8`..`0xdf`) to `fpu_exec1`/`fpu_exec2`;
- raises `#NM` for `CR0.EM|TS` on ESC and for `CR0.MP&TS` on WAIT, as the PC core does.

So the standalone harness is **not** missing FPU initialisation, CR0 bits,
dispatch or the #NM path. What it lacks is the newer FPU revision (item 1) and
the newer CPU fixes (item 2). Phase 1 should replace the standalone `fpu.c`
with the PC revision, keep the standalone `i386.c` (for the iomem rule), and
port only the CPU fixes that the FPU depends on. The task-switch fix is not
FPU-related and is not ported in R10.

## 4. What the x87 implementation models (PC `fpu.c`)

Implemented (source-reviewed): FADD/FMUL/FCOM/FCOMP/FSUB/FSUBR/FDIV/FDIVR
(all forms); FLD/FST/FSTP for float32, float64, float80 (load/store of
float80 is a 10-byte conversion); FILD/FIST/FISTP/FISTTP for int16, int32, int64;
FBLD/FBSTP; FLDCW/FNSTCW; FNSTSW; FNSTENV/FLDENV; FNSAVE/FRSTOR; FNINIT,
FNCLEX; FCHS, FABS, FTST, FXAM; FLD1, FLDZ, FLDPI and the other constants;
F2XM1, FYL2X, FYL2XP1, FPTAN, FPATAN, FXTRACT, FPREM, FPREM1, FSQRT, FSINCOS,
FRNDINT, FSCALE, FSIN, FCOS; FCMOV; FUCOMI/FCOMI; FCOMPP/FUCOMPP; FFREE;
FXCH; FDECSTP/FINCSTP; FNOP.

Limitations stated in the source and found by review:

- **No exceptions.** The header says "no exception". Invalid, zero-divide,
  overflow and precision exceptions are not raised, and CW exception masks
  have no effect.
- **No tag word.** `fpu.c` line 147 is marked `TODO: tag word`. Tag bits exist
  only for the empty/valid distinction used by FILD/FISTP. Tags are not
  reported by FNSTENV/FNSAVE as the hardware does.
- **Internal precision is double.** Arithmetic is done in C `double` (53-bit
  mantissa). float80 values are converted to and from double. Extended
  precision results, and any result that depends on 64-bit mantissa bits, are
  not reproduced.
- **Precision control is ignored; rounding control is modelled.** `fpround()`
  (line 413) applies the RC field to conversions and some stores. Precision
  control (PC bits) is not modelled.
- **FCOMI** is marked `TODO: raise IA` and does not raise invalid-operation for
  NaN operands.
- Transcendental operations use libm doubles. Their results are not bit-exact
  with the 387.

Consequence for R10: the emulator arithmetic is good enough for correct
double-precision programs whose results are checked within tolerance, but it
cannot be used as a proof of 387 IEEE conformance. R10 claims must say this.

## 5. Kernel state today

- The kernel has **no FPU state handling**. There is no `FNSAVE`/`FRSTOR` in
  `kernel/` or `platform/`, no `FNINIT` at boot, and `struct job`
  (`kernel/jobs.h`) has no x87 storage.
- The job switch is in `kernel/context.s` (not yet audited line by line in this
  round). Phase 2 has to save and restore x87 state at every switch point
  listed in the plan.
- Because nothing saves x87 state, two jobs that both use the FPU today would
  corrupt each other's stack. R10 must prove the fix, not assume it.

## 6. GCC 1.40 code generation (`x/r10probe`)

Compiled with the unmodified `bin/i386-gcc -O -S`:

- `double` arithmetic is inline x87: `fldl`, `fmull`, `fdivl`, `faddl`,
  `fstpl`. No helper is called for add, subtract, multiply or divide.
- A `double` return value is left in ST(0). A caller receives it with
  `fstpl (%esp)` after the call.
- Comparisons use `fcompp`, `fnstsw %ax`, `sahf`. No helper is called.
- Conversions call libgcc helpers. The helper names observed for a probe with
  `int`, `unsigned`, `long` and `long long` are:
  - `___fixdfsi` (double to int, used twice)
  - `___fixunsdfsi` (double to unsigned)
  - `___fixdfdi` (double to 64-bit integer)
  - `___floatdidf` (64-bit integer to double)

These four helpers are required for ordinary C89 floating-point code. They are
not in the historical tree (no `libgcc*` file was found under the scratchpad
or the historical tree). Phase 3 must supply them, either from a
`libgcc1.a` built from source with the historical toolchain, or as small
GAS routines in the SDK. Neither source has been checked yet, and no object
from another format may be linked.

`-msoft-float` has not been tested. Phase 3 should compare its output with the
default. It is a diagnostic and fallback only.

## 7. Blockers and decisions

No architectural blocker was found in Phase 0.

Decisions taken from this audit:

1. The reference FPU is the PC `tiny-build/fpu.c`. The standalone build uses it
   in Phase 1 (item 3 in section 3), with the standalone `i386.c` kept.
2. The FPU is claimed only as "double-precision x87 emulation with no
   exceptions and no tag word", until the guest tests show otherwise.
3. The libgcc helper dependency is recorded as a Phase 3 requirement.

## 8. Next steps (Phase 1)

1. Build the standalone harness with the PC `fpu.c` in a copy, and check that
   the R9 harness behaviour (16 MiB RAM, timer, console, input, device page)
   does not change.
2. Write guest probes that inspect memory after `FINIT`, `FLD`, `FSTP`, `FADD`,
   `FMUL`, `FDIV`, `FCOM`, and `FIST`, and run them on both emulators.
3. Only after the probes agree, start the Phase 2 kernel context changes.
