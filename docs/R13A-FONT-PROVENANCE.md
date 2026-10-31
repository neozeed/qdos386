# R13A font provenance and licensing

R13A separates the fonts from the kernel. **The R13A kernels contain no Minerva bitmap data**; the two built-in fonts
come from X11 bitmap fonts whose notices allow unrestricted use; Minerva 1.98's fonts are an optional, separately
identifiable GPL-2.0 file that is loaded from disk at run time. (Not legal advice: the obligations below are stated
as R13A understands them, not as a legal opinion.)

## Built-in fonts (in every R13A kernel)

| kernel font | file | source font | package | notice (verbatim, `fonts/src/xfonts-cyrillic.copyright`) |
|---|---|---|---|---|
| 1 recovery, 6×9 in 6×10 | `fonts/RECOVERY.FNT` → `kernel/font_recovery.c` | `koi6x9.pcf.gz` | Ubuntu `xfonts-cyrillic` 1:1.0.5+nmu1 (X.Org font-misc-cyrillic) | "Public domain font.  Share and enjoy." |
| 2 console, 8×16 | `fonts/CON8X16.FNT` → `kernel/font_console.c` | `koi8x16.pcf.gz` | same | "May be distributed and modified without restrictions." |

* Source files (in the tree, `fonts/src/`): `koi6x9.pcf.gz` SHA-256 `0a4b2fcba61c08127e94f5918afc512aad4bea0a3b040a1060b32cf948b9486b`
  (decompressed `61e0a3ea…`), `koi8x16.pcf.gz` `f242a2c7c68e266a6eeb86acef38c98e38d82617550f14169e8258fb24551066`
  (decompressed `0de7ab84…`); the package's full copyright file is kept next to them.
* Only codes 0x20..0x7E are taken (the ASCII half of KOI8-R, identical to the X11 "fixed" fonts there), placed on the
  font's own baseline. Codes the font does not have show a box glyph drawn by the converter (`qfnt.py`), not taken
  from any font.
* Conversion: `tools/r13a-font/qfnt.py pcf` (a PCF reader written for R13A: metrics, bitmaps, encodings and
  accelerator tables). `tools/r13a-font/build-fonts.sh` regenerates `RECOVERY.FNT` (`79c07a6a…`), `CON8X16.FNT`
  (`3b483c71…`) and the two kernel tables byte-identically (checked by `tests/r13a/run-host.sh`).

## Minerva 1.98 (optional, GPL-2.0)

* `third_party/minerva/MINERVA.FNT` (`1a11d1e16d1c8a67ce72ad941ccb5d20197413b88332c5f0af599b2630d15939`, 2,601
  bytes) is made by `qfnt.py minerva` from `third_party/minerva/src/tb_f0.s` and `tb_f1.s` — the user-supplied GAS
  translation of Minerva 1.98's source (the same inputs and hashes as R13: `docs/R13-FONT-PROVENANCE.md`); with
  `MINERVA_ROM=min198.rom` the converter checks every row byte for byte against the ROM (done for this release).
* Contents: tb_f0's 97 glyphs (codes $1F..$7F; $1F is Minerva's "invalid character" pattern, the default glyph) and
  tb_f1's glyphs for $80..$FF (tb_f1's further 32 glyphs, codes above $FF, are unreachable from a byte and left out).
  Pixels: the source's 5 columns (bits 6..2 of the ROM byte) as 5-bit rows; cell 6×10 with the glyph at x 1, exactly
  the QL's 6-pixel cell (the ROM byte's bit 7 is the blank left column). **QL mapping kept: $60 is £, $7F ©, $80..
  the QL accented letters**; $00..$1E show the invalid-character glyph, as Minerva's `cs_char`.
* Difference from R13: R13's built-in Minerva table put the gap column on the right (glyph at x 0); R13A uses the
  QL's own placement (x 1). The glyph bitmaps are identical (checked: every one of the 256 codes' 9 rows of the R13
  table equals the R13A file's rows shifted by three bits — `tests/r13a/cmp-r13-minerva.py`, run by `tests/r13a/run-host.sh`).
* Licence: GPL-2.0 (the public Minerva source repository declares it; see R13's provenance document). Shipped as:
  the `.FNT` file, `fs/r13a-system/SYSTEM/FONTS/MINERVA.TXT` (a notice installed next to it on the example volume),
  the corresponding source (`third_party/minerva/src`), the converter, and `third_party/minerva/COPYING` (GPL v2).
  Whoever distributes `MINERVA.FNT` (or a volume containing it) should do so under the GPL's terms for that file.
  The run-time file boundary keeps the Minerva bytes out of the kernel binary; R13A does **not** claim that this
  boundary settles every GPL question for a system that loads the file. A volume without it:
  `R13A_NO_MINERVA=1 sh scripts/r13a-sysvol.sh OUT.img`.
* The Minerva ROM itself is not distributed.

## Checking a binary for Minerva bytes

`tools/r13a-font/scan-minerva.py TB_F0.s TB_F1.s [--allow FONT.FNT ...] BINARY...` reads the Minerva glyphs from the
source modules and reports, for each binary, (a) the longest run of consecutive Minerva glyphs found contiguously in
any of the three layouts (QL ROM bytes, the R13 kernel table, QFNT rows) — a copy of the font or of part of it shows
as a run ≥ 2 — and (b) single-glyph coincidences (glyphs with at least three set rows), classified as inside an
allowed font image or elsewhere. Results for this release (`logs/r13a/final/gate1/scan.txt`):

| binary | longest run | coincidences | verdict |
|---|---|---|---|
| `r13a-vga.img`, `r13a.img` | 0 | 6, all inside the public-domain built-in font images (`+`, `:`, `T`, `^`, `{`, `}`) | no Minerva table |
| R13 `r13-vga.img` (frozen) | 221 | 280 | Minerva built in (as documented for R13) |
| R12C `r12c-vga.img` | 0 | 0 | — |

The six coincidences are simple shapes that the public-domain X11 6×9 / 8×16 designs share pixel for pixel with
Minerva; they are located inside the X11-derived font images and come from the X11 source files.

## A finding about R13's QDOS5x9 fallback

The same scanner shows that R13's "original" QDOS5x9 fallback font (`tools/r13-font/qdos5x9.txt`, used by the frozen
`r13-vga-q5.img`) has **45 of its 94 visible glyphs pixel-identical to Minerva's**, including a run of 5 consecutive
codes. It was drawn for R13 without consulting Minerva, but such an overlap cannot be shown to be independent
creation, so **R13A does not use QDOS5x9 at all** (it is not built into any R13A kernel); the recovery font is the
public-domain X11 font above instead. R13's frozen files are left unchanged; this note corrects R13's provenance
claim for QDOS5x9 ("original glyphs").
