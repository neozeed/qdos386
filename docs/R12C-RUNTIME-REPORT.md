# R12C runtime report — task selector, program titles, SYNC

Base: frozen R12B `27a5d49d9ba6f8f9436d392feb668efbdaa7b552` (archive `af43204b…`). Branch `r12c-task-sync`. The exact
final commit and the archive SHA-256 are in `PACKAGE.md` and the delivery note. Kernel under test (all final evidence):
QEMU `build/r12c/pc-vga/r12c-vga.img`, headless `build/r12c/pc/r12c.img`; hashes in `logs/r12c/final/binaries.sha256`,
and the kernel image every emulator run actually copied in `logs/r12c/final/kernels-used.txt`.

**Everything runs at CPL 0. Nothing in R12C is security isolation.**

## 1. Result

All final evidence comes from runs on the final build (`logs/r12c/final/`). The per-stage runs that first passed each
gate are kept in `logs/r12c/stage1..4/`; they used an earlier kernel in which SYSTASK was a job of its own (see §6).

| Gate | Content | Result |
|---|---|---|
| Phase 0 | audit (jobs/consoles, a.out weak symbols, FAT/IDE/Tiny386 write path), baseline smoke | done (`docs/R12C-AUDIT.md`) |
| 1 | global task selector: shell, EDIT, two BASICs (one in `EXEC_W` with a C child waiting for input), Phoon, a counter, disposable jobs; every job listed; select and return; end a job, the rest go on; focus, screen and cursor restored; stale-id race; no input leakage | **PASS (49/49)** |
| 2 | titles: `Hack-1.03` from `_prog_name`, `PHOON.AOU`, runtime titles changing, R12B-crt0 binary titled without relink, frozen R12B binaries unchanged | **PASS (20/20)** |
| 3 | real SYNC: writes + SYNC + SIGKILL + restart on QEMU and Tiny386, FAT copies/fsck/walker clean, injected flush failures reported, negative no-SYNC kills recorded, Tiny386 shutdown intact | **PASS (44/44)** |
| 4 | periodic SYNC: off by default, set/changed/turned off, bad values refused, heavy writes, a job exiting, editor and BASIC responsive | **PASS (15/15)** |
| 5 | integrated QEMU demo (two boots) | **PASS (20/20)** |
| 5 | one Tiny386 smoke (selector via `TASKS`, titles, SYNC, periodic SYNC, 0x8900 shutdown) | **PASS (11/11)** |
| 5 | regressions (§7), frozen images (§8), package (§9) | see below |

## 2. Evidence classes

* **Guest, QEMU full machine** (QEMU 8.2, VGA, PS/2 keyboard, IDE; scripted keys through the monitor; 80x25 text
  buffer capture; `%screendump` pictures; R12C additions to the harness: `%cursor` reads the VGA CRTC cursor with the
  guest stopped, `%kill` SIGKILLs QEMU, `R12_HDB_PREFIX` puts `blkdebug` in front of the volume).
* **Guest, Tiny386** (headless, scripted terminal input; the R12C emulator with `patches/tiny386-r12c-flush.patch`;
  the unpatched R11M-FIX1 emulator for the "no flush" case).
* **Host**: the symbol-table title reader (`tests/r12c/run-host-progname.sh`, kernel `aout.c` compiled for the host);
  strict C89 syntax check of the new kernel files.
* **Independent**: mtools reads of the volumes (byte comparison of every written file), `fsck.fat -n`, the VFAT
  walker, FAT-copy comparison (`scripts/fix1-check-image.sh`), hashes of images that must not change.

## 3. Stage 1 — task selector (`logs/r12c/final/s1`)

One QEMU boot (`tests/r12c/run-s1.sh`). Started: EDIT on console 2, BASIC A on 1 (then `EXEC_W "CHILDT", "a 0 57
read", 1`: BASIC waits, its child waits for a line), BASIC B on 3 (then `EXEC "TITLET", "launch 12000 PHOON …"`: the
launcher, titled `Hack-1.03`, holds Phoon so that it stays listed, then releases it), `COUNTER 4000 25` on 3 and a
disposable `TITLET hold … Disposable` on 0. `TASKS /L`: 11 jobs (SYSTEM K, SHELL.AOU P, IDLE K and the 8 programs),
consoles 1/3/0 marked shared.

