# R11D licence and provenance notes

This is private compatibility research. Historical GNU, C68 and other third-party
copyright and licence notices in the existing tree are kept unchanged. Redistribution
terms for any third-party file must be checked per file; none is asserted here.

## New in R11D (written for this project)

- Headers: `sdk/include/assert.h setjmp.h locale.h signal.h time.h`; additions to `stdio.h stdlib.h string.h math.h limits.h`.
- Sources: `sdk/libc/jmp.s misc.c time.c scanf.c`, `sdk/libm/trans.c xtrans.s`; additions to `sdk/libc/stdio.c stdlib.c errno.c string.c`; `sdk/lib/qdos.c` (`qd_exit_record`), `sdk/lib/crt0_libc.s`.
- Tests: `sdk/examples/r11d/` (C89COR, JMPTST, SCANTS, MATHX, check.h, exit test programs).
- Tools: `scripts/r11d-*.py|sh`.
- Generated: `sdk/examples/r11d/tables/mathx.h` (from `scripts/r11d-tables.py`, seed 1104), `docs/R11D-C89-COVERAGE.*`.

No file from the historical C68/QDOS libc 4.24.5 archive is copied into the R11D tree
(checked: the new files carry no C68 copyright or source markers). The archive is
unchanged.

## External inputs (not in the source archive)

Hashes are recorded in `EXTERNAL-DEPENDENCIES.txt` in the package: Tiny386, i386VM
and its harness, BIOS and VGA BIOS, the GCC 1.40 / GAS / i386-ld toolchain, and the
frozen fixtures `LGTEST4.aout` and `FPJOB.aout`. FPJOB's provenance is still under
separate investigation (see R11C).
