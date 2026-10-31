# R11B runtime report

Branch `r11b`, from `r11a` (commit `0b0ad47`). R11A kernel sources are unchanged.
R11B changes the SDK (`sdk/libc`, `sdk/lib`, `sdk/include`, `sdk/examples/r11b`),
the wrapper (`sdk/bin/qdos386-gcc`), the scripts and the documents.

## Labels

- **SCRIPTED (Tiny386)**: keys sent through the PS/2 path by
  `tests/interactive/run_pc_multi.py`.
- **SCRIPTED (i386VM)**: the same commands piped to the i386VM harness.
- **SCRIPTED (QEMU)**: keys injected with `xdotool` into the QEMU window.
- **USER-REPORTED (human, QEMU)**: the project owner's screenshot (R10-VGA,
  `logs/r11a/user-reported/`). The agent did not perform this session. No
  human-operated R11B session has been recorded.

## Results: R11B on the final tree

Volume `build/fat16/win1-r11b.img`, sha256 `030056e5…` (21 entries, including
`LGTEST.AOU`). Runner: `scripts/r11b-run-all.sh`, `ROUNDS=3`.

| Target | Rounds | Checks ok | PASS banners | FAIL | FAT16 before/after |
|---|---|---|---|---|---|
| Tiny386 (SCRIPTED) | 3 | 240 per round | 14 per round | 0 | unchanged |
| i386VM (SCRIPTED) | 3 | 240 per round | 14 per round | 0 | unchanged |
| QEMU VGA (SCRIPTED, xdotool) | 1 | 236 | 8 banners, plus `FPJOB PASS` and HELLO exit 7 | 0 | unchanged |

The 240 checks per round are:

| Program | Checks | Banner |
|---|---|---|
| STREAM | 44 | `QDOS/386 R11B STREAM PASS` |
| FORMAT | 32 | `QDOS/386 R11B FORMAT PASS` |
| LIBQDO | 22 | `QDOS/386 R11B LIBQDO PASS` |
| REPEAT (run twice) | 4 + 4 | `QDOS/386 R11B REPEAT PASS` |
| CONSOL (typed input) | 10 | `QDOS/386 R11B CONSOL PASS` |
| CTYPE | 46 | `QDOS/386 R11A CTYPE PASS` |
| STRINGS | 37 | `QDOS/386 R11A STRINGS PASS` |
| FILETEST | 41 | `QDOS/386 R11A FILETEST PASS` |

Also in each round, with their own output: EXITFL (text with no newline, written
after `main` returns, appears before `exit status 0`), LIBHEL, CATFIL, MEMSTR
(`R9` banners), HEAPEX, HELLO (exit status 7) and FPJOB (`FPJOB PASS`).
LIBHEL's exit status is its argc, by design.

The QEMU log is `logs/r11b/qemu/debug-0x402.txt` and the screenshot is
`logs/r11b/qemu/qemu-r11b-scripted.png`.

## Regression (R2 to R10)

Run on this tree after the R11B changes (`logs/r11b/r6`, `logs/r11b/r7`).

| Suite | Target | Result |
|---|---|---|
| R6 (`scripts/run-all.sh`) | i386VM and Tiny386, 3 rounds each | 185 checks ok, 0 FAIL, order ok, every run. Timer-off negative: no PASS, as expected. FAT16 disk hash `81dd402a…` unchanged before and after. |
| R7 (`scripts/r7-matrix.sh`) | i386VM and Tiny386, 3 rounds each | Suite: 205 CHECK lines, 0 FAIL, in every run on both targets. Three shell sessions (i386VM, Tiny386, i386VM EOF) pass in every round. Timer-off negative ok. The R7 banner expectation is now `R8` (see fixes). |
| R9 / R10 matrices | | **not re-run on R11B.** The R9 matrix ran earlier in the R10 worktree. Its R9 apps (LIBHEL, FILECAT, MEMSTRESS, HEAPEX) and FPJOB are run in every R11B round above. |
| R10 FPU LGTEST (3,576 checks) | Tiny386 | **not re-run on R11B.** LGTEST is in the volume but no R11B round runs it. Its result on the R11A tree was 3,573 pass and the three known `floatundidf` lines. |
| R2 to R5 | | not re-run as separate matrices. R7 prints the R4 and R5 banners. |

## Fixes made in R11B

