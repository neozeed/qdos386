# R13 font provenance

R13 has two bitmap fonts, chosen at build time (`R13_FONT` in the Makefile; `kernel/font.h`). Both are glyphs 5
pixels wide and 9 rows tall drawn in a 6×10 cell (53 × 20 cells on a 320×200 surface), one byte per row with
**bit 7 = the leftmost pixel**.

| build | font | codes | licence of the data |
|---|---|---|---|
| default (`make r13-pc`, `R13_FONT=minerva`) | **Minerva 1.98** QL fonts | all 256 (QL character set) | GNU GPL version 2 |
| `make r13-pc-qdos5x9` (`-DQDOS_FONT_QDOS5X9`) | QDOS5x9 (original fallback) | 0x20..0x7E, box for the rest | the QDOS/386 tree's |

The text sessions (TEXT80x25) are shown in VGA text mode with the card's own character generator, as before; the
bitmap font is used only for text drawn into INDEX8 surfaces (`qd_gfx_text`, `qd_gfx_print`).

## Minerva 1.98 (default)

### Inputs (attached by the user for R13)

| file | SHA-256 |
|---|---|
| `min198.zip` | `2f168734c6b4ba6c5ad30dcd08ffc345567ac10d9625b2bd7455e3584e4fb3b1` |
| &nbsp;&nbsp;`min198.rom` (48,822 bytes, dated 2003-04-09) | `aafbd3b0b2bf60d2cdeb139a355d552908b041ab021a1ff1b0be5547d9bcf2a2` |
| `MINERVA198-GAS-STANDALONE-MAKEFILE.zip` (Minerva 1.98 source translated to GNU as; rebuilds the ROM byte for byte, `make verify`) | `5a57823d25cc58c82275c4774c7b21eb0c653c3991c4209af2dfa8f596cd942a` |
| &nbsp;&nbsp;`src/tb_f0.s` (module 192, font 0) → `third_party/minerva/src/tb_f0.s` | `86ce4a921e7c6e4fd2746b6b77d2a600db1620058cb79c4c31200ea54ff38c69` |
| &nbsp;&nbsp;`src/tb_f1.s` (module 193, font 1) → `third_party/minerva/src/tb_f1.s` | `07b2e4d87562d517e347358fb28e36ef3f6e5cd4a8a303b2924524124e83e2dd` |
| &nbsp;&nbsp;`reference/min198.rom` and `build/minerva.rom` | identical to `min198.rom` above |

The `m198src.zip` original source archive was not attached; the translated GAS source keeps every original source
line as a `/* QL: ... */` comment next to the translated instruction, and the font rows are those original lines
(`dc.b O.O..` pixel art).

### Notices and licence

* Neither archive contains a licence file or a copyright notice text. The GAS edition's README says: "Existing
  Minerva source notices remain applicable. This build-system change does not assign a new license to the original
  material." The ROM shows its own copyright message at start-up (`sb_start.s`, `err.cp`).
* Minerva was written by Laurence Reeves (sold by TF Services); version 1.98 was open-sourced by him. The public
  source repository, github.com/MarcelKilgus/Minerva, declares **GPL-2.0** (its LICENSE is the GNU GPL version 2,
  June 1991) — checked for R13 through its GitHub page (kilgus.net/ql/minerva links it as "the sources").
* R13 therefore treats the Minerva font data as **GPL-2.0** material, copyright Laurence Reeves and contributors.
  `third_party/minerva/COPYING` is the GPL v2 text (from the Debian `common-licenses` copy in the build workspace).

### GPL obligations (documented, not legal advice)

* `kernel/font_minerva.c` is derived from GPL source, and the default R13 kernel images (`r13-vga.img`, `r13.img`)
  link it. Anyone **distributing** those images should do so under GPL-2.0-compatible terms and offer the
  corresponding source: the R13 archive carries the whole kernel source, `third_party/minerva/src/tb_f0.s` and
  `tb_f1.s` (the preferred form of the font data), the extractor, and `COPYING`.
* The QDOS/386 tree itself is not relicensed by this document; if that is not wanted, build with
  `make r13-pc-qdos5x9`, which contains no Minerva material (and keeps every feature; only the glyphs differ).
