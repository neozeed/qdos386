# R12B runtime report — system environment and SuperBASIC files/programs

Base: frozen R12A `6e30dd7046b4d09d77ec398bea38a76318b36c52` (package `979825fc…`). Branch `r12b-system-environment`.
The exact final commit and the archive SHA-256 are in `PACKAGE.md` and the delivery note. Kernel under test (all final
evidence): QEMU `build/r12b/pc-vga/r12b-vga.img` SHA-256 `d22fd9f1…`, headless `build/r12b/pc/r12b.img` `01ebca11…`
(`logs/r12b/final/binaries.sha256`).

**Everything runs at CPL 0. Nothing in R12B is security isolation.**

## 1. Result

All final evidence comes from one run on the final build (`tests/r12b/run-final.sh`, `logs/r12b/final/`); the per-stage
runs that first passed each gate are kept in `logs/r12b/stage1..4/`.

| Gate | Content | Result |
|---|---|---|
| Phase 0 | R12A baseline re-verified, R12A demo re-run, audit, ABI | done (R12A demo 23/23) |
| 1 | environment: two concurrent children inherit PATH, one changes its copy without affecting the parent or the sibling, counts back to baseline | **PASS (50/50)** |
| 2 | layout, STARTUP.CMD, PATH search, scripts, BASIC-first boot, broken/never-returning startup cannot lock out the shell | **PASS (61/61)** |
| 3 | SuperBASIC SAVE to `/HOME/My First Program.bas`, NEW, LOAD, LIST, RUN, reboot and LOAD again; corpus unchanged | **PASS (39/39)** |
| 4 | BASIC EXEC Phoon in the background, EXEC_W an interactive C89 child on another console, back in the same program with its status; two interpreters independently | **PASS (25/25)** |
| 5 | integrated demo (two boots), NEGDYN, Tiny386 smoke, regressions, package | see §6-§9 |

## 2. Evidence classes

* **Host** (strict C89, `-Werror`; `sh tests/host/r12b/run.sh`): environment block rules (56 checks), program search
  against a fake volume (30), shell expansion and script lines (31). SB4 corpus with the changed core
  (`tests/host/r12b/sb-core-corpus.sh`): 106 files (103 corpus + 3 examples), 0 failing, **report byte-identical** to the
  original core's.
* **Guest, QEMU full machine** (VGA, PS/2 keyboard, IDE; scripted keys; text-buffer capture; `%screendump` pictures).
* **Guest, Tiny386** (one bounded smoke, guest shutdown through port 0x8900).
* **Independent**: mtools reads of the volumes, `fsck.fat -n`, the VFAT walker, FAT-copy comparison, default-image
  hashes.

## 3. Stage 1 — environment (`logs/r12b/final/s1`)

Two boots of one volume (`tests/r12b/run-s1.sh`). Fresh boot: `(no environment variables)`. `SET PATH=/BIN;/SDK/BIN`
then `SET PATH` reports it; `SET HOME = /HOME` (spaces around `=`), `SET TEMP="/TMP"`, `SET EMPTY=` (empty value, listed
as `EMPTY=`), a quoted value with leading/trailing spaces, `;` and `=` kept byte for byte (a child prints it back); `SET`
lists in name order with upper-case names. `ECHO` expansion: `%PATH%`, `%home%` (case-insensitive), `%%`, a lone `%`,
`"%PATH%"` kept, an unset name gives nothing; `SET P2=%PATH%;/GAMES` appends. Refused: `1BAD`, `A-B`, a 37-character
name, `SET =x`, an unterminated quote; an expansion past 159 characters is not run and the shell goes on.

`ENVT.AOU 2` (C89 `getenv`/`setenv`/`unsetenv` and `qd_env_*`), **107 checks, 0 failed** (also read back from
`ENVT.LOG` on the volume): case rules, empty vs absent, truncated reads, name/value limits (-15, -4), the 32-variable and
2,048-byte limits (-5) with the old entries intact, bad pointers (NULL, kernel memory: -14) with nothing changed, list
order. **Gate 1 (ENVT as parent)**: child B (`hold`) and child A (`mod`) run at once with the same PATH; A changes its
PATH and adds a variable (status 10); B is shown still running after A ended and then ends 20 — its PATH as at its start,
neither A's variable nor one the parent set after both spawns visible; the parent's PATH unchanged; no child records
left; kernel heap and frame pool unchanged; the stale id is refused. 12 sequential and 6 concurrent spawns all see PATH; a
child gets the whole 1,811-byte environment; a spawn whose environment copy fails (diagnostic injection) is refused with
-30 and leaves kernel heap, frames, child records and process count unchanged, the next spawn works.

