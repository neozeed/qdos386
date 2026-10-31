# R9 runtime report

R9 adds a 16 MiB i386VM configuration, a 1 MiB child slot, a small C89 libc
(`libc.a`), and test applications that run as ordinary QDOS jobs. This report
records what was run, on which images, and what passed.

## Labels

- **SCRIPTED (i386VM)**: the bytes of each session are piped to the i386VM
  harness through its scripted input register (`0x800002`, status `0x800004`).
  This is not human keyboard input.
- **PS/2 via -term (Tiny386)**: `tests/interactive/session.py` drives the
  Tiny386 PS/2 keyboard path through the `-term` interface. It is scripted
  through that path, not typed by a person.
- **EOF (i386VM)**: a short session in which input ends at once; it checks the
  end-of-input behaviour.

No human keyboard testing was done in R9. The R9 claims are about scripted and
programmatic runs only.

## Environment

Hashes are SHA-256 (first 16 hex digits shown). Full lists are in
`logs/r9/matrix/images.sha256` and `logs/r9/matrix/examples.sha256`.

| Item | SHA-256 |
|---|---|
| i386VM harness, stock + R7 input patch, 8 MiB (R8 regression) | `2d2f8beb…` |
| i386VM harness, stock + R7 + R9 RAM patch, 16 MiB (R9) | `a4724f79…` |
| `tiny386_headless` (unmodified) | `fb63b405…` |
| `bios.bin` | `7bd96841…` |
| `vgabios.bin` | `f5aedb52…` |
| `build/fat16/win1.img` (R9 build) | `81dd402a…` |
| `build/fat16/win1-r8.img` (R9 build, R8 volume) | `16f6e3a7…` |
| `build/fat16/win1-r9.img` (R9 volume, 23 entries) | `9fabc3d1…` |
| `build/i386vm/qdos386-r6.bin` (R8 regression suite image, R9 build) | `71e6e83e…` |
| `build/i386vm-r8/qdos386-r8-shell.bin` (R9 build) | `53d3646b…` |
| `build/pc-r8/r8shell.img` (R9 build) | `0c2286b3…` |
| `build/i386vm-r9t/qdos386-r9-suite.bin` (R9 suite, R7 volume) | `cfd118cd…` |
| `build/pc-r9t/r9suite.img` | `811c9dc6…` |
| `build/i386vm-r9/qdos386-r9-shell.bin` | `202d915b…` |
| `build/pc-r9/r9shell.img` | `8cfc551b…` |
| `build/pc-r9/boot.bin` (PC boot sector, transfer at `0x340000`) | `041d8ae3…` |
| `build/i386vm-r9-notimer/qdos386-r9-shell-notimer.bin` | `9a2901c1…` |

Toolchain (historical tree, not in the repository): `bin/i386-gcc`
`3567f3be…`, `bin/i386-ld` `dfdb3965…`, `bin/as` `47369dd5…`.

Example applications (`sdk/examples/*.aout`, see `examples.sha256`):
`BIGAPP` `f5dd784a…`, `CATFIL` `610035a6…`, `HEAPEX` `991eb14d…`, `LIBHEL`
`caf4059a…`, `MEMSTR` `b3561d41…`.

### Rebuilt images versus the frozen R8 archive

The matrix used images rebuilt from the R9 sources. The R8 archive
(`qdos386-R8.tar.gz`) holds its own `win1.img` (`2dab8fd5…`), `win1-r8.img`
(`dfbcf562…`), kernel images and shell, and those differ from the rebuilt ones.
That is expected: the R9 changes touched `kernel/aout.c`, `kernel/appmap.h`,
`apps/shell/shell.c` and `tests/test_r8.c`, which go into the R5–R8 images
too. The archive itself was not changed. It is run separately below as a frozen
baseline.

The FAT16 volumes were hashed before and after the matrix and were unchanged
(`logs/r9/matrix/fat16-before.sha256`, `fat16-after.sha256`).

## Matrix

`scripts/r9-matrix.sh` with `ROUNDS=3`. Summary: `logs/r9-matrix.txt`;
per-run logs: `logs/r9/matrix/`.

| Check | Round 1 | Round 2 | Round 3 |
|---|---|---|---|
| R8 regression, i386VM kernel suite (8 MiB) | PASS, 204 CHECK, 27 R8, 0 FAIL | same | same |
| R8 regression, Tiny386 kernel suite | PASS, same counts | same | same |
| R8 regression, i386VM SCRIPTED session | PASS, 22/22 | same | same |
| R8 regression, Tiny386 PS/2 via -term | PASS, 22/22 | same | same |
| R8 regression, i386VM EOF session | PASS, 2/2 | same | same |
| R9, i386VM kernel suite (16 MiB) | PASS, 211 CHECK, 27 R8, 0 FAIL | same | same |
| R9, Tiny386 kernel suite | PASS, same counts | same | same |
| R9, i386VM SCRIPTED session | PASS, 29/29 | same | same |
| R9, Tiny386 PS/2 via -term | PASS, 29/29 | same | same |
| R9, i386VM EOF session | PASS, 2/2 | same | same |

