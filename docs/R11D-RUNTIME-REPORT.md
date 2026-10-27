# R11D runtime report

Scope actually run. Every key and input line is scripted; no session was operated by a person.

## Guest tests (independently linked i386 a.out programs)

| Program | Checks | Tiny386 (3 rounds) | i386VM (3 rounds) | QEMU VGA (scripted) |
|---|---|---|---|---|
| C89COR | 83 | PASS ×3 | PASS ×3 | PASS |
| JMPTST | 19 | PASS ×3 | PASS ×3 | PASS |
| SCANTS | 33 (reads "3 4" from stdin) | PASS ×3 | PASS ×3 | PASS |
| MATHX | 24 (table of 529 rows over 14 functions) | PASS ×3 | PASS ×3 | PASS |

Logs: `logs/r11d/tiny386-round{1,2,3}.txt`, `logs/r11d/i386vm-round{1,2,3}.txt`,
`logs/r11d/qemu/` (debug log and screenshot), `logs/r11d/volume.txt`.

## R11B/R11C carry-over (same run, same volume)

HELPER, FPARSE, FMATH, FPRINT, FPCOMB, FPMULT, FPJOB, STREAM, FORMAT, LIBQDO,
REPEAT (twice), CONSOL, EXITFL, CTYPE, STRTST, FTEST, LIBHELLO, CATFIL, MEMSTR,
HEAPEX, HELLO: every program that printed a PASS banner in R11C prints one in R11D
on Tiny386 and i386VM, in all three rounds.

LGTEST (3,576 checks): **3,573 pass, 3 fail** on every run. The three failures are
the known `floatundidf` results (got/want pairs are in the logs). They are documented,
not corrected, and the expected values were not changed.

## Filesystem and boot

- The FAT16 volume `build/fat16/win1-r11d.img` (sha256 `48c7e518…653c59`) is identical
  before and after every matrix run; the scripts check this.
- The QEMU run boots to the VGA shell, accepts scripted keys, runs SCANTS with its stdin
  line, prints `exit status 0` and returns to the `>` prompt (screenshot
  `logs/r11d/qemu/qemu-r11d-scripted.png`). The scripted `dir` command was sent, but its
  output scrolled out of the captured frame, so a file listing is **not** confirmed by this run.

## Not run, or not done

- Historical matrices R2–R10 were not rerun as separate R11D matrices. Their programs
  are in the volume, and the R11B/R11C carry-over above covers the ones that run unchanged.
- libqdos was not expanded (Phase 3 of the plan). The QDOSAPI test and the STREAM and
  REPEAT extensions were not written. Their absence is recorded in
  `docs/R11D-BLOCKED-APIS.md` and `docs/R11D-LIBQDOS-ABI.md`.
- libm accuracy was measured on the table and on Tiny386/i386VM/QEMU; it is not a
  proof for all inputs (see `docs/R11D-LIBM-MAP.md`).
- Build-only or untested symbols. The coverage CSV's `guest_tested` column is the
  per-symbol record. Of 200 rows, 100 implemented functions are named by a run-list
  program, but 8 implemented functions have no test naming them, 2 are named only by
  programs outside the run list, and many partial and blocked functions are untested.
  Among the untested: `strcat`, `puts`, `putc`, `getc`, `fscanf`, `vprintf`, `vfprintf`,
  `perror`, `exit`, `setbuf`, `rewind`, `strftime`, `mktime`, `localtime`, `ctime`,
  `gets`, `fgetpos`, `fsetpos`, `freopen`. These are not claimed as verified.
- No host-side GCC/Clang tests of pure C components were written.

## Claims NOT made

- No 100% ANSI C89 coverage. Time, signal, environment, system, the file-modifying APIs and
  wide characters are blocked or limited; see the blocked-API document.
- No accuracy guarantee for any transcendental function beyond the measured table.
