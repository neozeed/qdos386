# R13A runtime report — graphics usability

Base: frozen R13 `c3683d58e02817570f66919aead2451dd13e51a6` (archive `bac427c6…`, re-checked). Branch
`r13a-graphics-usability`. The exact final commit and the archive SHA-256 are in `PACKAGE.md` and the delivery note.
Kernels under test: QEMU `build/r13a/pc-vga/r13a-vga.img` (`f802435e…`), headless `build/r13a/pc/r13a.img`
(`10788251…`); these, shell-e, GFXA, the programs the tests use and the three font files are hashed in
`logs/r13a/binaries.sha256`.

**Everything runs at CPL 0. Nothing in R13A is security isolation**: the calls keep cooperating programs off the VGA;
they do not stop hostile ring-0 code from touching it.

Verdicts are **PASS**, **FAIL**, **NA** (cannot apply on that machine) and **NOT TESTED** (could apply, not run).

## 1. Result

| Item | Content | Verdict |
|---|---|---|
| Stage 0 | audit: heap on both machines, VGA modes, font bytes in the R13 kernels (`docs/R13A-AUDIT.md`) | done |
| host | font registry 38/38 (kernel parser built for the host), Python and kernel parsers reject the same 10 broken files, every generated font file/table up to date, MINERVA.FNT vs the R13 kernel's Minerva table: 256 codes, 0 differ; surface library 12/12 (512×256, view, colour-key blit, memmove copy, budget); SDK strict C89 | **PASS** |
| Gate 1 | fonts: no Minerva table in either R13A kernel (scanner); Minerva loaded from disk and selected without a rebuild; 10 broken files and a missing file refused, previous font kept; 4 surfaces with 2 fonts at once, every card screendump pixel-exact; reboot with `SET FONT=` in STARTUP.CMD; release/stale ids; no leak | **PASS 44/44** |
| Gate 2 | rendered consoles (mode 12h, 640×480×16): shell, EDIT, SuperBASIC drawn from their cells, every pixel; after a graphics session, after the selector: pixel-identical; Minerva as console font; back to VGA text with the same cells; kernel text checksum = judge's | **PASS 26/26** |
| Gate 3 | 512×256 (fit and crop) + 320×200 (COLOUR, static card) together, switched twice through the Task Manager; every screendump against the model at its frame; private buffers verified at exit; no leak | **PASS 22/22** (1 torn crop dump of 4, allowed and reported) |
| Gate 4 | key events: down/up, Shift/Alt, synthetic ups on focus loss, hotkeys consumed, held key across Alt+F1 and the selector (QMP key-down/up), nothing reaches the wrong session; blit/copy/clip matrix | **PASS 17/17** (+ matrix in Gate 1) |
| Stage 5 | three integrated QEMU cycles in one boot (fonts 8×16 / Minerva / recovery), final image FAT checks | **PASS 60/60**, IMAGE PASS |
| Stage 5 | one Tiny386 smoke (headless kernel, R12C emulator), 0x8900 shutdown | **PASS 15/15** (presentation **NA**) |
| Stage 5 | affected regressions on the R13A kernel: R13 Gates 1–4, R12C Stages 1/2/4, R12B Stages 3/4, R12A Stage 1 (§8) | **PASS** (two first-pass harness-timing FAILs, re-run PASS: §8) |
| Stage 5 | frozen images byte-identical, package verification | §9, §10 |
| — | mouse | **NOT IMPLEMENTED** |
| — | real VGA hardware, other keyboards | **NOT TESTED** |

All official evidence is in `logs/r13a/` (transcripts, judge output, the programs' logs, image checks, every
screendump as PNG: the judges re-run unchanged on that directory and give the same results).

## 2. Evidence classes

* **QEMU full machine** (QEMU 8.2, `-machine pc -cpu pentium -m 16 -vga std`, PS/2 keyboard through the monitor;
  Gate 4 also QMP `input-send-event` for real key-down / key-up with time between). Captures: QEMU screendumps of
  the VGA state (720×400 text, 640×400 for mode 13h, 640×480 for mode 12h), the cells (`%cells`). In mode 12h there
  is no text buffer to read, so the harness reads the **presenter's shadow** (the address the kernel prints at boot):
  the cells the presenter last drew, which the judge then renders itself and compares with the screendump.
