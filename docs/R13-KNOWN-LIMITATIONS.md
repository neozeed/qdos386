# R13 known limitations

* **No security isolation.** Everything runs at CPL 0. The surface API keeps cooperating programs off the VGA (no
  video-memory pointer is ever handed out, no supported call programs the card), but a hostile or buggy ring-0
  program can still write the VGA ports, `0xA0000`/`0xB8000` or kernel memory. Background drawing is confined to a
  program's own surface only for programs that use the API.
* **One display mode for graphics:** INDEX8 surfaces are presented in 320×200×256 (mode 13h) only, at the top left
  (smaller surfaces on black). No VBE, no other resolutions, no planar 16-colour modes, no GPU acceleration, no
  overlapping windows or compositor, no vsync: a screendump (or a person) can catch the presenter between rows of a
  frame. The judges accept one torn picture out of three per demo and report the count (all runs so far: 0 torn).
* **Presentation latency** is the SYSTEM job's poll: the presenter copies dirty rows when the SYSTEM job runs (it
  shares the CPU with the other jobs; there is no presenter priority). Full redraws (64,000 bytes) happen on every
  switch to a graphics session.
* **Text sessions are shown with the VGA's own text font** (the BIOS font in plane 2, saved at boot), not with the
  Minerva font; the Minerva (or QDOS5x9) bitmap font is used for text drawn into INDEX8 surfaces. A QL-style
  bitmap rendering of TEXT80x25 sessions inside a graphics mode is not implemented (text-mode fallback is the only
  text path).
* **Character set of surface text:** the QL set of Minerva in the default build (0x60 is £, 0x7F ©, 0x80.. QL
  accented letters); C strings are not translated, so a backquote draws a pound sign. The QDOS5x9 build is ASCII
  0x20..0x7E with a box for the rest. VGA text sessions keep CP437.
* **Limits:** 4 INDEX8 surfaces at once, 20 sessions (4 legacy + 16 private), surfaces ≤ 320×200. The job table
  (JOB_MAX 16: SYSTEM, IDLE, the shell + 13) is the binding limit: at most 13 private sessions can exist, so the
  "sessions full" refusal (−5) is a guard that no test can reach; the INDEX8 limit and the job-table limit are tested.
* **SuperBASIC `EXEC`/`EXEC_W name, args$, con`** (frozen R12B/R12C BASIC) accepts only the legacy consoles 0..3 for
  `con`; without the argument the child shares BASIC's own (possibly private) session, which is what Gate 3 uses.
  BASIC has no graphics keywords (no surface access from BASIC in R13).
* **Mouse: NOT IMPLEMENTED.** The intended rule (mouse follows keyboard focus, consumed by the selector, never
  delivered to a background session) is documented in `docs/R13-DISPLAY-ABI.md` only.
* **The task selector** keeps the R12C layout (mode shown as the `G` flag and the detail line `GFX WxH`; `TASKS /L`
  has a MODE column). It lists up to 16 jobs on one screen (the whole job table).
* **Tiny386:** the R13 headless kernel has no display presenter, so presenting sessions/surfaces on Tiny386 is
  **NA** (sessions and INDEX8 surfaces work in memory; their capabilities say "not shown"). The Tiny386 smoke covers
  that, SYNC and the clean 0x8900 shutdown only.
* **Fonts / licensing:** the default kernels contain Minerva 1.98's font data (GPL-2.0, see
  `docs/R13-FONT-PROVENANCE.md`); distributing them has GPL consequences that this document does not resolve
  legally. `r13-vga-q5.img` (QDOS5x9) contains no Minerva material. The original `m198src.zip` was not attached;
  the font was taken from the user's GAS translation of the 1.98 source and checked byte for byte against the ROM.
* **Shell `CON n`** takes 0..19; `START … /NEW` needs an R13 kernel (on R12C it says "this kernel has no private
  sessions" and starts nothing).
* **Not tested:** real VGA hardware (QEMU's standard VGA only); keyboards other than QEMU's PS/2 emulation; more than
  three integrated cycles in one boot; a fault inside a trap while the presenter switches (the presenter keeps the
  card BUSY-flagged and traps never touch it, by construction).
* **Regression not passing:** the historical R12A NEG/MULTI/TWICE comparison is 12–13/14 on the R13 kernel; the frozen
  R12C kernel gives the same 12/14 in this session (timing-sensitive expectations: a capture timeout and one job id).
  See `docs/R13-RUNTIME-REPORT.md` §8.
* **Out of scope (unchanged):** DOOM, a DOS box/V86, QLBox/Musashi, ring 3, a compositor.
