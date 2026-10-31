# R11C SuperBASIC dependency list (PROVISIONAL)

Status: **provisional.** The SB3 source archive (`SuperBASIC-C89-SB3.tar.gz`) is
not attached to this project, so the SB3 import list cannot be audited. This file
records the imports the R11C plan expects and what R11C provides for them. It is
not an audit of SB3 and it does not claim SB3 works.

R11C does **not** integrate SuperBASIC. No SuperBASIC code is in the package and
no SB3 test is run.

## Expected imports (unverified)

| Name | Expected use | R11C provision | Verified by |
|---|---|---|---|
| `strtod`, `atof` | number parsing | `sdk/libc/strtod.c`, C89 grammar, correctly rounded | FPARSE (303 rows), FPCOMB |
| `sprintf`, `snprintf` | number output | `%f %e %g` with flags and precision | FPRINT (900 rows) |
| `floor`, `ceil`, `fabs` | rounding, sign | `sdk/libm`, exact | FMATH (141 rows) |
| `frexp`, `ldexp`, `modf` | number decomposition | `sdk/libm`, exact except `ldexp` out-of-range | FMATH |
| `pow` | exponentiation | `sdk/libm`, bounded error (see LIBM map) | FMATH (198 rows) |
| `sqrt` | square root | `sdk/libm`, correctly rounded | FMATH |
| `DBL_MAX`, `DBL_MIN`, `DBL_EPSILON`, `DBL_DIG` | limits | `sdk/include/float.h` | compile-time and FMATH |
| `errno`, `EDOM`, `ERANGE` | error reporting | `sdk/include/errno.h` | FPARSE, FMATH |

## What SB3 needs that is outside R11C

- The actual import list of SB3 (to be produced from the source when attached).
- Any SB3 use of `long double` (none is expected to be supported: it is `double`).
- Any SB3 use of functions outside the list above (`exp`, `log`, `sin`, ...). None
  exist in R11C.

## Next step when SB3 is attached

Run the import audit against the SB3 source and replace the "expected" column
with the observed one. Then decide whether each missing name is a blocker.
