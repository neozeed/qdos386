# R13 display surfaces — ABI

Kernels built with `-DQDOS_SURFACES` (the R13 images `r13-vga.img`, `r13.img`). The calls are reached through
`QT_SURFACE` (trap function `0x3B`) and wrapped in strict C89 by libqdos (`sdk/include/qdos386.h`,
`sdk/lib/qdossurf.c`). Every existing R12C console, job, title, SYNC, environment, EDIT and BASIC call is unchanged;
a program that never calls `QT_SURFACE` behaves exactly as on R12C.

**This is cooperative ring-0 code, not security isolation.** Programs run in ring 0. The supported API never gives a
program a pointer to video memory and never programs the VGA for it, and the supported path keeps every program
off the hardware, but nothing stops hostile ring-0 code from writing the VGA ports or `0xA0000` directly.

## 1. Model

* A **session** is one virtual display plus one keyboard queue. Sessions 0..3 are the R12C consoles (Alt+F1..F4,
  static). Sessions 4..19 are **private sessions**, allocated on demand from the kernel heap.
* Every session has a **TEXT80x25** surface: 80 × 25 cells of character + attribute, a cursor, a scroll region,
  an input queue and a line discipline (the R11M/R12 console). A session may also hold one **INDEX8** surface
  (1..320 × 1..200 pixels, 256-entry RGB palette, a text stream drawn with the bitmap font); while it does, the
  session shows the INDEX8 surface instead of its text. The text cells are kept underneath and come back when the
  INDEX8 surface is closed.
* A surface belongs to its **session**, and a session to the **jobs on it**: the job that created a private
  session is recorded as its owner; the session lives until the last job on it ends. The Alt+Fn consoles are a
  compatibility view of sessions 0..3 only. More than four programs are reached through the task selector.
* Exactly one session is the **foreground**: only it is presented on the physical display and only it receives
  keys. Background sessions keep running and keep drawing — into their own surfaces only.
* Focus is independent of scheduling: the foreground session's jobs get no extra CPU, and background jobs are not
  stopped.

### Creating sessions

| how | result |
|---|---|
| shell `START prog … /NEW` (or `qd_spawn(..., con = QD_SPAWN_CON_NEW)` = −2) | the child starts on a new private TEXT session |
| `qd_surface_new()` | the caller moves to a new private TEXT session (refused `-6` if it owns the INDEX8 surface of its session) |
| `qd_gfx_open(w, h, flags)` from a shared session (a legacy console, or a private one with other jobs) | the caller first moves to a new private session, unless `QD_GFX_SHARE` |
| a child started without `/NEW` | shares its creator's session (the R12C rule); background, non-interactive jobs cost nothing |

Children inherit the creator's session; an `EXEC_W` child shares its parent's session and the session's keys go to
whichever of them reads. A job moved to a new session takes its CON channels with it (they are retargeted). An empty
private session is freed at once; if it was in front, session 0 is shown.

## 2. The call

`QT_SURFACE` with `a = op`, `b`, `c` (32-bit). All results are `int`; negative = error. Ids are
`(tag << 8) | session`, the tag changing every time a session is created, freed, or gains/loses its INDEX8 surface,
so an id from an earlier one is **stale** (`-2`). Id `0` means the caller's own session.

