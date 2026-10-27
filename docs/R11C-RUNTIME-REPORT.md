# R11C runtime report

Status: runs of the R11C tree on Tiny386 and i386VM (scripted), and one scripted
QEMU VGA run. Every key is scripted. None of these runs is a human session.
The owner's human sessions are not represented here.

## Build inputs

- Source: branch `r11c`, commit recorded in `PACKAGE.md` of the package.
- Volume: `build/fat16/win1-r11c.img` built by `scripts/r11c-volume.sh` with
  `LGTEST4.aout` (sha256 in `EXTERNAL-DEPENDENCIES.txt`).
- Toolchain: historical GCC 1.40 / GAS 1.38 / i386-ld tree (`QDOS386_TOOLS`).
- Compiler support: `sdk/lib/libgcc-qd.a`, built by `sdk/compiler/Makefile`.
- Final volume sha256: `0771f1c786bfd467ec5e5203b0bcfb3a7ce877acbb1617f3a38780b2a25c988d`.
  The FAT16 volume was unchanged by every run (hash checked before and after).

## Matrix: scripted Tiny386 and i386VM, three rounds each

Script: `scripts/r11c-run-all.sh` (ROUNDS=3). Command list: HELPER, FPARSE, FMATH,
FPRINT, FPCOMB, FPMULT, FPJOB, then the R11B set (STREAM, FORMAT, LIBQDO, REPEAT,
CONSOL with its two lines, EXITFL, CTYPE, STRTST, FTEST, LIBHEL, CATFIL, MEMSTR,
HEAPEX, HELLO) and LGTEST.

| Program | Tiny386 (x3) | i386VM (x3) | Notes |
|---|---|---|---|
| HELPER (compiler helpers, 30 checks) | PASS | PASS | Run first, independently of the rest |
| FPARSE (strtod table, 303 rows + 8 checks) | PASS | PASS | |
| FMATH (floor/ceil/fabs/frexp/modf/sqrt 141 rows, ldexp, pow 198 rows, 12 checks) | PASS | PASS | pow: 0 rows over the analytic bound |
| FPRINT (printf table, 900 rows + 9 checks) | PASS | PASS | |
| FPCOMB (combined program, run twice) | PASS | PASS | Same output both runs |
| FPMULT (two jobs, live x87 state, 3 rounds) | PASS | PASS | |
| FPJOB (R10 frozen fixture) | PASS | PASS | Unchanged binary, hash in the manifest |
| LGTEST (3,576 checks, frozen fixture) | 3 FAIL | 3 FAIL | Expected: the three `floatundidf` results are documented; `LIBGCC CHECKS passes=3573 fails=3` |
| R11B set (STREAM … HELLO) | PASS | PASS | All suites reported their own pass lines |

Logs: `logs/r11c/tiny386-round{1,2,3}.txt`, `logs/r11c/i386vm-round{1,2,3}.txt`,
`logs/r11c/volume.txt`, `logs/r11c/*-input.txt`.

The three `floatundidf` discrepancies are the unsigned `long long` to double
conversion (`___floatundidf`, not in `libgcc-qd.a`). The reference values are
one ulp apart in each case. They are kept as documented results, as the owner
confirmed for this milestone.

## QEMU VGA smoke (scripted)

Script: `scripts/r11c-qemu-smoke.sh`, run under Xvfb with xdotool keystrokes into
the QEMU GTK window. Output: `logs/r11c/qemu/` (debug log from port 0x402,
screenshot `qemu-r11c-scripted.png`).

Result: HELPERS, FPARSE, FMATH, FPRINT, FPCOMB, FPMULT and FPJOB all reported
PASS. The FAT16 volume was unchanged by the run. QEMU's `pow` measurement is in
`docs/R11C-LIBM-MAP.md` (1.0 ulp worst on the table).

The QEMU log does not contain the full LGTEST run (not in the QEMU command list).

## Known limits of this report

- The R2 to R10 regression matrices (`scripts/run-all.sh`, `scripts/r7-matrix.sh`
  and the R8-R10 suites) were **not** rerun against this tree in this session.
  The R11B command set above exercises the same libc and libqdos paths they cover,
  but this is not the complete historical matrix. Run them before any public release.
- `FPJOB` on Tiny386 and i386VM passes with the frozen binary; its provenance
  (no matching source) is still under separate investigation, as agreed.
- `pow` is not correctly rounded on any target. See the LIBM map.
- No file write is tested in R11C. The FAT16 volume is read-only in this release;
  no run modified it.
- SuperBASIC was not run (SB3 is not attached).
