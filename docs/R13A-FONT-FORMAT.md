# R13A font format — QFNT version 1

A QFNT file holds one bitmap font: fixed-size glyphs, a 256-entry code map and a checksum. The kernel's loader
(`kernel/fontreg.c`, `kfont_parse`) and the host tool (`tools/r13a-font/qfnt.py`, `parse`) apply exactly the same
rules; `tests/r13a/run-host.sh` checks that both accept the shipped fonts and reject the same ten broken files.

**All multi-byte fields are little-endian.** Sizes are in bytes, pixels in pixels.

## Layout

| offset | size | field | rule |
|---|---|---|---|
| 0 | 4 | magic | `QFNT` |
| 4 | 2 | version | 1 |
| 6 | 2 | header length | 64 |
| 8 | 1 | glyph width `gw` | 1..16 |
| 9 | 1 | glyph height `gh` | 1..32 |
| 10 | 1 | cell width `cw` | `gw`..32 |
| 11 | 1 | cell height `ch` | `gh`..32 |
| 12 | 1 | glyph x offset in the cell `xo` | `xo + gw <= cw` |
| 13 | 1 | glyph y offset in the cell `yo` | `yo + gh <= ch` |
| 14 | 1 | bit order | 0 = the most significant bit is the leftmost pixel (the only order) |
| 15 | 1 | row bytes `rb` | `(gw + 7) / 8` (1 or 2) |
| 16 | 2 | glyph count `n` | 1..512 |
| 18 | 2 | default glyph | `< n`: drawn for every code mapped to `0xFFFF` |
| 20 | 2 | map entries | 256 |
| 22 | 2 | encoding | 1 ASCII, 2 QL (Sinclair QL / Minerva), 3 CP437, 4 KOI8-R (informative; no translation is done) |
| 24 | 4 | map offset | 64 |
| 28 | 4 | bitmap offset | 64 + 512 |
| 32 | 4 | payload length | file length − 64, and exactly `512 + n * gh * rb` |
| 36 | 4 | payload CRC-32 | CRC-32 (IEEE 802.3, reflected, init/xorout `0xFFFFFFFF`) of bytes 64..end |
| 40 | 4 | header CRC-32 | the same CRC of bytes 0..39 |
| 44 | 20 | name | ASCII, NUL-padded (shown by `FONT`; not used for anything else) |
| 64 | 512 | map | 256 × u16: the glyph index of each code (`0xFFFF`: the default glyph) |
| 576 | `n * gh * rb` | bitmaps | glyph-major, row-major; each row `rb` bytes, MSB = leftmost pixel |

## Validation rules (each a refusal, `-4`; nothing is allocated or changed)

1. At least 64 bytes; at most **33,344 bytes** (`64 + 512 + 512 × 32 × 2`: the largest valid font). The kernel checks
   the file size before allocating anything, so an oversized file never costs memory.
2. Magic, version, header length; the **header CRC**.
3. Glyph and cell sizes in range, the glyph inside its cell; bit order 0; row bytes as computed.
4. Glyph count 1..512, default glyph inside it, 256 map entries, a known encoding.
5. Offsets as stated; the payload length both equal to the file's remainder and to the computed size (so a
   truncated or padded file is refused, and no size arithmetic can overflow: every factor is bounded above).
6. The **payload CRC**.
7. Every map entry is `0xFFFF` or `< n`; **no glyph is mapped by two codes** (duplicate mapping).
8. **No bit set outside the glyph width** in any row (the padding bits of the last row byte are 0).

## Memory

A loaded font costs one kernel-heap block of `sizeof(struct kfont)` (≈ 600 bytes) + `n × gh × rb` (at most
32 KiB). The registry has 8 slots (2 built-in), so at most 6 loaded fonts at once: at most ≈ 200 KiB in the
pathological case; the shipped fonts are 1.4 to 2.6 KiB each. `FONT_OPEN` reads the file into a temporary block of
the file's size (≤ 33,344 bytes), registers a copy and frees the temporary block before returning.

## Rendering

A cell is `cw × ch` pixels; the glyph's row `r` (0..`gh`−1) is drawn at cell row `yo + r`, its leftmost pixel at
cell column `xo`. Attributes (INDEX8 text): bold ORs the row with itself shifted one pixel right (inside the cell),
underline fills the last cell row, inverse swaps ink and paper, no-paper leaves paper pixels unchanged.

## Shipped files

| file | glyphs | cell | encoding | source | SHA-256 |
|---|---|---|---|---|---|
| `fonts/RECOVERY.FNT` (built in, font 1) | 96 (0x20..0x7E + box) 6×9 | 6×10 | ASCII | X11 `koi6x9` (public domain) | `79c07a6a…` |
| `fonts/CON8X16.FNT` (built in, font 2) | 96 (0x20..0x7E + box) 8×16 | 8×16 | ASCII | X11 `koi8x16` (unrestricted) | `3b483c71…` |
| `third_party/minerva/MINERVA.FNT` (optional, GPL-2.0) | 225, 5×9 | 6×10 at x 1 | QL | Minerva 1.98 `tb_f0`/`tb_f1` | `1a11d1e1…` |

Converters: `qfnt.py pcf …` (X11 PCF), `qfnt.py minerva …` (the Minerva GAS source, optionally checked against the
ROM), `qfnt.py c …` (a kernel built-in table), `qfnt.py check FILE` (validate), `qfnt.py png FILE OUT` (specimen).
`tools/r13a-font/build-fonts.sh` regenerates all of them; the output is byte-identical every time.
