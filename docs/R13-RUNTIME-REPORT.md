# R13 runtime report — virtual displays

Base: frozen R12C `7abb678b24764541845ef936adc45e9110f1a52b` (archive `96e7f6fc…`). Branch `r13-virtual-displays`.
The exact final commit and the archive SHA-256 are in `PACKAGE.md` and the delivery note. Kernels under test (all
final evidence): QEMU `build/r13/pc-vga/r13-vga.img`, headless `build/r13/pc/r13.img`; hashes of the kernels, the
shell and the R13 programs the final pass used are in `logs/r13/final/binaries.sha256` (checked unchanged after the
pass).

**Everything runs at CPL 0. Nothing in R13 is security isolation**: the surface API keeps cooperating programs off
the display hardware; it does not stop hostile ring-0 code.

## 1. Result

| Gate | Content | Result |
|---|---|---|
| Phase 0 | audit, memory budgets (QEMU 16 MiB / Tiny386 8 MiB), VGA mode-switch path, font provenance | done (`docs/R13-AUDIT.md`, `R13-MEMORY-BUDGET.md`, `R13-FONT-PROVENANCE.md`) |
| host | fonts (QDOS5x9 16/16, Minerva 11/11, both tables regenerate identically), INDEX8 library 26/26 with each font, SDK strict C89 | **PASS** |
| 1 | 13 private TEXT sessions + 4 consoles; 9 panels each shown by the selector and by `CON n` with all 4000 text-buffer bytes and the cursor exact; job table exhausted (14th START refused); key isolation; exit/kill cleanup; stale session numbers; SURFT API matrix; LEAK | **PASS (93/93)** |
| 2 | sole VGA presenter, text ↔ 320×200×256 on QEMU: 6 graphics screendumps pixel-identical to the recomputed cards; text after graphics identical to before; selector over graphics | **PASS (45/45)** |
| 3 | selector over shell, SuperBASIC (+ EXEC_W child), EDIT, COLOUR, BOUNCE, panel, legacy-console counter; focus follows selection, others continue | **PASS (46/46)** |
| 4 | COLOUR + BOUNCE + text together, 4 surfaces + refused 5th, separate framebuffers/palettes, background frames, forced (KILL, end-task, fault) and normal teardown | **PASS (49/49)** |
| 5 | three integrated QEMU cycles in one boot, final image FAT checks | **PASS (67/67)**, IMAGE PASS |
| 5 | one Tiny386 smoke (headless: presentation NA), SYNC, 0x8900 shutdown | **PASS (11/11)** |
| 5 | regressions: R12C Gates 1–4 + demo on the R13 kernel; frozen R12B and R12A suites on the R13 kernel; R13 programs on the R12C kernel | see §8 |
| 5 | frozen images byte-identical; package verification | see §9, §10 |

The per-stage runs that first passed each gate are in `logs/r13/stage1..4/`; the final evidence (all gates again on
the final build) is in `logs/r13/final/`.

## 2. Evidence classes

* **Guest, QEMU full machine** (QEMU 8.2 `-machine pc -m 16 -vga std`, PS/2 keyboard through the monitor, IDE).
  Captures: the 80×25 text buffer (characters; R13 adds `%cells`, characters **and attributes**, 4000 bytes),
  the CRTC cursor with the guest stopped (`%cursor`), and **real VGA screendumps** (`%screendump`, QEMU's rendering of
  the VGA state: 720×400 in text mode, 320×200 in mode 13h). Screendumps are kept as PNG in the logs.
* **Recomputed pictures** (`tests/r13/r13pics.py`): the judge redraws each test card / demo frame from the drawing
  calls the program makes, with the kernel's font table, and maps the palette through the 6-bit DAC exactly as QEMU's
  `vga.c` does (`c6_to_8`); a screendump passes only if **every pixel** matches. COLOUR and BOUNCE carry a binary
  frame counter on screen, so the judge reads the frame number from the picture and redraws that frame.
* **Programs' own verdicts**: COLOUR and BOUNCE keep a model of their surface and compare it at exit with the
  kernel's surface (read back, palette read back, checksum) — `verify=PASS`.