**Gate 1 (the shell as parent)**: `START ENVT hold …` and `START ENVT mod …` on consoles 1 and 2, then `SET ENVT_LATE=…`:
PS shows B still running while A has ended with 10; the shell's PATH unchanged and A's variable not visible; B ends 20.
LEAK equal before and after the ENVT run and the two children; after `UNSET` of every variable, `SET` is empty and every
count is back to the boot baseline. Boot 2: the environment is empty again (defaults), a child sees no PATH, same
baseline counts.

## 4. Stage 2 — layout, search, scripts, startup (`logs/r12b/final/s2`)

Five boots of one example system volume (`tests/r12b/run-s2.sh`; `fsck.fat` clean after installation). Boot a:
`STARTUP: done (7 commands)` exactly once, its `ECHO PATH=…` before the banner; `SET` shows PATH, HOME, TEMP, TMP,
INCLUDE, LIB. `CD /HOME`, `START EDIT notes.txt /CON:2` (EDIT is only in `/BIN`): typed, saved, ended 0; `TYPE notes.txt`
and the host read the file. A child started by name (`ENVT`, in `/BIN`) reads HOME, TEMP, TMP, INCLUDE, LIB. Search:
`WHO` found in `/SDK/BIN` (PATH order) and, after `SET PATH=/GAMES;/SDK/BIN`, in `/GAMES`; `BADP` (malformed in `/BIN`,
good in `/GAMES`) gives `not a valid a.out image (code -21)` — the search stops; `NOSUCHPROG`: no such file; `libhel`
(root only) runs with `argv[0] = "libhel"`; `"/HOME/T/Long Program Name"` (a `.aou` file) and `envt.aou` run; `START
PHOON …` from `/GAMES`. Scripts: CR LF + blank + REM + TAB + `%NAME%` + missing final newline; an unknown command stops
at line 2 with the later lines not run; ordinary failures (CD, TYPE, RUN) reported and the script goes on; a 170-char
line, a control byte, a 5 KiB script are refused; self-`CALL` stops at depth 4 (every level reports); `SOURCE` with a
nested `CALL` whose `EXIT` ends only the inner script; a missing script reported.

Boot b (BASIC-first `STARTUP.CMD`): BASIC appears on console 1 by itself and answers `PRINT 6*7` = 42; console 0 has the
banner and prompt; PS shows exactly one BASIC; `KILL /CON:1` ends it. Boot c: a startup whose `RUN BASIC` never returns
is skipped with ESC (`STARTUP: skipped (ESC) - safe start, environment empty`). Boot d: a startup with an unknown command
stops at line 3, the shell is usable, the lines before the error took effect and the one after did not. Boot e: the
default startup again. Final volume: fsck and VFAT walker clean.

## 5. Stages 3 and 4 — SuperBASIC (`logs/r12b/final/s3`, `s4`)

