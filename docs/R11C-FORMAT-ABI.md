# R11C floating-point ABI, conversions and formatting

Status: R11C implementation record. Verified on Tiny386 and i386VM by the
guest tests listed in `docs/R11C-RUNTIME-REPORT.md`.

## Types and the C89 float environment

| Item | Value | Source |
|---|---|---|
| `sizeof(float)`, `sizeof(double)` | 4, 8 (IEEE binary32, binary64) | GCC 1.40 i386 |
| `sizeof(long double)` | **8** (same as `double`). No 80-bit `long double` is provided. | Phase 0 audit; the GCC 1.40 target |
| Arithmetic | x87, control word PC = 53, RC = nearest in the default environment | `FNINIT` value 0x037f, unchanged by R11C |
| Per-job FPU state | Saved and restored by the scheduler; each job starts from the FNINIT state (checked by FPMULT and FPJOB) | R10 |

`float.h` (`sdk/include/float.h`) gives the characteristics of IEEE binary32
and binary64. `LDBL_*` equal the `DBL_*` values, because `long double` is
`double`. The values are the standard ones: `DBL_MAX` 1.79769313486231570815e+308,
`DBL_MIN` 2.22507385850720138309e-308, `DBL_EPSILON` 2.22044604925031308085e-16,
`DBL_DIG` 15, `DBL_MANT_DIG` 53, `FLT_MAX` 3.40282346638528860e+38, `FLT_EPSILON`
1.19209289550781250e-07, and so on.

`errno` (`sdk/include/errno.h`, `sdk/libc/errno.c`) is a plain `int`. `EDOM` is
33 and `ERANGE` is 34. Only `strtod`, `atof`, `pow`, `ldexp` and the domain checks
of `sqrt` set it. `sqrt` sets `EDOM` for x < 0. Nothing in the kernel or the
trap ABI sets it.

## Compiler support (libgcc-qd.a)

GCC 1.40 calls `___floatdidf`, `___fixdfdi`, `___fixunsdfsi` and related helpers
for `long long` and `double` conversions. R11C links them from
`sdk/lib/libgcc-qd.a`, a separate archive (not merged into `libc.a` or
`libm.a`). It is built from the historical gnulib sources with the GCC 1.40
`L_` macros, plus `fixdfsi.s` (the emx routine), because the gnulib C version
of `__fixdfsi` is compiled into a call to itself. See the provenance table in
`docs/R11C-LICENSE-PROVENANCE.md`.

Members: `fixdfdi floatdidf fixunsdfdi fixunsdfsi fixdfsi negdi2 adddi3 subdi3
lshldi3 ashrdi3 cmpdi2`.

The unsigned `long long` to double helper (`___floatundidf`) is **not** in
`libgcc-qd.a`. The three `floatundidf` discrepancies of the LGTEST suite are
kept as documented results (see the runtime report).

## Conversions: strtod and atof

`double strtod(const char *s, char **end)` and `double atof(const char *s)` follow
the C89 grammar in the C locale: optional white space, optional sign, digits with
an optional point, optional exponent (consumed only when it has digits). There is
no hexadecimal form, no infinity and no NaN.

- The significant digits are kept exactly, up to 801. A non-zero digit beyond that
  sets a sticky digit, so rounding still sees it.
- The value is rounded to the nearest double with exact integer arithmetic
  (`sdk/libc/fpbig.c`), round-half-even. The result is correctly rounded for every
  input the table covers (303 rows, Python `float()` as the reference).
- Overflow returns `±HUGE_VAL` (= DBL_MAX) with `ERANGE`. A non-zero input that
  rounds to zero or to a subnormal sets `ERANGE`. The result is still the correctly
  rounded value (subnormals included, e.g. `2.5e-310`).
- `end` points past the consumed text, or to the start of the string when no
  conversion was done.

## Formatting: printf family

`sdk/libc/stdio.c` implements `%f %e %E %g %G` in addition to the integer and
string conversions. The rules:

- Flags `- 0 + space #`, width, `.precision` (capped at 500), length modifiers
  `l h L` (`L` is a no-op because `long double` is `double`).
- Digits come from the exact binary value (`qd_fp_exact`), rounded half-even at
  the requested position, so output matches a correctly rounded `%` in Python.
  Up to 800 significant digits are generated, enough for `%.400f` on any double.
- `%g` chooses the style from the exponent after rounding to `P` significant
  digits (P = precision, 0 counts as 1), and removes trailing zeros unless `#`.
- `%f` of `-0.0` is `-0.0`. Infinity and NaN are not produced (no such inputs
  exist in the SDK).
- `snprintf` and `vsnprintf` are a bounded extension; the return value is the
  length that would have been written (C99 behaviour, documented as an extension).

The table `sdk/examples/r11c/tables/printf.h` holds 900 cases (27 specifications,
including `%.400f`, `%-9g|`, `%#.0e`, `% f`, `%+e`, `%010.2e`). FPRINT checks
every row by string comparison and the return count.

## What is not claimed

- No `%a`, no `%n`, no wide characters, no locale other than "C".
- No correctly rounded `%f` for precision above 800 significant digits.
- `strtod` input longer than 801 significant digits is correctly rounded (the
  sticky digit makes it so) but the digits beyond 801 are not stored.
