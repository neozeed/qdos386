# R13A graphics ABI — fonts, rendered consoles, logical surfaces, key events, blits

R13A kernels (`-DQDOS_R13A`: `build/r13a/pc-vga/r13a-vga.img`, `build/r13a/pc/r13a.img`) keep every R13 call of
`QT_SURFACE` (trap `0x3B`, `docs/R13-DISPLAY-ABI.md`) and add ops 22..36. `SURF_VERSION` stays 1; `CAPS` gains
`0x10` (R13A calls) and `0x20` (rendered text consoles can be shown). libqdos wrappers: `sdk/lib/qdosgfx.c`
(strict C89, a separate archive member), declared in `sdk/include/qdos386.h` (R13A section).

**Cooperative ring-0 code, not security isolation** (as R13): the API never gives a program video memory or port
access, but nothing stops hostile CPL-0 code from programming the VGA itself.

## 1. Display model

Three things are kept apart:

| term | meaning |
|---|---|
| **logical text** | a session's 80 × 25 cells (character + attribute) and cursor — the console API's model, authoritative in every mode |
| **surface pixels** | an INDEX8 surface's private buffer, 1..512 × 1..256, its palette; or, for a rendered console, the pixels the cells + font define (`80·cw × 25·ch`, not stored) |
| **physical mode** | what the VGA is in: mode 3 (80 × 25 text), mode 13h (320 × 200 × 256) or mode 12h (640 × 480 × 16) |

The presenter (`platform/pc/vgapres13a.c`, the only code that programs the VGA) chooses the physical mode for the
foreground session: an INDEX8 surface → mode 13h; a session in rendered-console mode → mode 12h; otherwise mode 3.
The task selector always appears in mode 3 (it needs no font or disk).

## 2. Fonts

QFNT v1 files (`docs/R13A-FONT-FORMAT.md`). Built-in, never freed: **1** recovery (6×9 in 6×10, ASCII + box) — the
default for INDEX8 text — and **2** console (8×16, ASCII + box) — the default for rendered consoles. Up to 6 more
fonts can be registered at once.

