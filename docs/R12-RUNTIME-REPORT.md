# R12 runtime report: Interactive Operating System Stretch

Baseline: frozen R11M-FIX1 `6fbb9ff7c8a695d91a368a985a2b1da8b53a9bbc`, branch `r12-interactive`. The exact final source commit and the archive SHA-256 are in `PACKAGE.md` / the delivery message (the archive cannot contain its own hash; `SHA256SUMS` lists every other file).

## Result in one table

| Gate | Content | Result |
|---|---|---|
| 1 | Shell: `DIR /W /P`, command-line editing, history, console API v2 | **PASS** (QEMU, 59/59 checks) |
| 2 | `EDIT.AOU` full-screen editor on its own console | **PASS** (QEMU, 62/62 checks) |
| 3 | Sessions: focus switching, three concurrent paged apps, `CON`, `KILL /CON`, `WAIT` cancel | **PASS** (QEMU, 30/30 checks) |
| 4 | Real SuperBASIC (SB4 core) as `BASIC.AOU` | **PASS** (QEMU, 58/58 interactive + 19/19 corpus checks) |
| 5 | Graphics proof | **Deferred, not claimed** (`R12-GRAPHICS-POC.md`) |
| Integrated | One QEMU demo + reboot persistence + Tiny386 shutdown smoke | **PASS** (27/27; smoke exit 0) |

R12 is complete for Gates 1-4 and the integrated acceptance; Stage 5 is optional in the plan and is a documented deferral.

## Evidence classes

**Host build:** C89 code cross-built with the historical GCC 1.40/GAS SDK; SuperBASIC core verified byte-identical to the supplied files except `sb_state.h` (`apps/superbasic/verify-core.sh`, `logs/r12/stage4/verify-core.txt`).

**Guest executed (QEMU full machine, VGA + keyboard + IDE, scripted keyboard injection, screen capture with completion markers):**
* Stage 1: `tests/r12/run-s1.sh` / `judge_s1.py`, `logs/r12/stage1/` and re-run on the R12 kernel in `logs/r12/integrated/matrix/m1*`.
* Stage 2: `run-s2.sh` / `judge_s2.py`, `logs/r12/stage2/`, re-run in `integrated/matrix/m2*` (62/62).
* Stage 3: `run-s3.sh` / `judge_s3.py`, `logs/r12/stage3/`, re-run in `integrated/matrix/m3*` (30/30).
* Stage 4: `run-s4.sh` (four parallel QEMU runs) / `judge_s4.py`, `judge_s4_corpus.py`, `logs/r12/stage4/`. In the guest, 103/103 corpus programs and 3/3 examples produce output identical to the supplied `.out` files; leak probes (free heap 667,024, 15 channels, 0 address spaces) identical across launches, kills and sessions.
* Integrated demo: `tests/r12/run-demo.sh` (boot 1, reboot on the resulting image, boot 2) / `judge_demo.py`, `logs/r12/integrated/demo/`: `DIR /W /P`, history, quoted VFAT paths, `START`/`PS`, editor saving a long-name file, BASIC running a supplied program on another console, ticker continuing, focus switching with no lost input, clean termination, reboot persistence, Phoon and SDK utilities (PATHT 68 checks, STRTST 37 checks) still passing.
* Tiny386 (secondary, headless, bounded): `tests/r12/seq-tiny-smoke.txt` via `scripts/fix1-tiny-run.sh`: `DIR /W`, `MD`, `COPY` to a long name, BASIC times table, `START`/`PS`/`KILL`, `LEAK`, then guest `SHUTDOWN.AOU`. Result `exited=1 rc=0`, emulator reports "guest shutdown request; disk images flushed and closed" (`logs/r12/integrated/tiny386/`).

**Independent comparison:** `fsck.fat -n`, mtools reads of the saved files (exact bytes), `tools/r11m-fatwalk.py --vfat --strict` (603 files, 12 directories, 0 errors, 0 warnings on both the demo image and the Tiny386 image), the supplied SuperBASIC host runner on the same corpus files, and the default-image identity check.

**Regression guards:** `scripts/r11l-default-check.sh`: all six default image hashes IDENTICAL to R11M-FIX1 (`logs/r12/integrated/default-check/`). Paging (R11K), writable FAT16 (R11L), subdirectories/VFAT (R11M), virtual consoles, job control and Tiny386 shutdown behaviour are exercised by the Stage 1-4 and demo sequences on the R12 kernel. No CPU core was changed.

## Tiny386 smoke note
The first smoke attempt ended with `rc=killed` (no graceful shutdown) because `SHUTDOWN.AOU` correctly refused while the started ticker was still running; that was a mistake in the test sequence, not in shutdown. The sequence now kills the ticker first. Both attempts left a consistent image; only the second is the passing smoke and its record is the one shipped.

## What the demo run looks like (QEMU)
Shell on console 0; `START EDIT.AOU ... /CON:1`, `START BASIC.AOU /CON:2`, `START TICKER.AOU /CON:3`; Alt+F2..F4 switch; the ticker and BASIC keep running while another console is visible; the editor's text and cursor survive the round trips; after quitting all jobs the focus returns to console 0, the long-name file is on the volume, and it is still there after a reboot.

## Limits
See `R12-KNOWN-LIMITATIONS.md` (includes the NOT TESTED list). Documents: `R12-AUDIT.md`, `R12-SHELL-ABI.md`, `R12-CONSOLE-EDITOR-ABI.md`, `R12-SESSIONS-JOBS.md`, `R12-SUPERBASIC-PORT.md`, `R12-GRAPHICS-POC.md`.
