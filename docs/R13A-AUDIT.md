# R13A audit (Stage 0)

Base: frozen R13 `c3683d58e02817570f66919aead2451dd13e51a6`, archive `qdos386-R13-VIRTUAL-DISPLAYS.tar.gz`
(`bac427c6c33b37123ec20e53b157a89a529d3655a3331f1fe606b19e25f50d32`, re-checked before branching). Branch
`r13a-graphics-usability`. R13's images are not touched: every R13A change is under `-DQDOS_R13A` / `-DR13A_SHELL`
or in new files, and `make r13-pc` still produces `r13-vga.img` `a7d86c21…` and `r13.img` `c0546a47…`.

## 1. What R13 had (read from the source)

* `kernel/surface.c` (QT_SURFACE ops 0..21), `kernel/gsurf.c` (INDEX8 ≤ 320×200, text stream), `kernel/vcon.c`
  (sessions 0..19), `platform/pc/vgapres.c` (the presenter: mode 3 read back at boot, mode 13h by registers),
  `platform/pc/i8042.c` + `kernel/kbd.c` (set-1 scancodes → bytes; Alt+Fn / Ctrl+Esc consumed), `kernel/font.c` with
  one compiled-in font table.
* Font bytes in the R13 kernels: the default build embeds **Minerva 1.98's two QL fonts** as `kernel/font_minerva.c`
  (2,304 bytes, GPL-2.0); `r13-vga-q5.img` embeds **QDOS5x9** instead (`kernel/font5x9.c`, from
  `tools/r13-font/qdos5x9.txt`). The new scanner (`tools/r13a-font/scan-minerva.py`) confirms the first (a run of 221
  consecutive Minerva glyphs) and shows that QDOS5x9 shares 45 of its glyph patterns, and a run of 5, with Minerva —
  see `docs/R13A-FONT-PROVENANCE.md`; R13A uses neither.
* No mouse: the i8042 aux port is disabled and its bytes discarded.

## 2. Memory (measured with LEAK, R13 and R13A kernels, idle shell)

| machine | free kernel heap | INDEX8 at 320×200 | 512×256 |
|---|---|---|---|
| QEMU `-m 16` | 754,416 (R13), 726,304 (R13A: larger static data, the shadow, key queues) | 64,000 pixel bytes + header | 131,072 pixel bytes + header |
| Tiny386 8 MiB | 762,608 (R13), 741,744 (R13A headless kernel) | same | same |

The kernel heap is the same size on both machines (the frame pool scales with RAM, the heap does not), so one
budget fits both: R13A caps all INDEX8 blocks together at **400 KiB** (three 512×256, or six 320×200, still at most
four surfaces) and fonts at 6 loaded × ≤ 33 KiB (≈ 200 KiB worst case; the shipped ones are 1.4–2.6 KiB; Minerva
costs 2,656 bytes of heap, measured in the integrated run). Worst case 400 KiB + 200 KiB still leaves ≈ 120 KiB of the
726 KiB idle heap. The GFXA matrix opens three 512×256 surfaces and has the fourth refused by the budget on both
machines, then checks that every pixel byte and font is returned; the kernel heap at that peak was not printed.

## 3. VGA modes actually available

* **QEMU** (`-vga std`: the Bochs/QEMU VGA): all standard VGA modes by register programming — mode 3 (used), mode 13h
  (used), **mode 12h 640×480×16 planar** (verified in R13A: QEMU's screendump is 640×480 and pixel-exact against the
  model). 256 colours at 640×400 or 640×480 need the VBE (Bochs DISPI) interface, which the handoff excludes: not used.
* **Tiny386**: its VGA model renders planar 16-colour and 256-colour modes (`vga.c`, `shift_control` 0/2), but the
  Tiny386 kernel is the headless build (no presenter, R13 rule), so presentation there is **NA**; sessions, fonts,
  surfaces and key-event state work in memory.
* The protected-mode switch path is R13's: register tables written by the presenter in the SYSTEM job with
  interrupts on; no BIOS call.

## 4. Short R13 smoke on the R13A kernel

Booted the R13A kernel with the R13A volume: R13 programs (SURFT card, COLOUR) run unchanged beside the R13A ones (Gate 3,
Gate 4, the integrated cycles); the R13 gate suites
are re-run on the R13A kernel in Stage 5 (`docs/R13A-RUNTIME-REPORT.md`). The R13 shell (`shell-d`) is unchanged; the
R13A shell is `shell-e` (`-DR13A_SHELL`: FONT, CONSOLE, SET FONT=).

## 5. Decisions

* Fonts: QFNT v1 (`docs/R13A-FONT-FORMAT.md`), loaded by the kernel from a path through its FAT/VFAT layer or from a
  buffer; built-in recovery and console fonts from public-domain / unrestricted X11 fonts; Minerva only as an
  optional file.
* Rendered consoles: presenter-side rendering from the cells (no per-session pixel buffer: 80×25 cells are the
  backing store; the pixels are a pure function of cells + font), physical mode 12h.
* Logical surfaces up to 512×256 with fit (nearest-neighbour downscale, aspect kept, centred) or crop presentation.
* Key events per session with synthetic releases on focus loss; mouse not implemented.