| op | name | b | c | result |
|---|---|---|---|---|
| 0 | CAPS | – | – | `(1 << 16) \| caps`: 1 TEXT, 2 INDEX8, 4 the display presents text, 8 the display presents INDEX8 (320×200×256) |
| 1 | INFO | id | 16-word buffer | 0; words: version, id, kind (1 TEXT, 2 INDEX8), w, h, stride, cols, rows, cursor (`col \| row<<8 \| visible<<16`), flags (1 shown, 2 private, 4 selector over it, 8 caller owns the INDEX8), users, owner job id, palette generation, pixel generation, frames, session index |
| 2 | NEW | – | – | id of the caller's new private TEXT session |
| 3 | GFX_OPEN | `w \| h<<16` (0 = 320×200) | flags: 1 FOREGROUND (show it now), 2 SHARE (stay on a shared session) | id |
| 4 | GFX_CLOSE | – | – | 0; the session shows its text again (owner only, else `-8`) |
| 5 | FILL | `struct surf_rect {x, y, w, h, colour, –}` | – | pixels changed |
| 6 | BLIT | `{x, y, w, h, src, src_stride}` | – | pixels copied |
| 7 | READ | `{x, y, w, h, dst, dst_stride}` | – | pixels read |
| 8 | PAL_SET | `first \| count<<16` (count 1..256, first+count ≤ 256) | `3·count` bytes R,G,B | count |
| 9 | PAL_GET | as PAL_SET | buffer | count |
| 10 | TEXT | `struct surf_text {x, y, ink, paper, attr, str, len ≤ 256}` | – | cells drawn |
| 11 | PRINT | string | length ≤ 256 | length: the text stream (CR, LF scrolls the whole surface one cell row at the bottom, BS, TAB to a multiple of 8, other control codes ignored, wraps) |
| 12 | CURSOR | `col \| row<<8 \| visible<<16`; `0xFFFFFFFF` = query | – | 0 / the position |
| 13 | SCROLL | `{x, y, w, h, dy, fill}` (dy > 0: up) | – | pixels changed |
| 14 | FOREGROUND | id (own session only) | – | 0; `-8` for another job's session |
| 15 | FOCUS | – | – | 1 if the caller's session is shown and gets the keys (and the selector is not over it), else 0 |
| 16 | SUM | – | – | 31-bit FNV-1a of the INDEX8 pixels (row by row, `w` bytes, no padding) then the 768 palette bytes |
| 17 | LIST | buffer | bytes | count; 16-byte header (version, entry size 32, count, VSESS_MAX) + per session: id, index, kind, w, h, users, owner, flags |
| 18 | CELLS | `struct surf_cells {row, col, n ≤ 2000, cells}` | – | cells written: TEXT cells `char \| attr<<8` straight into the caller's session (row-major from row, col; a run past the last cell is refused `-4`) |
| 19 | COLOURS | `ink \| paper<<8 \| attr<<16` | – | 0: the text stream's colours |
| 20 | FRAME | – | – | the surface's frame count + 1 (bookkeeping for animating programs) |
| 21 | STATS | 8-word buffer | – | 0: INDEX8 surfaces open, opened, refused, freed; presenter: switches to text, mode-13h sets, full redraws, rows copied |

Ops 5..13, 16, 19, 20 act on the INDEX8 surface of the **caller's session** (`-7` if it shows text). Any job on the
session may draw; only the job that opened it may close it.

### Errors

| code | name | meaning |
|---|---|---|
| −2 | STALE | no such surface (ended, or an id from an earlier one) |
| −3 | MEM | no kernel heap for the block |
| −4 | ARG | bad op, flags, size, negative width/height, palette range, stride (smaller than the visible width or above 65,536), text longer than 256 |
| −5 | FULL | 4 INDEX8 surfaces in use, or all 20 sessions in use |
| −6 | BUSY | the session already has an INDEX8 surface / NEW while owning one |
| −7 | NOGFX | an INDEX8 call on a session that shows text |
| −8 | OWNER | not the caller's surface or session |
| −14 | RANGE | an argument record or a buffer (the part a clipped rectangle reads or writes) outside the caller's memory |

An error changes nothing.

### Drawing rules (`kernel/gsurf.h`)

* Coordinates are signed 32-bit pixels from the top-left; every rectangle is **clipped** to the surface. Width or
  height < 0 is `-4`; a rectangle wholly outside, or of zero size, draws nothing and returns 0. Coordinate sums are
  computed on a clamped range, so huge values cannot overflow.
* BLIT/READ read/write only the visible part of the user rectangle: row `sy..` of the buffer at pitch `stride`,
  starting `sx` bytes in. The kernel checks exactly that span against the caller's memory.
* **Colour units:** pixels are palette indices 0..255; palette entries are R, G, B bytes **0..255**. The VGA shows
  the top 6 bits (the presenter writes `v >> 2` to the DAC). The default palette of a new surface is the 16 EGA
  colours, a 6×6×6 cube (16..231) and 24 greys.
* Text: the bitmap font's 6 × 10 cells (5 × 9 glyphs; 53 × 20 on 320 × 200). Attributes: 1 underline (row 9),
  2 inverse, 4 bold (ink also one pixel right), 8 no paper (ink only). **Character set: the QL set of Minerva 1.98**
  in the default build (ASCII for 0x20..0x7E except **0x60 = £**; 0x7F ©; 0x80.. accented letters, Greek, symbols;
  0x00..0x1E the invalid-character glyph); ASCII with a box for the rest in the `QDOS5x9` build. See
  `docs/R13-FONT-PROVENANCE.md`.