* **Recomputed pictures** (`tests/r13a/r13apics.py`): consoles rendered from cells with the font file
  (`fonts/*.FNT`, `MINERVA.FNT`), the 512×256 pattern at the frame its binary counter shows, mapped through fit or
  crop exactly as specified, cards drawn with each font, all through QEMU's 6-bit DAC expansion. A dump passes only if
  every pixel matches (cursor: one cell's last two rows, located and then checked exactly).
* **Programs' own verdicts**: GFXA pattern, COLOUR read their surface and palette back at exit and compare with their
  models (`verify=PASS`); the cards compare checksums before and after the switching; GFXA's matrix (52 checks).
* **Tiny386** (headless R13A kernel, R12C emulator, guest 0x8900 shutdown), **host** tests, **independent** image
  checks (mtools, `fsck.fat -n`, the VFAT walker, FAT-copy comparison).

## 3. Stage 1 — fonts (`logs/r13a/gate1`)

Format and loader: `docs/R13A-FONT-FORMAT.md`; provenance: `docs/R13A-FONT-PROVENANCE.md`. The fallback-only kernels
have two built-in fonts, recovery 6×9 (cell 6×10; X11 `koi6x9`, public domain) and console 8×16 (X11 `koi8x16`,
"may be distributed and modified without restrictions"); `tools/r13a-font/scan-minerva.py` over both kernel images:
**longest run of consecutive Minerva glyphs 0**, 6 single-glyph coincidences, all inside the
public-domain font images, 0 elsewhere. Minerva is `third_party/minerva/MINERVA.FNT` (225 glyphs, QL
mapping, converted from the GPL-2.0 1.98 source modules; host check: identical to the R13 kernel's table for all 256
codes), shipped on the volume as `/SYSTEM/FONTS/MINERVA.FNT` with `MINERVA.TXT`.

GFXA matrix 52/52 (on QEMU and on Tiny386): built-ins listed; missing file refused; the 10 broken fixtures
(`tools/r13a-font/mkbad.py`: bad magic, header CRC, payload CRC, truncated, larger than the 33,344-byte maximum, duplicate map,
bits outside the width, glyph outside its cell, empty, map index out of range) all refused −4 and the font table
unchanged; kernel-memory pointers −14; Minerva opens, its glyphs drawn exactly as in the file; release while selected
keeps it alive, then its id goes stale; the kernel keeps its own copy (the user buffer overwritten after loading
changes nothing); every loaded font freed at the end. Shell: `FONT /SYSTEM/FONTS/MINERVA.FNT /SURF` loads it; broken
and missing files refused with the previous font kept. Four cards ran at once with recovery, Minerva, Minerva,
recovery: each screendump pixel-identical to the card recomputed with its own font, checksums unchanged across the
switching. Second boot with `SET FONT=/SYSTEM/FONTS/MINERVA.FNT` appended to `/ETC/STARTUP.CMD`: loaded at boot as
default for surface text and consoles; a new card draws Minerva with no FONT command (pixel-exact). `LEAK`
726,304 → 726,304.

## 4. Stage 2 — rendered text consoles (`logs/r13a/gate2`)

`CONSOLE GFX /ALL`: text sessions are drawn in mode 12h from their cells with the session font, else the console
default, else recovery, centred (8×16: 640×400 at y 40; 6×10: 480×250 at (80, 115)). Applications are unchanged: the
shell, EDIT and SuperBASIC write cells exactly as in R13. Results: shell, EDIT (after typing and Ctrl+S), SuperBASIC
(`PRINT "R13A BASIC ";7*6`) and the shell again — **every pixel equals the cells rendered by the judge**, plus exactly
one cursor. After COLOUR (mode 13h) and after opening and closing the Task Manager (mode 3), EDIT and BASIC are
pixel-identical to before and their cells unchanged. The kernel's text checksum (`GFXA tsum`) equals the judge's
for EDIT's and BASIC's sessions. Minerva as console font: 6×10 cells, exact. `CONSOLE VGA /ALL`: VGA text mode again
with the same cells in `0xB8000`; EDIT's cells identical to its rendered ones. The file EDIT saved while rendered is
exact. `LEAK` back to 726,304.

## 5. Stage 3 — logical resolutions (`logs/r13a/gate3`)

Two GFXA patterns 512×256 (one fit: 320×160 at y 20, nearest neighbour; one crop at (96, 0)), COLOUR 320×200 and a
static SURFT card 320×200 at once (4 INDEX8 surfaces, 2×131,072 + 2×64,000 pixel bytes, none refused). Two rounds
through the Task Manager, two screendumps each: fit 4/4 exact; crop 3/4 exact, one dump 1,064 pixels off in the bar
region (the presenter was mid-copy: no vsync; allowed 1 of 4); COLOUR 4/4; card 4/4. Frame counters advance while
hidden (e.g. fit 1,453 → 1,558 → 3,742 → 3,845). At exit both patterns' private buffers and palettes equal their
models (verify PASS, frames shown and hidden), COLOUR verify PASS, the card's checksum unchanged. `LEAK` back to
726,304. (The first official run of this gate reported 21/22: the judge took the pattern lines from de-duplicated
console rows and lost the wrapped `=PASS` of the second; the judge now reads each log screen whole, the run was
repeated and this is its result.)

## 6. Stage 4 — key events and blits (`logs/r13a/gate4`, matrix in gate1)

GFXA keys (session 4) beside a text panel (5) and COLOUR (6). Events seen: `a`, `b` down/up with characters; Shift
down, `B` down with character `B` and the Shift modifier, ups; `a` held (QMP key-down), Alt down; Alt+F1 moves the
focus → synthetic ups for `a` and Alt, nothing for F1; the physical release of `a` later never arrives; `x`, `c`
held across Ctrl+Esc/Esc went to the panel (its console bytes exactly `xc`) and never to GFXA; `z` held then
released after the focus came back: down, then up; no key left down. The screen showed each held key's box lit.
COLOUR kept animating (verify PASS). Blit/copy/clip (matrix): colour-key blit keeps key pixels, both overlap
directions equal memmove, clipping and −14 for kernel pointers, a 4th 512×256 refused by the pixel budget, all
freed. Mouse: NOT IMPLEMENTED.

## 7. Stage 5 — integrated cycles and Tiny386

**QEMU, three cycles in one boot** (`logs/r13a/demo`, `tests/r13a/seq-demo.txt`): consoles rendered throughout;
console font 8×16 (cycle 1), Minerva loaded from the disk (2), recovery 6×10 (3). Each cycle runs EDIT, SuperBASIC,
COLOUR and a 512×256 pattern at once; the Task Manager (VGA text, all four listed); shell, EDIT, BASIC and EDIT again
pixel-exact from their cells with that cycle's font (12/12); COLOUR 6/6 and the fit pattern 6/6 screendumps exact
with advancing counters; COLOUR and pattern verify PASS with frames drawn while hidden; D_<n>.TXT saved; everything
ended; LEAK 726,304 at the end of cycles 1 and 3 and at the end, 723,648 at the end of cycle 2 (the shell still held
Minerva, 2,656 bytes, until `FONT /FREE`). Back to VGA text, SYNC, final image FAT copies / fsck / VFAT walker
clean: **60/60**.

**Tiny386** (`logs/r13a/tiny386`): built-in fonts listed; GFXA matrix 52/52; Minerva loaded from disk and listed as
surface default; broken file refused; `CONSOLE GFX` refused ("no rendered text on this display") — presentation NA
on the headless kernel; a 512×256 pattern (400 frames, all hidden, verify PASS) and a Minerva card (checksum stable)
as GFX jobs in TASKS; Minerva freed after `FONT /FREE`; SYNC; LEAK 741,744 → 741,744; 0x8900 shutdown, emulator
exit 0, image clean: **15/15**. (The first run of the sequence used a QEMU-only key directive and a pattern short
enough to end before `TASKS /L`; the sequence was corrected and re-run.)

## 8. Regressions on the R13A kernel

Only the affected suites, each with its own unchanged script and judge, R13 volumes with the R13 shell-d (the R13
judges take the surface font from the boot line: recovery on R13A, Minerva on R13):

| Suite | What it covers | First pass (harness probing the shadow) | Verdict |
|---|---|---|---|
| R13 Gate 1 | private sessions, selector, cells/cursor, API matrix | 93/93 | **PASS** |
| R13 Gate 2 | presenter text ↔ 320×200, cards pixel-exact (recovery font) | 45/45 | **PASS** |
| R13 Gate 3 | selector over shell, BASIC + EXEC_W child, EDIT, COLOUR, BOUNCE | 46/46 | **PASS** |
| R13 Gate 4 | graphics SDK, COLOUR + BOUNCE, 4 surfaces + refused 5th, teardown | 49/49 | **PASS** |
| R12C Stage 1 | Task Manager, titles, end task | 48/49 (Phoon's "ended with status 0" line never captured) | **PASS 49/49** on the re-run without probing |
| R12C Stage 2 | program titles | 20/20 | **PASS** |
| R12C Stage 4 | periodic SYNC | 14/15 ("2..4 periodic syncs in ~7 s": 5), again 14/15 (5) | **PASS 15/15** on the re-run without probing (4) |
| R12B Stage 3 | SuperBASIC LOAD/SAVE/MERGE with EDIT | 39/39 | **PASS** |
| R12B Stage 4 | SuperBASIC EXEC/EXEC_W | 25/25 | **PASS** |
| R12A Stage 1 | dynamic address spaces / process memory | 20/20 | **PASS** |

The two first-pass failures are harness timing, not kernel behaviour: the R13A capture script, seeing the kernel's
`PRESENTER shadow` line, probed the shadow's mode word with an extra monitor command on **every** screen sample, so
the harness ran slower and the suites' host-time windows moved (R12C Stage 4 counts syncs in a window of "~7 s"
host time: the frozen R13 kernel gave 4 at the top of the 2..4 range; R13A with probing 5 twice; the same R13A kernel
with `QEMU_NO_SHADOW=1` — reading `0xB8000` only, R13's harness behaviour — gave 4). None of these suites renders a
console, so `tests/r13a/run-regressions.sh` now sets `QEMU_NO_SHADOW=1` and the two suites were re-run with it; the
others passed with probing and were not re-run. Logs: `logs/r13a/regressions/{probing,no-probing,timing-check}`.

The historical NEG/MULTI/TWICE suite, the R12 corpus and the frozen-binary R12B/R12A lanes were **NOT TESTED** for
R13A (not affected by the R13A changes; their R13 results stand: 12/14 NEG/MULTI/TWICE, identical on the frozen
R12C kernel).

## 9. Frozen images

`make r13-pc r13-pc-qdos5x9 r12c-pc r12b-pc r12a-pc` and shells a–d after all R13A changes: r13-vga `a7d86c21`,
r13 `c0546a47`, r13-vga-q5 `569c863f`, shell-d `e7056b03`, R12C `50ab9a6e`/`fc4a1368`, shell-c `93b87d14`, R12B
`d22fd9f1`/`01ebca11`, R12A `257460b9`/`0b23ab9d`, shell-b `dc50e084`, shell-a `9ed34a73` — all unchanged.

## 10. Package verification

`tests/r13a/verify-package.sh` on a fresh extraction of the candidate archive built from the commit before the final
one (`logs/r13a/package-verify.txt`; the final commit only adds this record, and the delivered archive was verified
again with the same script, with the same results): manifest OK
(every file against `SHA256SUMS`); QEMU smoke from the extracted kernel and volume — the rendered shell console and
a Minerva card (font loaded from the extracted volume) pixel-exact against the judge; Minerva scanner over the
extracted kernels (R13A: no Minerva table; the frozen R13 kernel as positive control: a 221-glyph run found); the
Minerva-free volume has only RECOVERY.FNT and CON8X16.FNT; host tests pass; rebuilt from the extracted source:
both R13A kernels, shell-e, GFXA, SURFT, COLOUR **byte-identical**, the frozen R13 (three) and R12C kernels and
shells d/c byte-identical, and every font file and generated table regenerated byte-identical.
