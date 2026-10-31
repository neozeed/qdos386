#!/bin/sh
# R11H scripted Tiny386 run (SCRIPTED keys via tests/interactive/run_pc_multi.py, not a person).
# The shell disk is build/pc-r11h/r11h-shell.img; the FAT16 volume is the second disk.
# Sequence lines: plain = type and wait for the prompt; "!" = type and do not wait (for a program
# that reads the console); the first character is the only prefix.
#   usage: TINY386=<tiny386_headless> sh scripts/r11h-run-tiny386.sh <outdir> <volume.img> <sequence> <tag>
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd); cd "$ROOT"
: "${TINY386:?set TINY386}"
OUT=${1:?usage}; VOL=${2:?usage}; SEQ=${3:?usage}; TAG=${4:?usage}
mkdir -p "$OUT"; D=build/pc-r11h-$TAG-run; rm -rf "$D"; mkdir -p "$D"
cp build/pc-r11f-none-run/bios.bin build/pc-r11f-none-run/vgabios.bin "$D/"
cp "${SHELLIMG:-build/pc-r11h/r11h-shell.img}" "$D/hda.img"
printf '[pc]\nbios = bios.bin\nvga_bios = vgabios.bin\nmem_size = 8M\nhda = hda.img\nhdb = win1-r11h.img\n\n[cpu]\nfpu = 1\n' > "$D/run.ini"
cp "$VOL" "$D/win1-r11h.img"
set --
while IFS= read -r line; do set -- "$@" "$line"; done < "$SEQ"
PC_CMD_TIMEOUT=${PC_CMD_TIMEOUT:-300} PC_INPUT_PAUSE=${PC_INPUT_PAUSE:-8} PC_KEY_DELAY=${PC_KEY_DELAY:-0} \
  python3 tests/interactive/run_pc_multi.py "$TINY386" "$D" "$D/win1-r11h.img" "$OUT/tiny386-$TAG.txt" "$@" \
  > "$OUT/tiny386-$TAG.stdout" 2>&1 || echo "tiny386 $TAG: harness rc!=0"
echo "done: $OUT/tiny386-$TAG.txt"