* The text-stream **cursor** is non-destructive: the presenter draws it (an underline in rows 8..9 of its cell, in
  the ink colour) on the display only; the surface's pixels, READ and SUM never contain it.

### Lifetime

A surface lives until GFX_CLOSE or until the job that opened it ends — exit, `KILL`, Task Manager end-task or a
fault all take the same path (`job_gone` → `surf_job_ended`). The block is unlinked at once and freed by the SYSTEM
job (never while the presenter copies it). A private session is freed when its last job ends. LEAK/PINFO return to
the baseline (Gates 1, 2, 4).

## 3. The presenter (one hardware-facing path)

`platform/pc/vgapres.c` is the only code that programs the VGA for surfaces. It runs in the SYSTEM job
(`pres_poll`, interrupts on); no interrupt handler and no trap touches the card while it switches (`PHYS_BUSY`).

* At boot it reads back the BIOS's mode 3 registers (sequencer, CRTC, graphics controller, attribute controller,
  misc output), the 256-entry DAC and the 8 KiB text font in plane 2 — no BIOS call is made in protected mode.
* Foreground TEXT session: mode 3 with the saved registers, font and DAC; the cells are copied to `0xB8000` and the
  hardware cursor set (the R12C console code draws directly while text mode is up).
* Foreground INDEX8 session: the standard mode 13h register set (320×200×256, chain-4), the surface's palette to the
  DAC, a **full redraw** on every switch; afterwards only the **dirty rows** (and a palette change by generation
  number, and the cursor) are copied each poll. Smaller surfaces show at the top left on black.
* The task selector (Ctrl+Esc) is drawn in text mode: over an INDEX8 session the presenter switches to text for it
  and redraws the surface in full when it closes.
* Text-mode fallback: if the boot read-back did not happen, or on the headless build, CAPS has no SHOW bits and
  INDEX8 surfaces exist in memory only (programs still run and draw).

## 4. Keyboard, focus and the selector

* Keys go to the foreground session's input queue only. Alt+F1..F4 (the legacy consoles) and Ctrl+Esc (the task
  selector) are consumed by the kernel and never reach a program.
* The selector (`TASKS`, Ctrl+Esc; R12C SYSTASK) lists every job with title, program, state, session (`CON`) and
  mode (`TEXT`/`GFX` in the detail line and `TASKS /L`); Enter shows the selected job's session, Esc restores the
  previous view exactly, F1 ends a job (protected jobs refused). It runs inside the SYSTEM job (no job slot).
* **Mouse: NOT IMPLEMENTED.** Reserved rule for a future mouse: mouse events follow keyboard focus — they go only
  to the foreground session, are consumed by the selector while it is shown, and are never delivered to a
  background session; a pointer, if drawn, is drawn by the presenter like the text cursor.

## 5. libqdos wrappers (strict C89)

`qd_surface_caps`, `qd_surface_info`, `qd_surface_new`, `qd_gfx_open`, `qd_gfx_close`, `qd_gfx_fill`, `qd_gfx_blit`,
`qd_gfx_read`, `qd_gfx_scroll`, `qd_gfx_palette`, `qd_gfx_get_palette`, `qd_gfx_text`, `qd_gfx_print`,
`qd_gfx_colours`, `qd_gfx_cursor`, `qd_gfx_cursor_get`, `qd_surface_foreground`, `qd_surface_focus`,
`qd_gfx_checksum`, `qd_gfx_frame`, `qd_surface_list`, `qd_text_cells`, `qd_surface_stats`. On a kernel without
surfaces every call returns −1 (test `qd_surface_caps()` first). Examples: `SURFT.AOU` (the API matrix and the
gate modes), `COLOUR.AOU` (palette animation), `BOUNCE.AOU` (moving sprites).

## 6. Shell

`START prog args /NEW`, `CON` (lists all sessions: legacy and private, TEXT/GFX, jobs), `CON n` (show session n,
0..19), `KILL /CON:n`, `TASKS [/L]`. `CON n` of a session that does not exist: "no such console".
