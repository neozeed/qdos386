#!/bin/sh
# R11G runtime matrix: the Zork I acceptance sequence on Tiny386 and i386VM, ROUNDS times each
# (default 3).  All keys are SCRIPTED (tests/r11g/seq-zork-game.txt).  The FAT16 volume hash is
# recorded before and after the runs (the runs must not write to it).
#   usage: TINY386=<tiny386_headless> I386VM_HARNESS=<i386vm-r9> ZORK_STORY=<zork1_z3> \
#          QDOS386_TOOLS=<historical tree> sh scripts/r11g-run-all.sh <outdir>
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd); cd "$ROOT"
OUT=${1:?usage: r11g-run-all.sh OUTDIR}; ROUNDS=${ROUNDS:-3}
VOL=build/fat16/win1-r11g.img
mkdir -p "$OUT"
H0=$(sha256sum "$VOL" | cut -c1-64); echo "FAT16 volume sha256 before: $H0" | tee "$OUT/matrix.txt"
r=1
while [ $r -le "$ROUNDS" ]; do
  sh scripts/r11g-run-tiny386.sh "$OUT/tiny386-round$r" "$VOL" > "$OUT/tiny386-round$r.log" 2>&1 || echo "tiny386 round $r: harness rc!=0" >> "$OUT/matrix.txt"
  sh scripts/r11g-run-i386vm.sh "$OUT/i386vm-round$r" > "$OUT/i386vm-round$r.log" 2>&1 || echo "i386vm round $r: harness rc!=0" >> "$OUT/matrix.txt"
  echo "round $r complete" >> "$OUT/matrix.txt"
  r=$((r + 1))
done
H1=$(sha256sum "$VOL" | cut -c1-64); echo "FAT16 volume sha256 after:  $H1" >> "$OUT/matrix.txt"
[ "$H0" = "$H1" ] && echo "VOLUME UNCHANGED" >> "$OUT/matrix.txt" || echo "VOLUME CHANGED" >> "$OUT/matrix.txt"
echo "MATRIX DONE" >> "$OUT/matrix.txt"