| op | name | b | c | result |
|---|---|---|---|---|
| 36 | FONT_OPEN | path (NUL-terminated, < 128 bytes; relative to the caller's directory, as `RUN` resolves programs) | – | font id; FAT/channel errors (missing file …), −4 invalid/oversized/empty, −3 no heap, −5 table full |
| 22 | FONT_LOAD | QFNT image | bytes (1..33,344) | font id (the kernel copies the image) |
| 23 | FONT_RELEASE | id | – | 0; −8 not the loader's |
| 24 | FONT_SELECT | id (0: none → the system default) | – | 0: the caller's **session** font (its INDEX8 text grid changes with the cell; a rendered console uses it if 80×25 cells fit in 640×480) |
| 25 | FONT_INFO | id (0: the caller's surface-text font) | 16-word buffer | version, id, glyph w, h, cell w, h, glyphs, encoding, refs, flags (1 built-in, 2 loaded by the caller, 4 default for surface text, 8 default for consoles), CRC, name (20 bytes) |
| 26 | FONT_DEFAULT | 0 surface text / 1 consoles | id | 0: the system default from now on (new surfaces; consoles without their own font) |
| 27 | FONT_LIST | buffer | max ids | count |

**Ownership.** A registered font is immutable. References are held by: the job that loaded it (until
`FONT_RELEASE` or the job's end — exit, kill or fault), each session that selected it, each INDEX8 surface using
it, and each system default naming it. When the last reference goes the id becomes stale at once (−2) and the block
is freed by the SYSTEM job (never while the presenter draws). So freeing a surface, ending a job or loading another
font never invalidates another surface's font. Loading a broken file changes nothing: the previous font stays.

Shell: `FONT` (list), `FONT path [/CON|/SURF]` (load and make default for both / consoles / surface text),
`FONT n [/CON|/SURF]`, `SET FONT=path` (the same as `FONT path`: put it in `/ETC/STARTUP.CMD` to choose the font at
every boot).

## 3. Rendered text consoles

| op | name | b | c | result |
|---|---|---|---|---|
| 28 | CONSOLE | `0` VGA text mode, `1` rendered (`0xFFFFFFFF`: query) | `0` the caller's session, `1` every session and the default for new ones | 0; −7 if the display cannot show rendered text (headless) |
| 29 | TEXT_SUM | surface id (0: the caller's) | – | 31-bit FNV-1a of the session's text as rendered: `25·ch` rows × `80·cw` pixels, one byte per pixel = its colour 0..15 (foreground = cell bits 8..11, background = bits 12..14), cursor not included |

A rendered console is drawn from the session's cells by the presenter in mode 12h, 640 × 480 × 16 colours (planar,
standard VGA registers — no VBE, no BIOS call), with the session's font, or the console default, or the recovery
font (the first whose 80 × 25 cells fit). 8×16 cells fill 640 × 400 (rows 40..439); 6×10 cells (Minerva) fill 480 ×
250, centred. Colours: the 16 text colours (EGA palette); the blink bit (attribute bit 7) is ignored. The cursor is
the last two pixel rows of its cell in the foreground colour, drawn on the screen only (it never changes the cells
or the checksum). Applications are unchanged: `printf`, the console calls, EDIT and BASIC write cells as before; the
presenter compares each row with what it drew and redraws changed rows (and the cursor's) from its poll, with
interrupts on; nothing is drawn from an interrupt. `CONSOLE VGA` (or a headless display) is the recovery path: the
VGA's own text mode and font, the same cells.

Characters a font does not map (e.g. CP437 line-drawing codes in the ASCII built-in fonts) show the font's default
glyph (a box).

For test harnesses, the presenter keeps a copy of what it shows (`struct pres_shadow`, `kernel/present.h`; its
address is printed at boot: `PRESENTER shadow 0x…`): mode, session, cursor, font, cell size, origin and the 2000
cells last rendered.

## 4. Logical surfaces and presentation

`GFX_OPEN` accepts 1..**512** × 1..**256** (`0, 0` is still 320 × 200). All INDEX8 blocks together are limited to
a **pixel budget of 400 KiB** of kernel heap (3 surfaces of 512 × 256, or 6 of 320 × 200 — still at most 4 surfaces):
an open beyond it is refused −5 and changes nothing.

| op | name | b | c | result |
|---|---|---|---|---|
| 30 | VIEW | `0` FIT, `1` CROP | `vx \| vy << 16` (CROP: the window's top-left, inside the surface) | 0; −4 for a bad mode or a corner outside |

Presentation in mode 13h (the private buffer is never changed by it; `READ` and `SUM` see the exact pixels):

* a surface that fits the screen (≤ 320 × 200), view FIT: 1:1 at the top-left (the R13 rule);
* a larger surface, view FIT: scaled **down** to fit, aspect ratio kept, nearest neighbour (`src = dst · size /
  shown size`), centred with black bars — 512 × 256 is shown as 320 × 160 at rows 20..179;
* view CROP: a 320 × 200 window at (vx, vy), 1:1, black beyond the surface.

The text-stream cursor is drawn only in the 1:1 views. Dirty rows are mapped to the screen rows that show them.
Modes 640 × 400 / 640 × 480 at 256 colours do not exist on a standard VGA without VBE; R13A does not use VBE.

## 5. Native key events

| op | name | b | c | result |
|---|---|---|---|---|
| 31 | KEV_ENABLE | 1 on / 0 off | – | 0 (the caller's session; the queue is emptied) |
| 32 | KEV_READ | buffer | max (≤ 64) | events copied (never blocks) |

Event word: scancode (set 1 make code, bits 0..7), `0x100` extended (E0), `0x200` down, `0x400` synthetic, `0x1000`
Shift, `0x2000` Ctrl, `0x4000` Alt (at the time), bits 16..23 the translated character of a key-down where there is
one (the byte the console gets), else 0. Rules:

* Events go only to the **foreground session**, only while the task selector is closed, only if it enabled them.
  The session's console input (cooked/raw bytes) is unchanged and independent.
* Hotkeys (Alt+F1..F4, Alt+1..4, Ctrl+Esc) are consumed once: neither the Fn/digit/Esc key's down nor its up reaches
  a program. (The Alt or Ctrl key itself went down in the old session; it gets a synthetic up there.)
* When the focus moves (selector, Alt+Fn, `CON n`, a session ending), the session losing it receives a **synthetic
  key-up** (`0x400`) for every key it saw go down and not up; a later physical key-up whose key-down went to another
  session (or to a hotkey) is dropped. So a held key can never stick in a background program and never leaks.
* 64 events per session; when full, new events are dropped and counted (`MEMSTATS`).

## 6. Blits

| op | name | b | c | result |
|---|---|---|---|---|
| 33 | BLIT_KEY | `struct surf_rect {x, y, w, h, src, stride}` | transparent colour | pixels written (source pixels equal to the colour leave the surface's pixel) |
| 34 | COPY | `struct surf_copy {sx, sy, w, h, dx, dy}` | – | pixels copied, within the caller's surface, **as memmove** (overlap in any direction is safe) |
| 35 | MEMSTATS | 8-word buffer | – | pixel bytes in use, budget, budget refusals, loaded fonts, font bytes, key events delivered, dropped, synthetic |

Clipping and user-pointer rules are R13's: rectangles are clipped to the surface (negative width/height −4); BLIT_KEY
reads only the visible part of the source, checked against the caller's memory (−14); COPY copies only the part whose
source and destination are both inside the surface. The opaque `BLIT` and `FILL` are unchanged.

## 7. Mouse

**NOT IMPLEMENTED.** The kernel's i8042 driver keeps the auxiliary (PS/2 mouse) port disabled and discards aux bytes
(`platform/pc/i8042.c`); QEMU provides a PS/2 mouse, but no mouse path was built or verified in R13A. The rule
reserved for it is R13's: mouse events follow keyboard focus (§5's rules), and a pointer would be drawn by the
presenter.
