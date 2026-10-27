# R10 freeze report (draft, Phase 0 of R11A plan)

## Checkpoints
| Checkpoint | Ref | Commit | Notes |
|---|---|---|---|
| Headless/diagnostic R10 FPU baseline | tag `r10-fpu-frozen` | 533ee57 | per-job x87, FPU enablement, rounding evidence |
| R10-VGA human-facing build | tag `r10-vga-frozen` (on branch `r10-vga`) | 9f3340a | VGA text console behind QDOS_CON_VGA |

## Artifacts (sha256, first 20 hex chars)
- Tiny386 FPU rounding patch `fpu-round-nearest.patch`: ae8ef6f15c1ad7f43a91
- R9 archive `qdos386-R9.tar.gz`: b6ea266dd29fb441a974
- SeaBIOS `bios.bin`: 7bd968419c81fcf8262c (rel-1.17.0)
- VGA BIOS `vgabios.bin`: f5aedb52cde5c88d85e5
- i386-gcc (GCC 1.40 toolchain, under scratchpad/x/i386vm): 3567f3beab83e17d7f7b
- Default image `r9shell.img`: 8b4c7b864c6a (unchanged by the VGA work)
- Interactive VGA image: 6bbfc1a0638a (see R10-VGA release)
- Data volume `win1-vga.img`: 758c5a23f055 (rebuilt; earlier 734a09 superseded)

## Verified (Phase 0, this session)
- Regression matrix, R8 and R9, i386VM and Tiny386, 3 rounds each (`scripts/r9-matrix.sh`,
  run in a worktree of `r10-vga-frozen`): all suites and sessions pass, timer-off negative
  ok, FAT16 volumes unchanged, `MATRIX_EXIT=0`.
- R6 (`scripts/run-all.sh`) and R7 (`scripts/r7-matrix.sh`), 3 rounds, i386VM and Tiny386
  (run on the R11A tree, which has no kernel change). R6: 185 checks, 0 FAIL in every round.
  R7: every suite check passes, but the matrix banner check fails because of a stale
  harness string (`R7` expected, `R8` printed; 19/19 shell self-test checks pass).
  See `docs/R11A-RUNTIME-REPORT.md`.
- LGTEST4 (3,576 checks, Tiny386, SCRIPTED): 3,573 pass, 3 known `floatundidf` lines.
  Expectation unchanged.
- FPJOB: PASS on Tiny386, i386VM and QEMU.
- Graphical QEMU boot to the shell with keyboard input (SCRIPTED, xdotool): HELP, DIR,
  TYPE, RUN HELLO.AOU, backspace, unknown command, FPJOB, and the R11A tests.
- USER-REPORTED (human, QEMU graphical window): the owner booted the VGA shell and typed
  `dir` and `run hello.aou` (exit status 7). Stored in `logs/r11a/user-reported/`. It was
  not performed by the agent.

## Not verified / open
- Freeze reproducibility: the R2 to R5 matrices were not re-run separately; the R7 suite
  covers the R4 and R5 banners. The FPJOB binary is shipped, not rebuilt (see R11A runtime
  report).
- The 3,576-check suite binary is `fpu-diag/package/bin/LGTEST4.aout` (sha256 8003124a…).
  Its source is `lgtest.c` in the same package.
- Tiny386 binary: a local build from the patched dev tree (`fpu-diag/tiny/vga-window`),
  not the user's installed Tiny386. The user's binary is unchanged.

## Status
Phase 0 is complete for the checks listed above. R11A has started from `r10-vga-frozen`
(`r11a` branch). The freeze checkpoints are the tags `r10-fpu-frozen` (533ee57) and
`r10-vga-frozen` (9f3340a), which are not moved.
