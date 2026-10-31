#!/bin/sh
# R11G scripted Tiny386 run (SCRIPTED keys via tests/interactive/run_pc_multi.py, not a human).
# One boot: RUN INFOCOM.AOU, the acceptance sequence (tests/r11g/seq-zork-game.txt), QUIT, a shell
# check (DIR), a relaunch of the interpreter, a short game, QUIT, and a second shell check.
#   usage: TINY386=<tiny386_headless> sh scripts/r11g-run-tiny386.sh <outdir> [volume.img]
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd); cd "$ROOT"
: "${TINY386:?set TINY386}"
OUT=${1:?usage: r11g-run-tiny386.sh OUTDIR}; VOL=${2:-build/fat16/win1-r11g.img}
mkdir -p "$OUT"; D=build/pc-r11g-run; rm -rf "$D"; mkdir -p "$D"
cp build/pc-r11f-none-run/bios.bin build/pc-r11f-none-run/vgabios.bin "$D/"
cp build/pc-r11f-none/r11f-none.img "$D/hda.img"
printf '[pc]\nbios = bios.bin\nvga_bios = vgabios.bin\nmem_size = 8M\nhda = hda.img\nhdb = win1-r11g.img\n\n[cpu]\nfpu = 1\n' > "$D/run.ini"
set -- "!RUN INFOCOM.AOU"
while IFS= read -r line; do set -- "$@" "!$line"; done < tests/r11g/seq-zork-game.txt
set -- "$@" "DIR" "!RUN INFOCOM.AOU" "!look" "!quit" "!y" "DIR"
cp "$VOL" "$D/win1-r11g.img"
PC_INPUT_PAUSE=${PC_INPUT_PAUSE:-8} python3 tests/interactive/run_pc_multi.py "$TINY386" "$D" "$D/win1-r11g.img" "$OUT/tiny386-game.txt" "$@"
echo "done: $OUT/tiny386-game.txt"
