#!/bin/sh
# R11G failure tests on Tiny386 (SCRIPTED).  Each damaged-story volume is booted once; the shell
# must survive the failed launch and answer DIR.  A save/restore session runs on the good volume.
#   usage: TINY386=<tiny386_headless> sh scripts/r11g-negative-tests.sh <outdir> <negative-volume-dir>
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd); cd "$ROOT"
: "${TINY386:?}"; OUT=${1:?}; NEG=${2:?}
mkdir -p "$OUT"
for name in missing trunc corrupt oversized empty badcode; do
  D=build/pc-r11g-neg-$name; rm -rf "$D"; mkdir -p "$D"
  cp build/pc-r11f-none-run/bios.bin build/pc-r11f-none-run/vgabios.bin "$D/"
  cp build/pc-r11f-none/r11f-none.img "$D/hda.img"
  printf '[pc]\nbios = bios.bin\nvga_bios = vgabios.bin\nmem_size = 8M\nhda = hda.img\nhdb = win1-r11g.img\n\n[cpu]\nfpu = 1\n' > "$D/run.ini"
  cp "$NEG/win1-r11g-$name.img" "$D/win1-r11g.img"
  PC_INPUT_PAUSE=8 python3 tests/interactive/run_pc_multi.py "$TINY386" "$D" "$D/win1-r11g.img" "$OUT/neg-$name.txt" \
    "!RUN INFOCOM.AOU" "!look" "!quit" "!y" "DIR" > "$OUT/neg-$name.stdout" 2>&1 || echo "harness rc!=0 for $name"
  echo "$name: done"
done
# save/restore on the good volume: both are out of scope and must fail cleanly (no crash, shell alive)
D=build/pc-r11g-saverestore; rm -rf "$D"; mkdir -p "$D"
cp build/pc-r11f-none-run/bios.bin build/pc-r11f-none-run/vgabios.bin "$D/"
cp build/pc-r11f-none/r11f-none.img "$D/hda.img"
printf '[pc]\nbios = bios.bin\nvga_bios = vgabios.bin\nmem_size = 8M\nhda = hda.img\nhdb = win1-r11g.img\n\n[cpu]\nfpu = 1\n' > "$D/run.ini"
cp build/fat16/win1-r11g.img "$D/win1-r11g.img"
PC_INPUT_PAUSE=8 python3 tests/interactive/run_pc_multi.py "$TINY386" "$D" "$D/win1-r11g.img" "$OUT/saverestore.txt" \
  "!RUN INFOCOM.AOU" "!look" "!save" "!SAVEGAME" "!restore" "!SAVEGAME" "!quit" "!y" "DIR" > "$OUT/saverestore.stdout" 2>&1 || echo "harness rc!=0 for saverestore"
echo "saverestore: done"