The suite verdict requires the R4, R5, R6, R7 and R8 banners, zero `FAIL`
lines and at least 20 R8-tagged CHECK lines. The first run of the script
checked only four banners (R8's was dropped while the verdict was being
written); the banner is in every suite log, and the script now requires all
five. All twelve suite logs pass the five-banner verdict.

Timer-off negative (`build/i386vm-r9-notimer`): the shell printed its prompt,
read no input, and the harness stopped at its instruction budget. This shows
that without the timer the shell does not progress. It is a weak negative: the
script checks only that no `23 files` listing appears and the shell did not
exit, and the harness stop is by budget, not by a clean halt.

FAT16 volumes: unchanged across the whole matrix.

### Frozen R8 baseline (archived artifacts, not rebuilt)

Run from the R8 archive. Logs: `logs/r9/frozen-r8/`.

| Artifact | Harness | Result |
|---|---|---|
| `build/i386vm/qdos386-r6.bin` (R8 suite) | R7 8 MiB harness | 204 CHECK lines, 0 FAIL, R4/R5/R7/R8 banners present |
| `build/i386vm-r8/qdos386-r8-shell.bin` | R7 8 MiB harness, SCRIPTED session | PASS, 22/22 |
| `build/pc-r8/r8shell.img` | Tiny386, PS/2 via -term | PASS, 22/22 |

This is the reproducibility check for the 8 MiB R8 baseline: the archived
binaries still pass on the harness they were built for.

## Proof requirements

Each item is a genuine QDOS job run from the R9 volume. Step names are from
`SESSION_R9` in `tests/interactive/session.py`.

| Requirement | Evidence |
|---|---|
| Application larger than 224 KiB | `BIGAPP.aout`: text 4,688 + data 65,616 + BSS 409,600 = 479,904 bytes (72,023 bytes on disk). Runs in slot 1 (`big1`, `big2`). Slot 0 refuses images over 224 KiB (`APP_ERR_LAYOUT`). |
| `malloc`/`free`, strings, `printf` | `MEMSTR.aout` (`memstr`): "MEMSTRESS PASS", two rounds, exit 0. |
| FAT16 file read through the C library | `CATFIL.aout` (`catfil`): reads `README.TXT` twice through `fopen`/`fread`, first line checked; "FILECAT PASS", exit 0. |
| `argc`/`argv` | `libhel3` (`RUN LIBHEL.AOU one "two words"`): argc = 3, argv[0] and argv[2] checked. `args3` checks `ARGS.AOU` the same way. `libhel1` checks argc = 1 with no arguments. |
| Repeated execution without reboot | One session runs `LIBHEL` twice (`libhel3`, `libhel1`), `BIGAPP` twice (`big1`, `big2`) and `HEAPEX` three times (`heapex`, `leak`, `whole`), with no reboot. |
| Exit status and cleanup | Exit statuses are checked per step: `libhel3` 3, `libhel1` 1, `catfil`, `memstr`, `heapex`, `big1` 0. `heapex` (start free) and `whole` (after the leak run) compare the free count; the session's reclaim check requires them to match. |
| Timer-driven preemption continues | `big1` checks "timer ticks advanced during the work: yes", observed inside the job during its long computation. The timer-off negative shows no progress without the timer. |

## Results summary

- R2–R8 regressions pass on the default layout (R8 suite and R8 sessions on
  both targets, three rounds), and the archived R8 artifacts pass on their
  own harness.
- R9 suites and sessions pass on both targets, three rounds, with the scripted
  and PS/2 labels above.
- The R9 kernel suite runs on Tiny386 for the first time in R9 (211 CHECK
  lines, 0 FAIL). Earlier R9 work had verified it only on i386VM.

## Limitations

- **Scripted, not human.** No keyboard input was typed by a person.
- **getchar has no runtime test.** It is compiled and linked into `libc.a`,
  and the code path is present, but no program in `sdk/examples` calls it.
- **Timer preemption is shown by one check, not a dedicated test.** `big1`
  observes timer progress inside a job; no test forces a preemption order.
- **Tiny386 stays at 8 MiB.** The 16 MiB RAM configuration is i386VM only.
- **Per-slot untouched check samples 256 KiB.** `tests/test_app.c` still checks
  only the first 256 KiB of slot 1.
- **Shell banner still says R8.** `apps/shell/shell.c` prints
  "QDOS/386 R8 shell. Type HELP.". It was not changed in R9.
- **Read-only file stdio.** No writes, no `SEEK_END`, no `fprintf`, `fgets`,
  `fputs` or `scanf`; see `docs/R9-LIBC-PORT-MAP.md`.
- **Timer-off negative is weak** (see Matrix).
- **Floating point is not tested** in R9 applications.
- **The emulator change is a patch,** not a fork: `patches/i386vm-r9-ram16.patch`
  applies after `patches/i386vm-input-status.patch` to the stock harness
  (stock `machine.c` SHA-256 `8d92dbad…`). The harness source tree is not in
  the repository; the patch chain was checked to rebuild the R9 harness
  byte-for-byte.