Ctrl+Esc from the shell: the selector lists all 11 jobs with titles, programs, states (BASIC A `SLEEP` in `EXEC_W`,
CHILDT, Phoon `HELD`), consoles and parents; the cursor is hidden; the list refreshes while open (the disposable job's
title changes between two screens); Up/Down/PgUp/PgDn move by tagged id; `zzz` typed into it never reaches the shell;
Esc restores console 0 cell for cell and the hardware cursor at (24,7). Over BASIC A (in `EXEC_W`, child waiting):
opened, `qqq` typed, Esc → console 1 identical, cursor (3,0); `hello g1` then reaches the child exactly (`CHILDT a got
'hello g1'`), the child ends and BASIC returns from `EXEC_W`. Home + Down x3 selects EDIT; Enter shows console 2; `xyz`
typed in the selector is not in EDIT, `abc` typed after is. End selects the job started last; F1 asks, N keeps it, F1 +
Y ends it ("job 65546 ended; the other jobs go on"; the row goes, console 0 no longer shared). SYSTEM and the shell
are refused as protected. **Stale id:** `TITLET child 1200 Shortlived` reuses a freed slot with a new tag (131078); it
is selected, F1 asks, the job ends by itself while the question is open → "job 131078 has ended" and nothing selected;
Y → "job 131078 no longer exists: nothing was ended", all other rows still there. Ctrl+Esc while open closes it;
Alt+F4 while open closes it and shows console 3; the shell's `TASKS` opens it and Esc returns to the prompt. The
counter kept counting (468 → 496), Phoon was released and ended 0, and after `KILL /CON:1..3` only the 3 boot jobs remain.

## 4. Stage 2 — titles (`logs/r12c/final/s2`)

`TITLET.AOU` matrix, **16/16**: compiled title `Hack-1.03` from `_prog_name`; executable name kept separately; the
v2 task list shows source "compiled"; runtime set (17 bytes); `""` and `NULL` clear it; a 100-byte title is cut at 63;
control and non-ASCII bytes become `?`; a title in kernel memory is refused -14 and the old title kept; truncated reads;
a buffer in kernel memory -14; two copies with different runtime titles (same program, parent recorded); 12 rapid
exit/reuse rounds (a held ENVT in a reused slot always shows `ENVT.AOU`, never an old title).

`TASKS /L` and the selector with OLDT, two TITLETs, the Phoon launcher and SYNCT running: `Hack-1.03` (C),
`Copy two tick N` (R), `PHOON.AOU` (-, held), **`Old-Hack 0.9` (C) for OLDT, linked with the R12B crt0 — titled from
its symbol table, no relink**, `SYNC test` (C). A second listing shows the runtime titles moving (tick 30 → 36, 23 →
29). The 28 frozen R12B binaries (hashes as in the R12B package) run unchanged on the R12C kernel from `/R12B`: ENVT
matrix PASS, SPAWNT PASS (with its frozen CHILDT children), PINFO, LEAK, PHOON (status 0); their titles are their
program names. Host: the reader returns `Hack-1.03`, `Old-Hack 0.9`, `SYNC test`, `""` for programs with the SDK's
empty `_prog_name`, none for frozen binaries.

## 5. Stage 3 — SYNC (`logs/r12c/final/s3`, `docs/R12C-SYNC-DURABILITY.md`)

QEMU q1: `MD`, `COPY` to a VFAT long name, `COPY` + `COPY /Y` (overwrite shorter), `REN` to a long name, `DEL`, and
`SYNCT hold` writing 300 lines with raw channel writes and keeping the file open (its size pending in the directory
entry). `SYNC: done (1 open file entries written, drive cache flushed)`; the SYNCT API matrix PASS; later commands
answered (the emulator keeps running); **SIGKILL**. Host: FAT copies identical, `fsck.fat -n` clean, VFAT walker
clean; every file has exactly the bytes written; the open file all 2,592 bytes. q3 restart on that image: `DIR`
shows the long names, `TYPE` reads them (including a case-insensitive long name), the old and deleted names are gone.
q2 negative (same writes, no SYNC, SIGKILL): the open file shows 2,565 of 2,592 bytes (its size as of its last cluster
allocation); the closed files are complete. q4 `blkdebug` (flush_to_disk, EIO, once): `SYNC: FAILED (code -96) -
FLUSH CACHE error (error register 4: aborted)`, counted; the next `SYNC` succeeds.

