# R13A known limitations

* **No security isolation.** Everything still runs at CPL 0. The R13A calls keep cooperating programs off the VGA
  (no video-memory pointer is handed out, fonts are copied into kernel memory, surfaces are private buffers), but a
  hostile or buggy ring-0 program can still write the VGA ports, `0xA0000`/`0xB8000` or kernel memory. Nothing here
  claims that malicious VGA access is prevented.
* **Physical modes:** mode 3 (VGA text), mode 13h (320×200×256) and mode 12h (640×480×16, rendered consoles) only,
  all programmed by registers. No VBE/Bochs DISPI, so no 256 colours above 320×200, no 640×400×256, no
  GPU acceleration, no compositor, no overlapping windows.
* **No vsync.** The presenter copies rows when the SYSTEM job runs; a screendump can catch a frame half copied. The
  judges allow one torn picture per group and print the count (Gate 3's official run and the demo report theirs in
  `docs/R13A-RUNTIME-REPORT.md`).
* **Rendered consoles:** 16 colours (the CGA attribute palette), the blink bit is ignored (bit 7 of the attribute is
  dropped: backgrounds are colours 0–7, nothing blinks), the cursor is drawn as the last two pixel rows of its
  cell in the foreground colour and does not blink. Codes a font does not have are drawn as its box glyph: with
  the built-in 8x16 console font (ASCII 0x20–0x7E) that includes CP437 line-drawing and accented characters, so
  programs that draw CP437 boxes look plainer than in VGA text mode. Text sessions keep 80×25 cells; a font whose
  80×25 cells do not fit 640×480 (wider than 8 or taller than 19 pixels) is not used for a console: the next font
  in the chain (console default, then recovery) is. The Task Manager always uses VGA text
  mode (mode 3) and switches back.
* **Logical surfaces:** at most 512×256 and four INDEX8 surfaces, all INDEX8 pixels together ≤ 400 KiB. Fit
  presentation scales down only (nearest neighbour, aspect kept, centred in 320×200: 512×256 shows as 320×160);
  no upscaling of small surfaces (they stay 1:1 at the top left as in R13); crop is a 1:1 window. No smoothing.
* **Fonts:** QFNT v1 only (bitmap glyphs ≤ 16×32 in cells ≤ 32×32, ≤ 512 glyphs but a 256-entry byte-code
  map, no Unicode, no proportional spacing). At most 6 loaded fonts besides the two built-ins. A font is read whole
  into the kernel heap (≤ 33,344 bytes). The built-in fonts cover ASCII 0x20–0x7E plus a box; QL characters (£, ©, accents) are available only
  with the optional Minerva file.
* **Minerva:** `MINERVA.FNT` contains Minerva 1.98's font data (GPL-2.0). It is a separate, optional file with its
  notice; the R13A kernels do not contain it (scanner result in `docs/R13A-FONT-PROVENANCE.md`). Distribution of
  the file has GPL consequences this document does not resolve legally. The frozen R13 kernels still embed Minerva
  (R13 default build) or QDOS5x9 (which shares 45 glyph patterns with Minerva); R13A does not change them.
* **Key events:** set-1 scancodes only (QEMU's PS/2 keyboard); E0 keys carry the `0x100` extended flag; there is no
  separate repeat event (typematic repeats arrive as further "down" events, as the keyboard sends them); 64 queued
  events per session, new events dropped when the queue is full (counted in MEMSTATS). The shell and the R12 programs
  keep reading bytes; events are opt-in per session.
* **Mouse: NOT IMPLEMENTED.** The i8042 aux port stays disabled; the intended routing rule is in
  `docs/R13A-GRAPHICS-ABI.md` only.
* **Tiny386:** the headless kernel has no presenter, so rendered consoles (CONSOLE GFX is refused there) and surface
  presentation are **NA** on Tiny386; fonts, surfaces, key-event state and the GFXA matrix run in memory.
* **SuperBASIC** has no graphics or font keywords (R13A adds none); it runs unchanged in rendered consoles.
* **Test harness timing:** with an R13A kernel the QEMU capture script probes the presenter's shadow on every
  screen sample (needed to read rendered consoles); that slows it enough to move host-time windows in older suites
  (R12C Stage 4's periodic-sync count). Suites without rendered consoles run with `QEMU_NO_SHADOW=1`.
* **Not tested:** real VGA hardware and monitors, non-QEMU keyboards, fonts other than the three shipped ones and the
  ten broken fixtures, more than three integrated cycles in one boot.
