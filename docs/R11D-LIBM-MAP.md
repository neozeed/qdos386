# R11D libm map and measured accuracy

## Implementation

| Function | Where | Method |
|---|---|---|
| exp | `libm/trans.c` + `libm/xtrans.s` | Cody-Waite reduction x = n·ln2 + r (ln2 split so n·LN2_HI is exact), x87 `f2xm1`/`fscale` on r, exact power-of-two scaling |
| log, log10 | `xtrans.s` (`qd_x_log`) | x87 `fyl2x` with ln2 or log10(2); base argument read from `16(%ebp)` (fixed in R11D: the R11D draft read the high word of x) |
| sin, cos, tan | `xtrans.s` (`qd_x_trig`) | x87 `fsin`/`fcos`/`fptan`; |x| ≥ 2^63 is a domain error (C2 set) |
| asin, acos | `trans.c` | atan2 with sqrt((1-x)(1+x)); EDOM for \|x\| > 1 |
| atan, atan2 | `xtrans.s` | x87 `fpatan`; EDOM for atan2(0,0) |
| sinh | `trans.c` | Taylor series for \|x\| < 1; (e^x - e^-x)/2 otherwise; ERANGE above 709.78 |
| cosh | `trans.c` | (e^x + e^-x)/2 via exp; ERANGE above 709.78 |
| tanh | `trans.c` | series/cosh for \|x\| < 1; (e^2x-1)/(e^2x+1) otherwise; saturates for \|x\| > 22 |
| fmod | `trans.c` | **exact** C method: scaled subtraction with Sterbenz-exact steps; no x87 remainder; EDOM for y = 0 and infinite x |
| pow, sqrt, floor, ceil, frexp, ldexp, modf, fabs | R11C | unchanged in R11D |

The x87 `fprem` instruction was replaced for fmod: on the emulators used here the
extended-precision remainder is not modelled, so it gave errors up to 1536 ulp.
The C method is exact in IEEE double arithmetic on any conforming FPU.

## Measured accuracy (MATHX, `scripts/r11d-tables.py` reference, seed 1104)

Reference values come from the Python `math` module on the build host. Errors
are in ulp of the result, worst case over the table rows. Bound in MATHX: 2.0 ulp
(not loosened; all functions are well inside it).

| Function | rows | Worst (Tiny386) | Worst (i386VM) |
|---|---|---|---|
| exp | 49 | 1.00 | 1.00 |
| log | 45 | 1.00 | 1.00 |
| log10 | 45 | 1.00 | 1.00 |
| sin | 47 | 0.00 | 0.00 |
| cos | 47 | 0.00 | 0.00 |
| tan | 40 | 0.00 | 0.00 |
| asin | 30 | 1.00 | 1.00 |
| acos | 30 | 1.00 | 1.00 |
| atan | 30 | 0.00 | 0.00 |
| atan2 | 30 | 0.00 | 0.00 |
| sinh | 33 | 1.00 | 1.00 |
| cosh | 30 | 0.00 | 0.00 |
| tanh | 33 | 1.00 | 1.00 |
| fmod | 40 | 0.00 | 0.00 |

## What these numbers do and do not show

- They are **measured on a sample** (the table), not proven bounds for all inputs.
- Tiny386 and i386VM model the x87 in host double precision, so the figures reflect
  those models. Real 80-bit x87 behaviour (QEMU) was not measured for libm in R11D.
- sin/cos/tan/atan match the reference to 0 ulp on the sample, which suggests the
  x87 transcendental results round to the reference on these samples; this is not a claim for all inputs.
- Domain and range errors set errno as C89 requires (EDOM, ERANGE); see `trans.c`.
- Not provided: `ldexp`/`frexp` range edge tests beyond MATHX, `lgamma`, `cbrt`, `hypot`, `expm1`, `log1p` (not C89).