Tiny386 (R12C emulator): t1 the same writes + `SYNC` + SIGKILL → clean, complete; t2 without SYNC → the same 2,565
bytes, image consistent (evidence); t3 `TINY386_FLUSH_FAIL=1` → stderr `tiny386: flush 1: fsync failed (injected)`,
guest `SYNC: FAILED (code -96) - FLUSH CACHE error (error register 64)`, the next SYNC ok, SYNCT `rc=0`, then the
0x8900 shutdown flushes and closes the image (exit 0) and the image is clean; t4 the same at the `fflush` step; t5 the
unpatched R11M-FIX1 emulator: `SYNC` fails (error register 4) instead of pretending, shutdown still correct.

## 6. Stage 4 — periodic SYNC (`logs/r12c/final/s4`)

Boot: interval 0, no periodic sync. `SET SYNC_INTERVAL=2` → on; 4 periodic syncs in ~7 s. `SYNC /INTERVAL:1` → 13
after ~5 s more. With EDIT, BASIC and `SYNCT heavy 4 64 4` (4 rounds of 4 x 64 KiB written and read back, 0 bad bytes)
running: EDIT accepted and saved a typed line, BASIC answered `PRINT 6*7`, periodic syncs went on (51, none failed).
A job that wrote and closed a file while syncs ran: no failure, its 100 lines on the volume. `SET SYNC_INTERVAL=abc`
and `SYNC /INTERVAL:4000` refused, setting unchanged; `UNSET SYNC_INTERVAL` → off, no periodic sync in the next ~4 s.
85 syncs in all, 0 failed, 0 device errors; the final image is clean. Periodic sync never runs in an interrupt
handler: SYSTASK's step runs in the SYSTEM job (one call at a time).

**Design change during Stage 5.** SYSTASK was first a kernel job of its own. The frozen R12A Gate 1 regression on that
kernel failed one check (19/20): its frame-exhaustion test starts BIGBSS copies until the kernel refuses with "no free
page frames", and with one job slot fewer the job table ran out first. SYSTASK now runs in the SYSTEM job's loop (which
already wakes every tick to watch the shell); the job table available to programs is R12B's again, and all final
evidence is from that kernel.

## 7. Stage 5 — demo, Tiny386, regressions

**Integrated demo** (`logs/r12c/final/demo`, pictures `demo-selector.png`, `demo-edit.png`, `demo-ended.png`): EDIT
writes two lines and saves them as `/HOME/My R12C notes.txt`; two BASICs, a counter and `TITLET hold … "Demo game"`;
Ctrl+Esc over BASIC B lists the 8 jobs (console 3 shared); Home + Down x3 + Enter shows EDIT; Ctrl+Esc, Esc back to
EDIT unchanged; the shell's `TASKS`, End, F1, Y ends the game; `TASKS /L` 7 jobs; `DIR /HOME` shows the long name;
`SYNC`; BASIC A still answers; SIGKILL. The image is clean and the file holds exactly the typed text. Boot 2 on the
killed image: `DIR` and `TYPE` show it, a fresh job table (3) and fresh SYNC counters.

**Tiny386 smoke** (`logs/r12c/final/tiny`): `TASKS /L` 3 jobs; TITLET 16/16; `Tiny game` (R) and `SYNC test` (C)
listed; `TASKS` puts the selector on the diagnostic stream (`TM|` lines with the titles), Alt+1 closes it; `SYNC`
writes the pending entry and flushes the image; SYNCT PASS; periodic SYNC every second without failures; `RUN
SHUTDOWN` with periodic SYNC on → images flushed and closed, emulator exit 0; image clean.

**Regressions on the R12C kernel** (`tests/r12c/run-regressions.sh`, `logs/r12c/final/reg`):