**Gate 3** (three boots, `tests/r12b/run-s3.sh`): a six-line program (FOR/NEXT, DATA/RESTORE/READ) typed at the
BASIC prompt, `SAVE "/HOME/My First Program.bas"` (the host reads exactly the LIST text, LF), `NEW` (LIST empty), `LOAD
"My First Program.bas"` (relative to the interpreter's cwd), `LIST` identical, `RUN` (1..25, `data 18`). Refused with the
program unchanged: an unnumbered line (bad line, line 2 named), a control byte, a 300-character line (buffer full), line
number 99999, a 190 KiB file (buffer full), a missing file (not found), a directory (bad name), `LOAD 42` (bad
parameter). Empty file → empty program; `LOAD prog_bas` (bare name); 1-line and 151-line files (`big 149`; SAVE of it
equals the original file); MERGE of a CR LF file with collisions (20 replaced, 25 and 40 added); a bad MERGE refused;
LOAD inside a running program → `not implemented at line 20`; SAVE into `B/Sub Dir/nested copy.bas`; SAVE errors (missing
directory, a directory, empty name, trailing junk — nothing written); empty program saves an empty file; DIR; no
`~SB*` files left; LEAK equal before and after. Boot b: reboot, LOAD the saved file, LIST identical, RUN identical. Boot
c: the volume filled by the host to 1,024 bytes free: `SAVE "B/keep.bas"` → `drive full`; the old `keep.bas` loads and
the host reads it unchanged; no temporary file left; fsck and walker clean. The guest corpus (BASTEST with the changed
core) is in §8.

**Gate 4** (`tests/r12b/run-s4.sh`): BASIC A (console 1) loads and runs a program: `EXEC "PHOON", …` (background, found in
`/GAMES`), `EXEC_W "CHILDT", "a 0 57 read", 2` — console 2 is shown, the child asks for a line, the line typed there is
read (`CHILDT a got 'hello from console 2'`), console 1 comes back, `EXEC_STATUS` = 57, `JOB_WAIT(phoon)` = 0, the next
program line runs. At the same time BASIC B (`/Q B/g4b.bas` on console 3) does `EXEC` (background, 3 s), `EXEC_W`
(status 44), `JOB_WAIT(bg, 0)` = -1 (still running), `JOB_WAIT(bg)` = 33, and quits. EXEC on console 2 leaves console 1
shown. A child that faults (FAULTT NULL store; the kernel's one exception report): EXEC_W returns, EXEC_STATUS -17, BASIC
goes on. JOBS, `JOB_WAIT(k, 10)` = -1, JOB_KILL then -66, collected/unknown ids -2, `JOB_KILL 99999` invalid Job. EXEC_W
of a malformed program: bad parameter; console 9: bad parameter; empty name: bad name; missing program inside a program:
`not found at line 20`. The 8-children limit: the 9th EXEC → invalid Job. An orphan outlives its interpreter and ends by
itself. LEAK equal before the two interpreters and after everything ended.

## 6. Integrated demonstration (`logs/r12b/final/demo`)

Two boots of the example system volume (`tests/r12b/run-demo.sh`, 30/30; screen captures `demo-basic.png`,
`demo-shell.png`): STARTUP.CMD once; `SET` shows the environment; `DIR /` the hierarchy, `DIR /BIN` its programs; EDIT
started by name from `/HOME` writes `hello.bas`; BASIC on console 1 LOADs, LISTs and RUNs it; `EXEC "PHOON", …, 3` in the
background; `EXEC_W "CHILDT", "d 0 61 read", 2` on console 2 (line typed there); back in BASIC `child status 61, phoon 0`;
the shell on console 0 meanwhile (PS, CON). Inherited and isolated environments: the shell sets `DEMO_VAR` after BASIC
started — a shell child sees it; BASIC's EXEC_W child does not, but sees HOME and PATH. One intentional failure per new
subsystem: BASIC `LOAD "missing.bas"` and `EXEC_W "NOSUCHPROG"` (not found), shell `SET 1BAD=x`, `RUN NOSUCHPROG`, `CALL`
of a missing script. BASIC `SAVE "hello2.bas"` (host reads the LIST text), QUIT, focus back on console 0. LEAK (heap,
channels, spaces) and PINFO (free frames) equal before and after. Boot 2: startup again, `hello.bas` still there, BASIC
LOAD + RUN, same baseline. Final volume: fsck clean, VFAT walker clean, both FAT copies identical.

## 7. Dynamic NEG/MULTI equivalents (`logs/r12b/final/negdyn`)

`NEGDYN.AOU 2` on the R12B kernel (`tests/r12b/run-negdyn.sh`), 67 checks over 2 cycles, 0 failed: the ten R11H
malformed images are refused by `qd_spawn` with exactly the R11H codes (BADMAG -21, RELOC -22, BADENT/ENTOFF -25, BADSYM
-23, BIGTXT -24, TRUNC/TRAIL/SHORT/EMPTYX -20), each leaving frames, kernel heap and child records unchanged; the good pair
still exits 3 and 7; two copies of MULTIA and two of MULTIB run at once (exits 3 3 7 7) — the dynamic replacement of the
fixed-slot "slot busy" expectation; 40 spawn/wait cycles, every old id refused afterwards; invalid ids (0, -1, huge,
self, parent) refused; frame pool, kernel heap and child records as at the start; LEAK unchanged. The historical NEG/MULTI/
TWICE transcripts on the R12B kernel are kept separately (§8).

## 8. Regressions on the R12B kernel (`logs/r12b/final/reg`)

`tests/r12b/run-regressions.sh` with `R12_QKERNEL` = the R12B kernel. **Frozen binaries**: the R12A and R12 suites run
the R12A package's guest programs and shells (`guest-programs-r12a/`, hashes checked against the R12A `SHA256SUMS`) — only
the kernel is new.

| Suite | on R12A kernel (R12A report) | on R12B kernel |
|---|---|---|
| R12A Gate 1 (MAPTEST, BIGBSS, frames) | 20/20 | **20/20** |
| R12A Gate 2 (HEAPT, STKT, FAULTT, bad pointers) | 24/24 | **24/24** |
| R12A Gate 3 (SPAWNT, CHILDT, orphans, interactive child) | 35/35 | **35/35** |
| R12A integrated demo (two boots) | 23/23 | **23/23** |
| R12 Stage 1 shell | 59/59 | **59/59** |
| R12 Stage 2 EDIT | 62/62 | **62/62** |
| R12 Stage 3 sessions/jobs | 30/30 | **30/30** |
| R12 Stage 4 BASIC interactive (R12 BASIC) | 58/58 (run b alone) | **58/58 with run b alone**; 57/58 in the four-in-parallel pass (see below) |
| R12 Stage 4 corpus (R12 BASTEST: 103 programs + 3 examples + known files vs host) | 19/19 | **19/19** |
| R12 integrated demo (Phoon, PATHT, STRTST, VFAT, reboot) | 27/27 | **27/27** |
| historical NEG / MULTI / TWICET (fixed-slot expectations) | NEG 848 checks, 3 failures; MULTI FAIL; TWICET M F W PASS, H E N S Z FAIL | **identical** (same 3 NEG contention checks, same MULTI collision, same TWICET cases; explained in R12A-RUNTIME-REPORT §5; replaced by NEGDYN, §7) |

**R12B BASIC** (`BASIC.AOU`/`BASTEST.AOU` with the extension hooks) through the same R12 Stage 4 suite: corpus **19/19**
(guest: 103 of 103 corpus programs, 3 examples and the known-difference files identical to the host oracle), interactive
**58/58 with run b alone**, 57/58 in the parallel pass.

The one parallel-pass failure is the same in both builds and was seen in R12A: run b's 4000×100 BASIC loop is awaited by
a fixed 120-second `%wait` while four emulators share two CPUs; the program finishes later (`done 3999` is in the
transcript) and run b alone passes. Timing of the harness, not a kernel or interpreter fault; recorded, not hidden
(`reg/r12/s4/judge_s4.txt`, `reg/r12/s4/b-rerun/`, same for `new-s4`).

## 9. Tiny386 (`logs/r12b/final/tiny`)

One bounded full-machine smoke (`tests/r12b/run-tiny.sh`): headless R12B kernel `01ebca11…`, 8 MiB, the example system
volume. `STARTUP.CMD` ran (no keyboard: no ESC); PINFO: pool **1,216 frames (4,864 KiB)**, caps `0110001f`; `DIR /W /BIN`;
`ENVT` by name from `/HOME` reads PATH, HOME, TEMP; `SET SMOKE=tiny386` seen by a child and by a CR LF script
(`script: SMOKE=tiny386 HOME=/HOME`); MAPTEST (root fallback) in the background; `BASIC /Q /S B/smoke.bas`: program,
SAVE, NEW, LOAD, RUN, `EXEC_W "CHILDT"` → `smoke: child status 23`; `TYPE` of the saved file; `KILL /CON:2`; LEAK identical
before and after (`heap=800096 chans=15`); `RUN SHUTDOWN` (found in `/BIN`) → `tiny386: guest shutdown request; disk
images flushed and closed`, exit rc 0. Image: both FAT copies identical, `fsck.fat` clean, walker clean (390 files, 18
directories).

## 10. Memory and leaks

| | R12A | R12B |
|---|---|---|
| free kernel heap, idle shell (QEMU, LEAK) | 806,064 B | 799,840 B (−6,224 B: the larger kernel image — environment and path-search code, per-job tables, a 2 KiB edit buffer — and the shell's own environment) |
| one environment | — | one kernel-heap block per job, 256-byte steps, at most 2,048 B |
| shell BSS | — | 4 × 4 KiB script buffers |

Every gate compares LEAK (kernel heap, channel slots, address spaces, paged bytes) and, where it matters, the frame
pool (PINFO, `qd_proc_info`) before and after; all equal (judges). SET followed by UNSET leaves the kernel heap
unchanged (256-byte steps, edited in place).

## 11. Regressions guarded

* Default images: 6 × IDENTICAL (`scripts/r11l-default-check.sh`).
* R12A kernels rebuild byte-identical (`257460b9…`, `0b23ab9d…`); R12 kernels (`dd11fe87…`, `1e00386b…`); R12A and R12
  shells (`shell-a` `9ed34a73…`, `shell-m` `36720c5b…`) byte-identical: every R12B kernel and shell change is under
  `QDOS_ENV` or `R12B_SHELL`. The SB4 core hooks are inert without a table (BASTEST installs none: corpus 19/19). libc's
  `getenv` now calls the trap, so programs relinked against the new libc differ from their R12A builds; the regressions
  therefore run the frozen R12A binaries.
* The frozen R12A guest binaries (`guest-programs-r12a/`) are what the regression suites run.

## 12. Package verification

`tests/r12b/verify-package.sh` on a fresh extraction of the archive (`logs/r12b/final/package-verify.txt`): manifest
(`SHA256SUMS`, 2,566 files) OK; a QEMU smoke from the extracted R12B kernel and example system volume (STARTUP.CMD done,
PINFO, ENVT by PATH, two MAPTEST copies 11/12, a BASIC SAVE/LOAD/EXEC_W smoke with child status 23, LEAK); a rebuild from
the extracted source of both R12B kernels, the R12B shell and ENVT, NEGDYN, BASIC, BASTEST — **byte-identical** to the
shipped files. The check is repeated on the delivered archive (delivery note).

## 13. Not tested

See `R12B-KNOWN-LIMITATIONS.md` §7.
