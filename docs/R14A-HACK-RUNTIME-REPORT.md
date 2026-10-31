# R14A runtime report — Hack 1.03 (the QDOS/C68 port) native on QDOS/386

Branch `r14a-hack-ql-port` from frozen R14 (`43a1f448…`). QEMU 8.2.2 (full machine, R14 VGA kernel **unchanged**) is
the primary platform; one bounded Tiny386 smoke. Logs, transcripts and screenshots: `logs/r14a/`. Status words:
**build verified**, **host tested**, **guest executed**, **differentially verified** (against the original QL binary
under QLBox), **not tested**, **blocked**.

## 1. What R14A delivers

* The supplied QL Hack 1.03 source, **unchanged (0 lines)**, built by an opt-in `Makefile.qdos386` into `HACK.AOU`
  (a.out at 0x240000, title `Hack-1.03` from `_prog_name`), deterministic, 59 library externals, 0 unresolved.
* The C68 interfaces Hack uses, generic and reusable (`docs/R14A-QL-C-ABI.md`): `<ansicondef.h>` as in C68 libc
  4.24.5, a VT100/LibVT-compatible `ANSI_conwrite` (`docs/R14A-LIBVT-ANSI.md`), `getch`, `getlogin`, `kill`, and a C68
  libc variant `libc68.a` (C68's `_conwrite` console vector, blocking console reads, C68's end message, C68's
  full-count file `read`, deleting an open file). `libc.a`, the kernel and every R14 binary are unchanged.
* A QL oracle: the original QL binary under QLBox R20 with a fixed clock (repeatable games), its screens read with
  the QL ROM font, compared with the native screens.

## 2. Results

| Gate | Content | Result | Logs |
| --- | --- | --- | --- |
| 0 | intake: archive hash/listing, pristine tree + 124 hashes, 5 NetHack files withheld, Makefile/conditional/dependency audit, licences, patch proposal | done | `docs/R14A-HACK-INTAKE.md` |
| 1 | two builds byte-identical (HACK.AOU, link map), 0 unresolved, empty source diff, `_prog_name`, origin, libc.a/libqdos.a = R14 | **10/10 PASS** (build verified) | `logs/r14a/gate1/` |
| 2 | host test of `ANSI_conwrite` (49: every Hack sequence, 1/2/3-byte splits, bounds, malformed, 200 000 random bytes; mutation-checked) + ANSIT in two sessions: exact screens/attributes/cursors, keys only to the foreground, three switching rounds (Task Manager, Alt+F1, CON), C68 end message, session release, LEAK | **30/30 PASS** (host tested, guest executed) | `logs/r14a/gate2/` |
| 3 | a real game (controlled seed): character, inventory, combat, drop/pick-up, gold, door, corridor, digging down to level 2, wallet, version, 3 help pages, save, relaunch + restore (map, status, inventory, gold identical), quit, score sheet, record, new game, second quit; 31 checks against the QL binary's screens | **53/53 PASS** (guest executed; 31 differentially verified) | `logs/r14a/gate3/`, `logs/r14a/oracle/` |
| 4 | coexistence and errors | **see §4** | `logs/r14a/gate4/` |
| Tiny386 | Hack in the shell's session: start, inventory, version, save, restore, quit, record, SYNC, LEAK, guest shutdown, clean image | **15/15 PASS** | `logs/r14a/tiny/` |
| R14 identity | 25 frozen images + R14 kernels, libraries and guest programs rebuilt from the R14A source | see §5 | `logs/r14a/identity.txt` |
| Regressions | R14 Gates 1 and 4, R13B Gates 2 and 4, R13A Gate 2 on the R14 kernel | see §5 | `logs/r14a/regressions/` |

## 3. Comparison with the QL build (Gate 3 detail)

Identical (character for character, positions included): the first prompt and the role menu; the greeting "Hello
root, welcome to hack!" (`getlogin` → root on both); the status line format and columns; the inventory lines of the
fixed Speleologist items and the `--More--` column; "You dropped a large box."; the five digging messages and the dig
prompt's form; "Level 2" after falling through; the wallet message form; the version line; "Long or short help?";
**all 23 rows of the three short-help pages** (TAB expansion and pager) with `--More--` in reverse video at row 21;
"Really quit?"; the score sheet's fixed lines, its formats and the reverse-video top-ten entry; C68's "Press a key to
exit"; the record line format — and **Hack 1.0.3's year-2000 record bug**, which garbles the first entry on the second
quit in exactly the same way on both (`  2     12 N SM rootquit-1 quit on dungeon level 0 [max 2].`).

Different (kept, explained in `docs/R14A-KNOWN-LIMITATIONS.md` §1): the QL build loses the pet dog (its monster sits
at 0,0 — RAM dump `logs/r14a/oracle-dog/`), shows garbage object-letter lists in prompts, cannot create
`save/0root` under QLBox's host filesystem, needs `DATA_USE` to find its files, draws in a bordered 85-column window.
Dungeons differ because the two C libraries' `rand()` differ.

