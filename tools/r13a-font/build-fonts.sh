#!/bin/sh
# R13A: (re)generate every font file and kernel table from its source, deterministically.
#   fonts/RECOVERY.FNT  <- fonts/src/koi6x9.pcf.gz  (X11 misc-cyrillic, "Public domain font.  Share and enjoy.")
#   fonts/CON8X16.FNT   <- fonts/src/koi8x16.pcf.gz (X11 misc-cyrillic, "May be distributed and modified without
#                          restrictions.")   - both: codes 0x20..0x7E (the ASCII half of KOI8-R) + a box glyph
#   kernel/font_recovery.c, kernel/font_console.c  <- the two files above (the kernel's built-in fonts)
#   third_party/minerva/MINERVA.FNT <- third_party/minerva/src/tb_f0.s, tb_f1.s (Minerva 1.98, GPL-2.0: OPTIONAL,
#                          never linked into a kernel); with MINERVA_ROM=<min198.rom> the rows are checked against the ROM
#   usage: sh tools/r13a-font/build-fonts.sh [OUTDIR]   (OUTDIR: write there instead of the tree, for comparison)
set -eu
ROOT=$(cd "$(dirname "$0")/../.." && pwd); cd "$ROOT"
O=${1:-.}
mkdir -p "$O/fonts" "$O/kernel" "$O/third_party/minerva"
Q="python3 tools/r13a-font/qfnt.py"
$Q pcf fonts/src/koi6x9.pcf.gz "$O/fonts/RECOVERY.FNT" "Recovery 6x9 (PD)" ascii 0x20 0x7e 6 10
$Q pcf fonts/src/koi8x16.pcf.gz "$O/fonts/CON8X16.FNT" "Console 8x16" ascii 0x20 0x7e 8 16
$Q c "$O/fonts/RECOVERY.FNT" font_recovery "$O/kernel/font_recovery.c"
$Q c "$O/fonts/CON8X16.FNT" font_console "$O/kernel/font_console.c"
$Q minerva third_party/minerva/src/tb_f0.s third_party/minerva/src/tb_f1.s "$O/third_party/minerva/MINERVA.FNT" ${MINERVA_ROM:-}
sed -i 's#from \./#from #; s#from '"$O"'/#from #' "$O/kernel/font_recovery.c" "$O/kernel/font_console.c"