* The R13 deliverable does not include the Minerva ROM (`min198.rom` is only an input for the cross-check).

### Format, encoding and extraction

* QL font layout (Minerva `cs_char`, `tb_f0`/`tb_f1`): byte 0 = first code, byte 1 = number of glyphs − 1, then 9
  bytes per glyph, one per row, the 5 pixels in **bits 6..2** (bit 6 leftmost; the pattern equates in
  `constants.inc`, e.g. `M192_o....` = `0b1000000`). Not a flat IBM 8×8 table.
* `tb_f0`: codes **$1F..$7F** (97 glyphs; $1F is the "invalid character" pattern). `tb_f1`: codes **$80..$11F**
  (160 glyphs; codes above $FF are unreachable from a byte). ROM offsets 0xB5A8 and 0xB914.
* Mapping rule (Minerva's): a code in font 0's range uses font 0, else one in font 1's range uses font 1, else font
  0's first glyph. So **$00..$1E show the invalid-character glyph**.
* **Encoding: the QL character set, not ASCII/CP437.** $20..$7E equal ASCII except **$60 = £** (pound sign; there is
  no backquote) and **$7F = ©**; from $80 the specimen shows accented lower-case letters, then ß ¢ ¥ and a
  backquote-like accent (to $9F), accented capitals and Greek letters ($A0..), punctuation, arrows and other
  symbols up to $FF (the QL character set). Programs that draw text into a surface get these glyphs for those
  bytes; nothing is translated.
* `tools/r13-font/minerva-font.py ROM|- tb_f0.s tb_f1.s OUT.c [PNG]` reads the pixel rows from the source comments,
  locates the same header + rows in the ROM (exactly one match each — **source and ROM agree byte for byte**), and
  writes `kernel/font_minerva.c`: a 256 × 9 table, rows shifted left by one into the font.h layout (bit 7 leftmost,
  column 5 of the cell = the gap, as on the QL where the 6-pixel cell is bits 7..2 with bit 7 blank). Data SHA-256
  (2,304 bytes): `521b596a52443418ef99ec15dc33aa11895b964bd1c0af026e5c53d4f9c2689f`. With `-` instead of the ROM the
  table is rebuilt from the source alone; `tests/r13/run-host.sh` does that and checks the committed table is current.
* Specimen: `logs/r13/phase0/font-minerva-specimen.png` (16 × 16 codes). Checked by eye: space blank, `A`, `0`/`O`
  distinct, `.` `,` `:` `;` `!` `?` `"` `'` `(` `)` `[` `]` `_`, descenders on `g p q y j`, £ at $60, © at $7F.
* Host test `tests/r13/host-minerva.c` parses `tb_f0.s`/`tb_f1.s` itself (independently of the Python tool) and
  checks all 256 codes × 9 rows against `font_glyph`, the range rule, the bit layout, descenders and a rendered cell.

## The fallback: QDOS5x9 (original)

* `tools/r13-font/qdos5x9.txt`: 96 glyphs drawn for this project as text art (`#` ink, `.` paper), codes 0x20..0x7F.
  Not traced or converted from any ROM, BIOS, font file or the Minerva/JS ROM; licence: the QDOS/386 tree's.
* Same geometry: 5 × 9 glyphs in a 6 × 10 cell (rows 0–6 capitals and digits, rows 7–8 descenders).
* Encoding: index = code − 0x20; code 0x7F is a box used for **every code outside 0x20..0x7E** (ASCII only).
* Built by `tools/r13-font/mkfont.py` into `kernel/font5x9.c` (generated); data SHA-256 (864 bytes)
  `58a4e5aff44481e5e88dd0126f8734b29ac665109ffc69b199bced4fdedb4a63`; specimen `logs/r13/phase0/font-specimen.png`;
  host test `tests/r13/host-font.c` (compiled with `-DQDOS_FONT_QDOS5X9`).
* Built and kept working by `make r13-pc-qdos5x9` (`build/r13/pc-vga-q5/r13-vga-q5.img`).