Two port defects were found by these comparisons and fixed in the C68 libc variant, not in Hack: (1) the QDOS/386
channel read returns at most 256 bytes, while C68's `read()` returns the whole count for files — Hack's save printed
"Read 256 instead of 3520 bytes." and its save was unusable; (2) the score sheet vanished with the private session at
exit — C68 shows "Press a key to exit" (`_endmsg`), now implemented the same way. Gate 4 found a third: QDOS/386
refuses to delete an open file, so Hack's deletion of a bad save it is reading failed and the bad save stayed for
ever; the C68 variant's `unlink()` now defers the deletion to the next close and to exit.

## 4. Coexistence and errors (Gate 4: **48/48 PASS**, guest executed, `logs/r14a/gate4/`)

Run A (two emulator runs at once, normal timing): Hack in its own session with a second shell, SuperBASIC, QLTerm and
BOUNCE (a graphics session) all running; the Task Manager lists **Hack-1.03**; three tours Hack → SH → Hack → BASIC
→ Hack → QLTerm → Hack → BOUNCE → Hack, plus Alt+F1 / `CON 4` three times: every time Hack's whole screen, the
attributes and the cursor are exactly as before (no key leaked; SH and BASIC received only their own input:
`ECHO tour1..3`, `PRINT 6*7` …). CPU while Hack waits: SPINM counts 948,534 loops per 300 ticks with no game running
and 1,029,141 / 1,026,584 with Hack waiting at its first prompt / in the dungeon — a polling Hack would halve it; it
sleeps in the kernel's console wait. `KILL /CON:4` ends Hack while the others run; a relaunch starts a new game; at
the end every job is gone and LEAK (heap, channels, address spaces) is back to its start value; fsck clean.

Run B (errors): a 100-byte `save/0root` → "Error restoring old game.", C68 end message, the file is deleted (after the
`unlink` fix); a playground with no `data`, `help`, `rumors`, `save/` and an `hh` full of malformed sequences (CSI
999999;999999H, 12 SGR parameters, a high byte and CAN/SUB inside CSI, unknown finals, OSC, a lone ESC at the end of
a line, a 40-digit parameter, save/restore) → the pager shows every text around them, no ESC reaches the screen, the
cursor stays inside 25×80 and the game goes on; "Cannot open help.", "Cannot open data file!", "Cannot open save
file. (Continue or Quit)" (the QL build's message too); bad program paths give the shell's error and no job; Hack
started from `/HOME` runs and cannot save; **disk full while saving** (volume filled to 3 free clusters) → Hack's own
panic "Suddenly, the dungeon collapses. ERROR: cannot write …", the game ends, the OS answers; the partial save is
refused on the next start and deleted; **low memory** (BIGBSS copies hold the frame pool) → `START HACK` gets "out
of memory: no free page frames (code -60)" with no hang, and after `KILL /CON:2` Hack starts again; LEAK back to its
start value; fsck clean after the disk-full run.

## 5. R14 identity and regressions

* `tests/r14a/check-r14-identical.sh` (`logs/r14a/identity.txt`): the 25 frozen R12–R13B kernel and shell images
  rebuilt from the R14A source: **0 differ**; the R14 kernels (QEMU and headless), `libc.a`, `libqdos.a`, `libm.a`,
  `libgcc-qd.a`, QLTerm and the R14 guest programs (UARTPRB, SERTEST, QLPROBE, TERM, XFER, BOUNCE, LEAK, BASIC)
  rebuilt and compared with the R14 delivery archive: **15/15 identical**. The R14A additions live in new files,
  under `-DQDOS_C68_*` in `stdio.c`/`posixio.c` (built only into `libc68.a`), and in a definition-only `ESRCH`.
* Targeted regressions on the R14 kernel, each with its unchanged script and judge (`logs/r14a/regressions/`):

| Suite | Result |
| --- | --- |
| R14 Gate 1 (serial driver, host peer) | 42/42 PASS |
| R14 Gate 4 (QLTerm against lrzsz: X/YMODEM both ways, errors, menus) | 37/37 PASS |
| R13B Gate 2 (program lookup, PATH, START, SuperBASIC EXEC) | 33/33 PASS |
| R13B Gate 4 (CMOS clock, DATE/TIME, Phoon oracle) | 24/24 PASS |
| R13A Gate 2 (rendered text consoles) | 26/26 PASS |

* Tiny386 (R14 headless kernel, R12C emulator, `logs/r14a/tiny/`): Hack in the shell's session — start, character,
  inventory, version, save, restore, quit, record line, save removed, SYNC, LEAK, guest shutdown, clean image:
  **15/15 PASS**. Hack in a private session on Tiny386 is not tested (the harness drives one terminal).

## 6. How to reproduce

    . env (QDOS386_TOOLS=...); make r14-pc; make -C sdk/lib; make -C sdk/libc ../lib/libc68.a
    sh scripts/r14a-build-hack.sh                     # build/r14a/hack/src/HACK.AOU
    sh tests/r14a/run-gate1.sh OUT1 ... run-gate4.sh OUT4
    sh tests/r14a/oracle-ql.sh <QLBox R20 dir> logs/r14a/oracle; sh tests/r14a/run-gate3.sh OUT3
    TINY386=<tiny386_headless> sh tests/r14a/run-tiny.sh OUTT
    sh tests/r14a/check-r14-identical.sh <R14 package dir> LOG; sh tests/r14a/run-regressions.sh OUTR
