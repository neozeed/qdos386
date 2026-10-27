# R11C libm map and accuracy

Status: R11C implementation record. The functions are in `libm.a`, built from
`sdk/libm/`. Tests: FMATH (141 table rows for floor, ceil, fabs, frexp, modf and
sqrt; 198 rows for pow; plus ldexp and special cases).

## Functions

| Function | Implementation | Exactness | Errno | Test |
|---|---|---|---|---|
| `fabs` | sign bit cleared | exact | none | FMATH |
| `floor`, `ceil` | `modf` split, then ±1 | exact; zero results keep the sign of x (`ceil(-0.5)` is -0.0) | none | FMATH |
| `modf` | significand bits cleared | exact; a zero fraction has the sign of x | none | FMATH |
| `frexp` | exponent field and bit masks | exact; `frexp(0)` returns 0 with exponent 0 | none | FMATH |
| `ldexp` | exponent field for normal results; scaling by 2^-1000 and one final multiply for subnormal results | exact in the normal range; a subnormal result is rounded once | `ERANGE` on overflow (±HUGE_VAL) and on underflow to zero | FMATH |
| `sqrt` | x87 `fsqrt` with PC = 53 | correctly rounded (IEEE sqrt) | `EDOM` for x < 0 (returns 0) | FMATH (141 rows) |
| `pow` | x87 `fyl2x`, `frndint`, `f2xm1`, `fscale`, see below | bounded error, see below | `EDOM` (0 raised to a negative power, negative base with a non-integer exponent), `ERANGE` (overflow, underflow) | FMATH (198 rows) |

Domain rules of `pow`: `pow(x, 0)` is 1 for every x; `pow(0, y)` is 0 for y > 0
and `EDOM` with HUGE_VAL for y < 0; a negative x with a non-integer y is `EDOM`
and 0; a negative x with an odd integer y gives a negative result.

Not provided: `exp`, `log`, `log10`, the trigonometric functions, `fmod`,
`hypot`, `cbrt`, the `float` (`f`-suffixed) functions, `NaN` and infinity inputs.
None of these are needed by the R11C scope. SuperBASIC needs are in
`docs/R11C-SUPERBASIC-DEPENDENCIES.md` (provisional).

## pow: what the error bound is

`pow` computes t = y·log2(x) with `fyl2x`, splits t into an integer n and a
fraction f, evaluates 2^f with `f2xm1`, and scales by 2^n with `fscale`. The
x87 routine (`qd_pow_core` in `sdk/libm/xmath.s`) sets the control word to a
64-bit significand and rounds only once, when it stores the result to a double.

**The accuracy depends on the x87 that runs the code.** On QEMU (which models the
80-bit format), t keeps 64 significant bits. The measured error on the 198-row
table was **1.0 ulp at worst** (the QEMU debug log line below), with the worst
observed/bound ratio at 0.002. A result of 1 ulp is not a correctly rounded
result, so the claim is "within 1 ulp on the table on QEMU", not "correctly
rounded".

On Tiny386 and i386VM the x87 is modelled with host `double` arithmetic
(`emulator-tiny386/fpu.c`: `fyl2x` computes `temp2 * log2(temp)` in double).
The extended precision of the code is then lost, and the error is set by the
double rounding of t. For |t| that is about |t| ulp of the result, so the error
grows with the size of the logarithm.

The FMATH test therefore checks an analytic bound, not a fixed number of ulp.
For a result r = m·2^e (0.5 ≤ m < 1), |log2 r| lies in [|e|-1, |e|]. The test
allows `|e| + 6` ulp. This allows a few ulp of slack and the growth in |t|.

Measured on i386VM (and on Tiny386, same figures): 198 rows, 0 over the bound,
worst ratio observed/bound 0.636. Measured on QEMU (scripted VGA run):
198 rows, 0 over the bound, worst observed 1.0 ulp, worst ratio 0.002. Worst observed error: 430 ulp (x = 355.886,
y = -104, result 4.6e-266, |e| = 882, bound 888). That is about 9.5e-14 relative.
Typical non-integer rows (|t| below 30) are within 10 to 20 ulp.

What this means for users: on QEMU, `pow` is within 1 ulp on the table. On
Tiny386 and i386VM, `pow` is accurate to about 12 significant digits even in
the worst table row (430 ulp, about 1e-13 relative), and to about 14 digits in
typical rows. That is adequate for the C89 programs this release targets. It is **not** a
correctly rounded `pow` on any target. Do not rely on the last digit of a `pow`
result, or on `pow(10, n)` being exact for n beyond about 22.

## Measurement

The FMATH output prints the table result line. i386VM:

    pow: 198 rows, 0 over the bound, worst observed 430.0 ulp, worst ratio observed/bound 0.636

QEMU (`logs/r11c/qemu/debug-0x402.txt`):

    pow: 198 rows, 0 over the bound, worst observed 1.0 ulp, worst ratio observed/bound 0.002

The "worst observed" figure in ulp is the largest distance from Python's
`math.pow` (correctly rounded in practice), in units of the ulp of the reference.