| Suite | Binaries | Result |
|---|---|---|
| R12B Gate 1 (environment) | frozen R12B guest programs and `shell-b` | **PASS 50/50** |
| R12B Gate 2 (layout, PATH, STARTUP.CMD, scripts) | frozen R12B | **PASS 61/61** |
| R12B Gate 3 (SuperBASIC SAVE/LOAD, reboot) | frozen R12B | **PASS 39/39** |
| R12B Gate 4 (SuperBASIC EXEC/EXEC_W) | frozen R12B | **PASS 25/25** |
| R12B integrated demo (two boots) | frozen R12B | **PASS 30/30** |
| R12A Gate 1 (same-origin processes, frame exhaustion) | frozen R12A | **PASS 20/20** |
| R12A Gate 2 | frozen R12A | **PASS 24/24** |
| R12A Gate 3 | frozen R12A | **PASS 35/35** |
| historical NEG / MULTI / PGRUN / TWICET | frozen R12A | **identical to the R12A/R12B baseline (14/14)**: NEG 848 checks with the same 3 fixed-slot contention failures, MULTI FAIL, PGRUN DONE, TWICET M F W PASS and H E N S Z FAIL, the same deliberate fault reports (fixed-slot expectations, `docs/R12A-RUNTIME-REPORT.md` §5) |
| R12B Gates 3 and 4 | the R12C builds (relinked with the R12C crt0: BASIC, BASTEST, EDIT, ...) | **PASS 39/39, 25/25** |
| R12C shell and programs on the **R12B** kernel | R12C builds | **PASS 8/8**: TITLET runs to its end with its title checks failing cleanly, SYNCT `rc=-1`, OLDT normal, shell `SYNC` reports "no SYNC service" (no false success), ENVT/LEAK as before |
| R12 suites (Stages 1-4, the 103-program SuperBASIC corpus) | — | **NA (not run)**: R12C does not change the interpreter or the console paths those suites test beyond what the R12A/R12B suites above cover; the corpus run takes hours |

The first R12A Gate 1 run on the earlier kernel (SYSTASK as its own job) failed 1 of 20 checks; see §6. The first
compatibility run failed 1 of 8 because its test volume lacked `OLDT.AOU` ("no such file"); the volume script was fixed
and the run repeated (8/8). Summary: `logs/r12c/final/reg.out`.

## 8. Images that must not change (`logs/r12c/final/default-images.txt`)

Rebuilt from this tree: R12B `r12b-vga.img` `d22fd9f1…`, `r12b.img` `01ebca11…`; R12A `257460b9…`, `0b23ab9d…`; R12
`1e00386b…`, `dd11fe87…`; shells `shell-b` `dc50e084…`, `shell-a` `9ed34a73…`, `shell-m` `36720c5b…` — all identical to
the R12B package; `scripts/r11l-default-check.sh`: 6 IDENTICAL. The R12C code is behind `QDOS_TITLES`, `QDOS_SYNC`
and `QDOS_TASKMGR` (kernel) and `R12C_SHELL` (shell); the SDK change (crt0 calls `qd_prog_title`; two new library
members) only changes programs relinked with this SDK.

## 9. Package

`qdos386-R12C-TASK-MANAGER-SYNC.tar.gz` (`scripts/r12c-package.sh`): the source tree at the final commit, the R12C
kernels and shell, the unchanged R12B/R12A/R12 kernels and shells, the R12C guest programs, the frozen R12B and R12A
binaries used by the regressions, the example system volume `volumes/r12c-system.img`, the Tiny386 R12C emulator
(changed sources and binaries), firmware, `logs/r12c/` (stage and final transcripts, judge output, screen pictures,
Tiny386 evidence, before/after image checks, hashes) and `SHA256SUMS`. `tests/r12c/verify-package.sh` checks a fresh
extraction: manifest, a QEMU smoke from the extracted kernel and volume, and a rebuild from the extracted source with
hash comparison (`logs/r12c/final/package-verify.txt`).

Verification of the package built from the Stage 5 commit (`logs/r12c/final/package-verify.txt`): manifest OK
(2,881 files); the fresh-extract QEMU smoke boots the extracted R12C kernel and example volume (`SYSTASK: task selector
on Ctrl+Esc`, `TASKS /L` 3 jobs, TITLET 16/16, SYNCT 13/13, `SYNC: done`, ENVT, LEAK); the rebuild from the extracted
source is **byte-identical** for both R12C kernels (`50ab9a6e…`, `fc4a1368…`), `shell-c` (`93b87d14…`), both R12B
kernels, both R12A kernels, `shell-b`, `shell-a`, and the guest programs TITLET, SYNCT, ENVT, BASIC, EDIT. The delivered
archive is built again from the final commit (this report included) and verified the same way; its SHA-256 is in the
delivery note.