* **Guest, Tiny386** (headless, R12C emulator), **host** tests, and **independent** checks (mtools reads, `fsck.fat -n`,
  the VFAT walker, FAT-copy comparison).

## 3. Stage 1 — sessions and surfaces (`logs/r13/final/s1`)

`SURFT` (API matrix, 53 checks: capabilities, private session, NOGFX, sizes, BUSY, negative sizes, clipping of
fill/blit/read, kernel-memory and past-the-end pointers (−14), stride rules, palette ranges, text/print/cursor, a
fifth INDEX8 surface refused −5 with three children holding surfaces, children's surfaces freed at exit, stale ids
(−2) after close and after a session ends, cells, bad op): **PASS 53/53**.

Nine `SURFT panel K` jobs (`START … /NEW`) on sessions 4..12, COUNTER on 13, panels 10–12 on 14–16: `CON` lists 17
sessions (4 + 13). The 14th `START` is refused ("no free job slot": JOB_MAX 16 is the binding limit). Each panel draws a
fixed pattern with the console calls (characters and 16 attributes) and `qd_text_cells`, and parks the cursor at
(21, 5K). Pass A shows each through Ctrl+Esc; pass B through the shell's `CON n` in reverse order; also the selector
over panel 5 then Esc, and panel 6 a third time after the counter: **all 2000 cells + attributes identical to the
expected pattern and between passes; cursor position and shape identical**. Keys: each panel logged exactly the one
key typed while it was shown (`PANELk.LOG`, `keys=k`), the key typed on the counter reached nobody else. Panels 1–7
ended with `q` (display back to the shell), 8–9 and 10–12 killed; `CON 19`, `CON 25`, `CON 4` (ended) refused ("no
such console"). Memory: `LEAK` 754,416 → 501,168 with 12 private sessions + 12 jobs → **754,416** (heap, channels,
spaces, held frames identical).

## 4. Stage 2 — the presenter on QEMU (`logs/r13/final/s2`)

`platform/pc/vgapres.c` is the only code that programs the VGA for surfaces: mode 3 registers, DAC and the plane-2
font are read back at boot (no BIOS call in protected mode); mode 13h is set by the standard register table; the
switch runs in the SYSTEM job with interrupts on while traps are kept off the card (`PHYS_BUSY`); IRQs only set a
flag. Two `SURFT card` jobs (own palettes 0..47, colour bars, every font code 0x20..0xFF, the four attributes, a
text-stream line with the cursor) and a text panel:

