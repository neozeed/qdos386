# R11A math and floating-point roadmap

**R11A does not provide libm, and does not claim floating-point printf or
parsing.** This note records what was inventoried so that R11B/R11C can start from
evidence.

## What R11A verified

- Native x87 with per-job save and restore is the R10 result, reused unchanged.
  `LGTEST4` (3,576 checks, the compiler-runtime suite) passes 3,573 on the R11A
  kernel with the same three known GCC 1.40 `floatundidf` discrepancies:
  `43e998f0c723a198`/`…199`, `43e0caf45c58267c`/`…67b`, `43ee1ac9ad6f4a8e`/`…8d`.
  The suite was not changed to hide them.
- The FPJOB guest test (x87 per-job state across two jobs) reports PASS on Tiny386,
  i386VM and QEMU, in the scripted runs listed in `R11A-RUNTIME-REPORT.md`.
- The libc in R11A contains no floating-point code.

## C68 floating-point dependencies (inventory)

| C68 file | Role | Note |
|---|---|---|
| `libieee/*.s` (`GYdfadd`, `GYdfmul`, `GYdfdiv`, `GYsfadd`, … `Ydfadd`, `Yasmuldf`, …) | IEEE single and double arithmetic in 68000 assembler | Would need i386 rewrites or direct use of x87 instructions. |
| `libieee/frexp.s`, `ldexp.s` | Mantissa and exponent split and join | Candidates for a minimal C or x87 version. |
| `libieee/Xnorm4.s`, `Xnorm8.s`, `Xerror.s` | Normalisation and error paths | Depend on the 68000 status register conventions. |
| `libieee/GYdftol.s`, `GYdfltodf.s`, `GYdftosf.s` | Conversions | Needed by `printf %f` and `strtod`. |
| `stdlib/strtod.s`, `stdlib/_strtod.c`, `strtod_aux.x` | String to double | Depends on the libieee routines. |
| `stdio/_f_printf.c`, `_f_sprintf.c`, `_f_pow10.c`, `_ipow10.c`, `_z_cvt.c`, `_f_cvt.c` | Floating-point formatting | Depends on `libieee`. |
| `pow`, `floor` | Not located in the attached tree (searched by name). | Needs a source before R11B can plan them. |

## SuperBASIC dependencies

The SuperBASIC integration needs `strtod`, `floor`, `frexp`, `ldexp`, `pow` and
`%f` formatting (from the R11A brief). Of these, only `frexp` and `ldexp` have
68000 sources in the tree; `floor` and `pow` have none. Recorded for R11B.

## Recommended R11B order

1. A bounded `double` ABI probe under GCC 1.40: argument passing, `ST(0)` returns,
   conversion helpers, and preemption across jobs (the probe, not libm).
2. `frexp` and `ldexp` in C using the x87 `FSCALE`/`FXTRACT` instructions (the
   R10 FSCALE operand-order lesson applies).
3. `floor`, then `pow` if SuperBASIC needs it.
4. `%f` formatting and `strtod`, last, because they depend on correct rounding
   (the Tiny386 rounding limits are in the R10 FPU notes).

## Known limits carried forward

- Tiny386: the external rounding-mode and subnormal patch is version-specific
  (`fpu-round-nearest.patch`); see `R10-FREEZE-REPORT.md`.
- The FNSTENV tag word reads 0000 on Tiny386 and i386VM, and ffff on QEMU.
