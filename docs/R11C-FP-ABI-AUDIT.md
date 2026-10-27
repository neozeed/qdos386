# R11C Phase 0: floating-point ABI and dependency audit

Branch `r11c` from `r11b` (`aa44be5`). Status: Phase 0 audit, written before any
R11C implementation. Every claim below was checked with the historical GCC 1.40
(`cc1`, `-O`) output or a link attempt through `sdk/bin/qdos386-gcc`.

## Baseline

- R11B archive `qdos386-R11B.tar.gz`, sha256 `d9e3d386…5545964`, source commit `aa44be5`.
  Fresh extraction: 478 of 478 manifest entries OK. Source-only rebuild from
  `git archive`: `libc.a`, `libqdos.a`, every example and `win1-r11b.img`
  (`030056e5…`) match the packaged bytes.
- Integrated baseline on Tiny386, i386VM and QEMU for R11C: not yet run on the
  R11C branch. It is part of Phase 5 and is not claimed here.

## GCC 1.40 floating-point ABI (verified from output)

Probe: `double`, `float`, `long double`, `unsigned long long -> double`,
`long long` and `unsigned int` conversions, and a variadic call with `double` and `float`.

| Item | Result | Evidence |
|---|---|---|
| `sizeof(double)` | 8 | static assertion compiles |
| `sizeof(float)` | 4 | static assertion compiles |
| `sizeof(long double)` | **8** (same as `double`; no 80-bit type) | static assertion compiles; `ldd` loads `fldl 8(%ebp)` |
| Argument passing | on the stack, cdecl, 4-byte slots; `double` takes 8 bytes | `addd` reads `fldl 8(%ebp)`, `fmull 16(%ebp)` |
| `float` argument (prototyped) | passed as 4 bytes, loaded with `flds` | `addf` |
| `float` in variadic call | promoted to `double` (8 bytes) | `call` pushes two words for `3.5f` |
| `double` return | in ST(0) | `addd`, `ldd` leave the result on the x87 stack |
| Arithmetic | inline x87 (`fldl`, `fmull`, `faddl`, `ftst`, `fnstsw`, `sahf`) | `addd` |
| Constants | emitted as `.double` in `.text` | `LC0`, `LC1`, `LC2` |

Consequences for R11C:
- `long double` is the same as `double`. Any `long double` code path is a
  double-precision path and must not be documented as extended precision.
- `float.h` values must describe the 8-byte `double` that GCC 1.40 uses. The
  x87 extended precision of the FPU is not visible to C.

## Compiler helper references (the link finding)

GCC 1.40 emits calls to runtime helpers for some conversions. A probe with
`(double) unsigned long long`, `(long long) double` and `(unsigned int) double`
fails to link through `qdos386-gcc`:

```
undefined reference to `___fixunsdfsi'
undefined reference to `___fixdfdi'
undefined reference to `___floatdidf'
```

- `sdk/bin/qdos386-gcc` does not link any compiler runtime. It links `crt0`,
  the objects, `libc.a` and `libqdos.a` only.
- Plain `double` arithmetic and comparison use inline x87 and need no helper.
  A plain-`double` program links.
- Helpers in the attached historical material, to be reviewed for provenance:
  `gnulib.c`/`gnulib2.c` (GCC 1.40 tree, GNU), `_udivsi3.s`, `_divsi3.s`,
  `_fixdfsi.s` (emx/gcc, Eberhard Mattes, 1990-92), `gccmath.zip` (`fixdfsi.s`,
  `fixudfdi.s`, `fixuxfdi.s`, `cmpdi2.s`, `muldi3.s`, `ulldiv.s`, 1993, author
  not stated in the archive).
- Decision for Phase 1: a separate compiler-support archive, `sdk/lib/libgcc-qd.a`,
  built only from helpers whose provenance is recorded. It is not merged into
  `libc.a` and not hidden in the wrapper. `strtod` and `printf` will avoid
  `long long` conversions where x87 can do the job, so the libc does not depend
  on the helper archive.

## FPU and control-word behaviour

- The x87 helpers in the historical runtime save the control word (`fstcw`),
  set truncation (`orw $0x0c00`) and restore it (`fldcw`). Any R11C routine that
  changes the rounding mode must restore it before returning.
- The R10 per-job save and restore of x87 state is reused unchanged. R11C adds no
  scheduler change.
- `ftst`/`fnstsw`/`sahf` comparison sequences are used by compiler output.

## Current libc and headers (what exists)

- `sdk/include`: `ctype.h limits.h qdos386.h stdarg.h stddef.h stdio.h stdlib.h string.h`.
  **No** `math.h`, `float.h`, `errno.h`, `assert.h`.
- `stdlib.c`: `strtol`, `strtoul`, `atoi`, `atol`. **No** `strtod`, `atof`, `errno`.
- `stdio.c` formatter: integer, string and character conversions only. No `double`
  path. `errno` is not set anywhere (the R11B ABI says so).
- `libc.a` contains no floating-point code (R11A/R11B).

## C68 reference sources (libc-4.24.5)

- `libieee/` (68000 assembler): `frexp.s`, `ldexp.s`, `modf.s`, `modff.s`,
  `Ydfadd.s`, `Ydfmul.s`, `Ydfdiv.s`, `Ydftol.s`, `Ydfutodf.s`, `Ydftoul.s`,
  `Ydfltodf.s`, `Xnorm8.s`, `Xnorm4.s`, `Xerror.s`, `FPcheck.s`, and others.
  All are 68000 code. They are reference only; no 68k object is linked into an
  i386 application.
- `floor`, `pow`, `ceil`, `sqrt`, `fabs`: no file of these names in the C68 tree
  (checked by file name against `libc-4.24.5.tar.bz2`). They will be written for
  R11C, not ported.
- Also present and reference only: `stdlib/atof.c`, `stdlib/strtod_aux.x`,
  `stdio/_f_cvt.c`, `stdio/_z_cvt.c`, `stdio/powers.h` (the power-of-ten table).
- The C68 `stdio/_f_printf.c` and `stdlib/strtod.s` depend on `libieee`. They are
  reference for behaviour only.

## SuperBASIC SB3 inputs

- **Not attached.** No `SuperBASIC-C89-SB3.tar.gz` is in the attachments. The
  dependency audit (`docs/R11C-SUPERBASIC-DEPENDENCIES.md`) therefore cannot list
  SB3's real imports yet. The imports named in the plan (`strtod`, `sprintf`,
  `floor`, `pow`, `frexp`, `ldexp`, `DBL_MAX`) are recorded as the expected set
  and marked unverified.

## Phase 0 findings that change the plan

1. `sizeof(long double) == 8`. No extended-precision C type exists in the SDK.
2. Compiler conversion helpers are not linked by the SDK. This blocks
   `(double)` of `long long` and `(long long)` of `double` in ordinary programs.
   A helper archive, separate and with recorded provenance, is needed. Phase 1
   must decide whether the scope is `sdk/lib/libgcc-qd.a` with only the helpers
   actually required.
3. No `math.h`, `float.h` or `errno` exists. `float.h` and `errno` come first.
4. SB3 source is missing. The SB3 audit is partial.

## Next

Phase 1 begins with the helper archive decision, `float.h`, and `errno`, then
`strtod`/`atof` with differential tests against a reference (host `strtod`, run
offline and its output stored as fixtures).