1. **Stale example binaries (R11A).** The example rules did not depend on
   `libc.a`. They now depend on `libc.a`, `libqdos.a` and the startup sources
   (`sdk/examples/Makefile`, `LIBS`). The volume script no longer deletes binaries.
2. **Shell banner in the R7 harness.** The R7 matrix expected
   `R7 SHELL SELFTEST PASS`. The shell has printed `R8 SHELL SELFTEST PASS` since
   R8. The expectation is now `R8`, documented in `scripts/r7-matrix.sh`. The
   guest is unchanged.
3. **stdout lost at exit.** Buffered stdout is now written by `exit()` and by
   the return from `main` (`crt0_libc.s`).
4. **CR as Enter.** The keyboard sends CR for Enter. `stdin` returns LF.
   Without this, `fgets` never saw the end of a typed line.
5. **REPEAT heap baseline.** The first `printf` opens the console channel, which
   holds about 80 bytes of heap for the life of the job. The baseline is now
   taken after that.
6. **FORMAT buffer size.** A test expectation was wrong (six characters need a
   buffer of seven); the test is corrected, not the formatter.
7. **Runner.** `run_pc_multi.py` takes `!` (type without waiting for a prompt)
   and `@` (short wait) prefixes, and `PC_CMD_TIMEOUT`. A program waiting for
   input no longer blocks the runner for 1,400 seconds.
8. **Added:** `fprintf`, `fputc`, `putc`, `fputs`, `fwrite`, `fflush`, `setvbuf`,
   `setbuf`, `vsprintf`, `rewind`, and the buffered output model.

## FPJOB fixture (Phase 0, item 4)

Three FPJOB binaries and two sources exist:

| Binary | Size | sha256 | Status |
|---|---|---|---|
| `tests/r10/FPJOB.aout` (tested fixture, in the repository) | 8,601 | `114ed367…` | **Used by every R11A and R11B run.** |
| `fpu-diag/package/bin/FPJOB.aout` (R10 package) | 7,842 | `e8252557…` | Different binary; not used. |
| Rebuild of `fpu-diag/package/src/fpjob.c` with the current wrapper | 9,472 | `ed4d2c7d…` | Not the tested binary. |
| Rebuild of `tests/r10/fpjob.c` with the current wrapper | 11,415 | `239e5833…` | Not the tested binary. |

The two sources are different programs (the repository version has R10 round
structure and a different job set). Neither source reproduces the tested
binary, so **FPJOB is an external frozen fixture**. It is not replaced silently.
Its behaviour is checked by its own PASS banner in every run.

## Harness and build notes

- `scripts/r11a-volume.sh` builds the volume (name `win1-r11b.img`, script name
  kept from R11A). `scripts/r11b-run-all.sh` runs the matrix.
- `scripts/r11a-qemu-smoke.sh` now runs the R11B programs and types the CONSOL input.
- `scripts/r11b-symbols.py` generates the symbol inventory in
  `docs/R11B-LIBC-MAP.md`.
- The R6 and R7 logs in `logs/r6` and `logs/r7` are tracked historical files.
  The new logs are kept under `logs/r11b/r6` and `logs/r11b/r7`, and the tracked
  files are restored with `git checkout`.

## Known limits and not done

- **No file writes.** Write modes fail with `NULL`. This is a kernel and FAT16
  limit, not a libc one.
- **Stdin on QEMU** was exercised only by scripted typing. A human session has
  not been recorded.
- **Preemption during console input** is covered only indirectly: CONSOL waits
  with `qd_yield`, and FPJOB and REPEAT run under the timer. No test starts a
  second job that takes the CPU while CONSOL waits.
- **Invalid FILE pointers** are not detected. The functions return `EOF`/`NULL`
  for `NULL` only.
- **Channel busy** is not tested (the kernel's channel table behaviour under
  load was not exercised).
- **File names** are still limited to 10 characters with the dot.
- **`strtod`, `atof`, `strerror`, `errno`, `scanf`, `getenv`, `rand`, `system`**
  and the rest of the list in `docs/R11B-LIBC-MAP.md` are not provided.
- **C68 licence and redistribution.** See `docs/R11B-LICENSE-PROVENANCE.md`.
  LGTEST4 and FPJOB redistribution needs the owner's confirmation.
- **Libm and float printf** belong to R11C (`docs/R11A-MATH-ROADMAP.md`).
