# Apollo 14 (R18) — the planar VGA presenter (mode 12h for INDEX8 surfaces)

Source: `platform/pc/vgapres13a.c`, inside `#ifdef QDOS_R18`. It is the only code that writes the VGA, and it runs in
the SYSTEM job on CPU 0.

## Registers

The mode is set by the existing R13A `mode12` table (standard IBM values), written by `write_regs()`:

| Group | Values |
| --- | --- |
| Miscellaneous output | 0xE3 (25 MHz clock, 480 lines, colour addressing) |
| Sequencer | 03 01 0F 00 06 (map mask 0x0F, memory mode: extended, no odd/even, no chain-4) |
| CRTC | 640×480 timing, offset 0x28 (80 bytes per row), line compare 0x3FF |
| Graphics controller | 00 00 00 00 00 00 05 0F FF (set/reset off, write mode 0, read mode 0, memory map 0xA0000 64 KiB, bit mask 0xFF) |
| Attribute controller | palette 0..15 = identity, mode control 0x01 (graphics), colour plane enable 0x0F, colour select 0 |

With an identity attribute palette and colour select 0, a 4-bit pixel value *p* selects DAC register *p*.

## Writing pixels

Each screen row is composed in `line[640]` (one byte per pixel, values 0..15). `put_line(y)` then writes it plane by
plane. The meaning is:

```
for plane p in 0..3:
    sequencer map mask (SR02) = 1 << p          -- only plane p is written
    for byte b in 0..79:                         -- 8 pixels per byte, bit 7 = leftmost
        VRAM[y*80 + b] = sum over k in 0..7 of ((line[8b+k] >> p) & 1) << (7-k)
map mask back to 0x0F
```

R18 computes this in one pass over the pixels. `spread[v]` has bit *p* of *v* at bit 8*p*, and eight
`w = (w << 1) | spread[pixel]` steps leave the four plane bytes of a byte group in the four bytes of `w`. Each plane's
80 bytes are then written as 20 dwords under its map mask. The R13A builds keep the original four-pass loop.

Write mode 0 with set/reset disabled and bit mask 0xFF writes the CPU byte unchanged. The latches are never used, so
they need no save or restore. Nothing reads VRAM.

## Presentation of a surface (`to_g12gfx`, `g12_row`, `g12_update`)

* **Layout** (`g12_layout`):
  * xs = 2 for `GS_VIEW_VGA12W` when 2w ≤ 640, else 1;
  * x0 = (640 − w·xs) / 2;
  * y0 = (480 − h) / 2.

  Rows are never scaled: a surface up to 512×256 always fits.
* **Colour**: `line[x0 + xs·x + k] = pix[y][x] & 15`.
* **DAC registers 0..15**: palette entries 0..15, each RGB channel >> 2 (6 bits), written at the first presentation and
  whenever the surface's `pal_gen` changes.
* **Border**: rows outside [y0, y0 + h) and columns outside the picture are colour 0.
* **Full redraw**: on entry to the state (`PHYS_G12G`), and on any change of session, session tag, surface, view or
  focus recheck. The registers and DAC are set at once; the rows follow in bounded steps.
* **Updates**: only the dirty rows [dy0, dy1) of the surface, taken and cleared under `irq_save`. A text-stream cursor
  (if a program turns it on) is drawn into `line` only, never into the surface.
* **Bounded work per call**: at most `G12_ROWS` (64) rows per `pres_poll` call. After a full redraw the border rows go
  first, then the surface rows. Rows not done are merged back into the surface's dirty range and finished on the next
  ticks. A full 640×480 redraw therefore takes 8 ticks (80 ms), and a 256-row EliteQL frame 4 ticks (the screen follows a
  program that redraws everything at 25 Hz). The cap is a compile-time choice: `make qdos386-r18-pc
  R18_EXTRA=-DG12_ROWS=128` halves the update time and doubles the longest interrupts-off section.
* **Leaving**: `to_text`, `to_gfx` (mode 13h) and `to_gtext` (rendered text console) each write their own register
  table and DAC. Mode 12h → rendered text console skips the register write (same table) but reloads the 16 EGA text
  colours.

### Register state between paths

| State | Map mask | GC | DAC |
| --- | --- | --- | --- |
| Text (mode 3) | from the BIOS state read at boot | from the BIOS state | the BIOS DAC (saved at boot) |
| Mode 13h | 0x0F | mode13 table | the surface palette (256 entries) |
| Mode 12h text console | 0x0F after each row | mode12 table | 16 EGA text colours |
| Mode 12h surface (R18) | 0x0F after each row | mode12 table | surface entries 0..15 |

## Cost under the big kernel lock

`pres_poll` runs with the BKL held and interrupts off (`kernel/shellboot.c`, the R16 rule). R18 wraps it with TSC
reads and exposes the result with `SURF_OP_PRES_TIME`: calls, drawing calls, the maximum and total cycles, and the
maximum rows in one call. `PLANAR TIME` converts cycles to µs with a 1-second TSC calibration. Measured values are in
`APOLLO14-RUNTIME-REPORT.md`.

## Host oracle (`tests/a14/oracle.py`)

The oracle takes a dump of what the kernel's surface holds: size, effective view, palette and pixels. It gets this from
PLANAR (written by the program with `qd_gfx_read`) or from `%gsnap`, which reads the presenter's surface from guest
memory with the guest stopped. From the dump it rebuilds the screen in four steps:

1. The 640×480 index image (layout above, & 15, border 0).
2. Each row split into the four bitplanes exactly as `put_line` packs them, then recombined. This models the VGA
   memory, so a packing error would show as an inconsistent model.
3. DAC: entry RGB >> 2, then QEMU's 6-to-8-bit expansion `(v << 2) | (v & 1) * 3`.
4. A compare of all 307,200 pixels with the QEMU screendump (PPM).

A run passes only when every pixel matches.
