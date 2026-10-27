# R11A runtime report

Branch `r11a`, from `r10-vga-frozen` (9f3340a). Kernel sources are unchanged in
R11A; the milestone changes the SDK (`sdk/`), tests, scripts and documents only.

## Labels used throughout

- **SCRIPTED (Tiny386)**: keys are sent by `tests/interactive/session.py` through
  the Tiny386 PS/2 path (`run_pc_multi.py`).
- **SCRIPTED (i386VM)**: the same commands piped to the i386VM harness.
- **SCRIPTED (QEMU)**: X keystrokes injected with `xdotool` into the QEMU GTK window.
- **USER-REPORTED (human, QEMU)**: a screenshot supplied by the project owner, who
  typed the commands himself. It is stored as `logs/r11a/user-reported/`. The
  agent did not perform this session and does not claim to have done so.

No agent-driven run is a human-operated session.

## Results

### R11A guest tests (C89 libc expansion)

Final volume `build/fat16/win1-r11a.img`, sha256 `1a0f5d77bc181d9e…` (13 entries),
rebuilt from source by `scripts/r11a-volume.sh`. Runner: `scripts/r11a-run-all.sh`.

An earlier volume (sha256 `1d56a815…`) was superseded. Its four libc example binaries
(LIBHEL, CATFIL, MEMSTR, HEAPEX) were stale: the example rules did not depend on
`libc.a`, so they had not been relinked. The volume script now removes them before
building (commit `0a51fdf`). A clean `git archive` extraction of that commit produces
the same volume hash as the main tree, `1a0f5d77…`.

| Target | Checks ok | PASS banners | FAIL |
|---|---|---|---|
| Tiny386, SCRIPTED (13 commands, one boot) | 248 | 11 | 0 |
| i386VM, SCRIPTED (same commands, piped) | 248 | 11 | 0 |
| QEMU VGA, SCRIPTED (xdotool, `scripts/r11a-qemu-smoke.sh`) | 124 (CTYPE 46, STRINGS 37, FILETEST 41); HELLO exit 7; FPJOB PASS | 4 | 0 |

Emulator binaries: Tiny386 is the patched dev-tree build `fpu-diag/tiny/vga-window`
(sha256 `e201502f21f488aa…`), the same binary as the R10-VGA freeze; the i386VM harness
is `i386vm-r9` (sha256 `a4724f79b26a716e…`). Neither is redistributed in the package.

All three rows are from the final volume. The QEMU run is in `logs/r11a/qemu-final/`
(`debug-0x402.txt`, `qemu-r11a-scripted.png`) and the volume hash was unchanged by the
run. The first attempt sent keystrokes with `xdotool --window`, which QEMU ignored; it
was discarded and the script now focuses the window and types to it.

Commands in the Tiny386 and i386VM sequences: DIR, CTYPE, STRTST, FTEST, LIBHEL,
CATFIL, MEMSTR, HEAPEX, CTYPE, STRTST, FTEST (repeated), HELLO, FPJOB. The repeated
libc programs ran twice in one boot. Each program prints its own banner, for example
`QDOS/386 R11A CTYPE PASS`, only when all its guest-side assertions pass.

Per-program checks: CTYPE 46 (counts over ASCII, boundaries, EOF, signed-char
arguments); STRINGS 37 (span, token, strtol clamps, qsort/bsearch, snprintf); FILETEST 41
(fopen refusal, fgets to EOF, ungetc and ftell, fseek discarding pushback, 20
open/close cycles). The 248 checks are exactly two runs of those three programs
(2 × (46 + 37 + 41)), since the repeated sequence runs them twice.

Existing SDK examples rebuilt against the new libc and run unchanged: LIBHEL
(exit status = argc, by design), CATFIL, MEMSTR, HEAPEX, all PASS.

### Regression (R2 to R10)