* `c1a`–`c1d`, `c2a`, `c2b` (via selector, selector+Esc, graphics→graphics, Alt+F1/`CON n`): **all 6 screendumps are
  320×200 and pixel-identical to the recomputed cards** (palette through the DAC, Minerva glyphs, underline / inverse
  / bold / no-paper, the presenter's cursor); the two cards differ (separate framebuffers and palettes).
* The text panel shown before any graphics (`p3pre`) and twice after (`p3a`, `p3b`): **pixel-identical 720×400
  pictures** except the blinking cursor cell (masked), identical cells, cursor unchanged — font plane, DAC and text
  registers restored.
* Selector over a graphics session: text mode with the task list; Alt+F2 from graphics: text mode.
* Cards' checksums unchanged by the switching; each card got only its own key; presenter counters in the logs
  (`pres text=5 gfx=6 full=6 rows=1200` for card 1: only full redraws, nothing re-copied for a static card).

## 5. Stage 3 — the selector over many programs (`logs/r13/final/s3`)

Shell, SuperBASIC on session 4 (`PRINT "G3 BASIC ";6*7` → 42, then `EXEC_W "CHILDT", "a 0 57 read"` without a console
argument: the child shares BASIC's private session), EDIT on 5, COLOUR on 6, BOUNCE on 7, a panel on 8, COUNTER on
legacy console 1. Every job is listed by title with `G` for the graphics jobs; `TASKS /L` shows `GFX`. Focus follows
the selection: COLOUR and BOUNCE screendumps (3 each, live) are **pixel-identical to the programs' frames** and their
counters advance; keys typed there (`zz`, `yy`) reach neither the child nor EDIT; the child later reads exactly
`hello g3` and BASIC resumes; EDIT's cells are identical each time it is shown; the saved file is byte-exact. End-task
of the panel from the selector (confirm, list updates, Esc returns to EDIT exactly); a COLOUR of 600 frames that ends
while the selector is on its row ("has ended", Enter → "no longer exists: choose again"); COLOUR/BOUNCE quit with `q`
(verify PASS, frames drawn both shown and hidden), EDIT Ctrl+Q, BASIC and the counter killed; four legacy sessions
left; LEAK at the baseline.

## 6. Stage 4 — graphics programs (`logs/r13/final/s4`)

COLOUR (palette animation), BOUNCE (four blitted sprites) and a text panel, plus two `SURFT gfx` jobs: 4 INDEX8
surfaces open; `RUN COLOUR 10 1` refused cleanly (`no graphics surface (-5)`, status 2). Each graphics job shown twice
with the other hidden in between: **12/12 screendumps pixel-identical to the model of the frame shown** (so the
framebuffer and the palette restored are the job's own); COLOUR +620 and BOUNCE +923 frames while hidden (figures from
this run; the judge requires > 30). The panel's 2000 cells exact between them. Forced teardown: `KILL`, selector
end-task, and `SURFT gfxfault` (page fault: "process ended") — surface counters open 2, 3 more freed; a new surface can
be opened again; COLOUR ended with `q` (`verify=PASS`, frames shown and hidden), BOUNCE killed while running (no log),
`SURFT` matrix PASS afterwards; counters `open=0 opened=11 refused=2 freed=11`. Heap: 754,416 → 389,632 with 4
surfaces + 5 sessions + 5 jobs (364,784 bytes in use) → **754,416**. BOUNCE's error-rule checks (bad pointers,
negative and off-surface rectangles, stride, palette range): 0 failures.

## 7. Stage 5 — integrated cycles, Tiny386 (`logs/r13/final/integrated`, `…/tiny`)

**Three cycles in one QEMU boot**, each: BASIC with an EXEC_W child, EDIT saving `C5_n.TXT`, COLOUR and BOUNCE
animating, COUNTER on console 1; Ctrl+Esc, Alt+F2/Alt+F1, `CON n`; 3 + 3 live screendumps per cycle (**all 18
pixel-identical**); keys typed to the graphics jobs never reach the child; EDIT unchanged when shown again; teardown
varied (cycle 1 COLOUR `q` + BOUNCE `KILL`; cycle 2 BOUNCE `q` + COLOUR end-task; cycle 3 both `q`); after every cycle
only the four consoles, 3 jobs, LEAK **at the baseline**; the three files byte-exact on the final image: **67/67**;
final image: FAT copies identical, `fsck.fat -n` clean, VFAT walker 398 files / 17 directories, 0 errors.

**Tiny386** (R13 headless kernel, R12C emulator): capabilities `text 1 index8 1 show-text 0 show-gfx 0` — the headless
build has no display presenter, so **presentation on Tiny386 is NA**; the SURFT matrix PASS; a private text session
and a GFX session listed by `CON` and `TASKS /L`; the selector on the diagnostic stream; `COLOUR 20 1` drawn in memory
only, `verify=PASS`; SYNC; surface counters back to 0; LEAK at the baseline; 0x8900 shutdown, emulator exit 0; final
image clean: **11/11**.

## 8. Regressions

| suite | kernel | result |
|---|---|---|
| R12C Gate 1 (selector), 2 (titles), 3 (SYNC, QEMU + Tiny386 with the R13 headless kernel), 4 (periodic SYNC), demo | R13 | **49/49, 20/20, 44/44, 15/15, 20/20** |
| frozen R12B Gates 1–4 + demo (frozen R12B binaries, shell-b) | R13 | **50/50, 61/61, 39/39, 25/25, 30/30** |
| frozen R12A Gates 1–3 (frozen R12A binaries) | R13 | **20/20, 24/24, 35/35** |
| frozen R12A historical NEG/MULTI/TWICE vs the R12B baseline | R13 | **FAIL 13/14** (lane run), **12/14** (rerun alone) — see below |
| R12B Gates 3–4 with the current builds of BASIC/EDIT | R13 | **39/39, 25/25** |
| R13 shell and programs on the frozen R12C kernel (no `QT_SURFACE`): SURFT/COLOUR/BOUNCE refuse cleanly, `START /NEW` refused with a message, `/CON:2` works, `CON` lists the 4 consoles | R12C | **10/10** |

**NEG/MULTI/TWICE is not counted as a pass.** Its judge compares against the historical R12B baseline transcript, including the
exact job ids in three expected contention failures and the absence of any capture timeout. On the R13 kernel the
lane run failed one check (a capture timeout: background TWICE output arrived after the prompt of `TWICET N`, so the
harness waited for a prompt that was not the last line) and a rerun alone failed two (the same timeout, and one of
the three identical contention failures carrying job id 65544 instead of 131078 - a different job-table slot).
Control in the same session: **the frozen R12C kernel (byte-identical to the one that passed 14/14 in R12C) gives the
identical 12/14 with the same two checks when run alone**. Run beside another emulator, both kernels gave 12/14 with
the timeout and a different count of fault-report lines (R13 13, R12C 12; the baseline and both solo runs have 11;
not investigated further than these control runs). In every run the 848 NEG checks show exactly the 3 known contention
failures, and MULTI and the five TWICET cases end as in the baseline. Classified: FAIL (timing-sensitive expectations,
reproduced on the frozen R12C kernel; no R13 change involved). Logs: `logs/r13/final/regressions/negmulti-*`.

The R12 suites (Stages 1–4, the 103-program SuperBASIC corpus) are **NA** here, as in R12C: their kernel paths are
covered by the R12A/R12B suites above. The R12C runners `run-s2/s3/s4.sh` gained a kernel override
(`R12_QKERNEL`, `R12_QKERNEL_HEADLESS`); their default is unchanged.

## 9. Frozen images

Rebuilt from a clean copy of the final tree: R12C `r12c-vga.img` `50ab9a6e…`, `r12c.img` `fc4a1368…`, `shell-c`
`93b87d14…`; R12B `d22fd9f1…`/`01ebca11…`; R12A `257460b9…`/`0b23ab9d…`; R12 `1e00386b…`/`dd11fe87…`; `shell-b`
`dc50e084…`, `shell-a` `9ed34a73…`, `shell-m` `36720c5b…` — **all byte-identical** (all R13 code is under
`QDOS_SURFACES` / `R13_SHELL`; the console code is shared through a macro that compiles to the same bytes).

## 10. Package

`qdos386-R13-VIRTUAL-DISPLAYS.tar.gz` (`scripts/r13-package.sh`): source tree at the final commit, the R13 kernels
(Minerva font; `r13-vga-q5.img` with the QDOS5x9 fallback), `shell-d`, the unchanged older kernels and shells, guest
programs, the frozen binaries, `volumes/r13-system.img`, the Tiny386 emulator, firmware, `logs/r13/` and `SHA256SUMS`.
`tests/r13/verify-package.sh` checks a fresh extraction (`logs/r13/final/package-verify.txt`): manifest OK (3,140 files);
a QEMU smoke from the extracted kernel and example volume (R13 shell banner, `SURFACES: sessions, INDEX8, VGA text +
320x200x256, font Minerva 1.98`, SURFT caps with both SHOW bits, COLOUR shown live with its screendump pixel-identical
to its frame, `verify=PASS`, LEAK); the host tests from the extracted source; and a rebuild from the extracted source
that is **byte-identical** for the R13 kernels (`r13-vga.img` `a7d86c21…`, `r13.img` `c0546a47…`, `r13-vga-q5.img`
`569c863f…`), `shell-d` (`e7056b03…`), the R12C, R12B and R12A kernels and shells, and the guest programs SURFT,
COLOUR, BOUNCE, TITLET, BASIC, EDIT. The Minerva ROM is not in the package. The delivered archive is built again
from the final commit (this report included) and verified the same way; its SHA-256 is in the delivery note.

## 11. Fonts

Default: Minerva 1.98's QL fonts, extracted from the user's GAS source (`tb_f0`, `tb_f1`) and checked byte for byte
against `min198.rom`; GPL-2.0 (see `docs/R13-FONT-PROVENANCE.md` for the obligations). Fallback: the original QDOS5x9
(`make r13-pc-qdos5x9`), which is built and host-tested; the QEMU gates ran with the Minerva kernel.