| Suite | Target | Result |
|---|---|---|
| R8 and R9 matrix (`scripts/r9-matrix.sh`, in worktree of `r10-vga-frozen`) | i386VM and Tiny386, 3 rounds | suites PASS (R8 204 checks, R9 211 checks, each round); R9 scripted sessions 29/29; PS/2 29/29; EOF sessions 2/2; timer-off negative ok; FAT16 unchanged; `MATRIX_EXIT=0` |
| R6 (`scripts/run-all.sh`, R11A tree) | i386VM and Tiny386, 3 rounds | 185 checks, 0 FAIL, order ok, every round; timer-off negative ok; FAT16 `81dd402a…` before and after |
| R7 (`scripts/r7-matrix.sh`, R11A tree) | i386VM and Tiny386, 3 rounds | every suite check passes, see note below; R7 scripted sessions PASS in all 3 rounds; timer-off negative ok |
| R10 FPU, LGTEST4 (3,576 checks) | Tiny386, SCRIPTED | 3,573 pass, 3 known `floatundidf` lines; `LIBGCC FAIL (3576 checks, 3 failures)` as before (expectation unchanged) |
| R10 FPJOB | Tiny386, i386VM, QEMU | PASS in all three |
| R2 to R5 | | not re-run as separate matrices. The R7 suite prints the R4 and R5 banners (`R4 CHANNEL PASS`, `R5 A.OUT EXEC PASS`) and passes. R2 and R3 are covered only by the suite's own checks |

**R7 harness note.** The R7 matrix requires the banner `QDOS/386 R7 SHELL SELFTEST PASS`.
The shell prints `QDOS/386 R8 SHELL SELFTEST PASS` (`apps/shell/shell.c` line 598),
and its self-test reports `SHELL SELFTEST: 19 checks, 0 failed`. The string in the
harness is stale; the shell is unchanged since before R10. The matrix verdict therefore
reads FAIL in all three rounds, while the underlying checks pass. This was **not**
changed in the harness, to avoid editing an expectation to force a result. It should be
corrected in a separate harness commit that names the banner from the shell source.

### R10 human and VGA evidence

- USER-REPORTED: the project owner's QEMU graphical window shows
  `QDOS/386 R8 shell. Type HELP.`, `dir` (SHELL, HELLO, FPJOB, README, CHAIN, 5 files),
  `run hello.aou` → `Hello from an independently linked i386 a.out!`, `exit status 7`.
  Stored in `logs/r11a/user-reported/`.
- SCRIPTED: the same boot and commands in QEMU (`qemu-final-scripted-shot.png`,
  `qemu-final-0x402.txt`) and in the windowed Tiny386 (R10-VGA release). These two
  earlier QEMU files (`qemu-final-*`) were produced on the superseded `1d56a815…` volume.

## Deviations from the plan

1. **Names.** The kernel allows at most 10 characters with the dot in a volume file name
   (`WIN1_` + name in a 16-byte buffer). `LIBHELLO.AOU`, `STRINGS.AOU` and `FILETEST.AOU`
   cannot be run, so the tests are `LIBHEL.AOU` (existing), `STRTST.AOU` and `FTEST.AOU`.
   Changing this limit is a kernel change and is out of scope.
2. **FPJOB binary.** The tested `FPJOB.AOU` (8,601 bytes, sha `114ed367…`) is shipped
   in `tests/r10/`. Rebuilding `tests/r10/fpjob.c` with the current wrapper gives
   9,572 bytes, not the same bytes, so the rebuild is not reproducible. Not fixed here.
3. **Logs.** The R6 and R7 matrices write into tracked historical log directories.
   This was caught during the run; the tracked logs were restored with `git checkout`,
   and the new runs are kept under `logs/r11a/r6` and `logs/r11a/r7`.
4. **Human session.** No human-operated R11A session has been recorded. The only human
   evidence is the owner's R10-VGA screenshot above.

## Not done

- A complete C68 libc (895 files), libm, SuperBASIC integration, and float printf/scanf.
  See `R11A-C68-PORT-MAP.md` and `R11A-MATH-ROADMAP.md`.
- Automated QEMU coverage beyond the smoke test: boot, keyboard, FAT16, shell, HELLO,
  libc tests and FPJOB. No QEMU timing or regression matrix.

## Open questions

- **Licence.** `libc-4.24.5` has no licence file in the attached tree. Only the
  semantics were used, and no file was copied. The owner should confirm the licence
  before any redistribution of derived code. Private testing is not affected.
- **LGTEST4 provenance.** The binary is the one from the R10 FPU package
  (`fpu-diag/package/bin/LGTEST4.aout`, sha256 `8003124a…`). Its source `lgtest.c` is also in the
  package. Its expectation (three known discrepancies) is unchanged.

## Known emulator notes

- FNSTENV tag word reads 0000 on Tiny386 and i386VM, and ffff on QEMU (R10 notes).
- Tiny386 windowed build clips the last text row at 720×400 (cosmetic, R10-VGA report).
