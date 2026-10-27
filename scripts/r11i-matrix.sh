#!/bin/sh
# R11I verification matrix (SCRIPTED keys on every target; no human session).
#   i386VM legacy (RAM disk, one-argument harness) : MULTI x3, NEG x3
#   i386VM block  (host-file disk, two-argument)    : MULTI x3 (release volume), NEG x3 (negative volume)
#   Tiny386 PC (unchanged path)                    : MULTI x3, NEG x3, plus the R11H regression sequences
# Volume hashes are taken before and after the matrix and must not change.
#   usage: TINY386=<tiny386_headless> I386VM_HARNESS=<i386vm-r9> I386VM_BLOCK_HARNESS=<i386vm-r9-block> \
#          sh scripts/r11i-matrix.sh <outdir>
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd); cd "$ROOT"
: "${TINY386:?}" "${I386VM_HARNESS:?}" "${I386VM_BLOCK_HARNESS:?}"
OUT=${1:?usage: r11i-matrix.sh OUTDIR}; mkdir -p "$OUT"
REL=build/fat16/win1-r11h.img; NEG=build/fat16/win1-r11h-neg.img
LEG=build/r11i/legacy/qdos386-r11i-legacy.bin; LEGNEG=build/i386vm-r11h-neg/qdos386-r11h-neg.bin
BLK=build/r11i/block/qdos386-r11i-block.bin
hash_all() { for v in $REL $NEG; do echo "$v $(sha256sum "$v" | cut -c1-64)"; done; }
hash_all > "$OUT/volumes-before.txt"
for r in 1 2 3; do
  sh scripts/r11h-run-i386vm.sh "$OUT" "$LEG" tests/r11h/seq-multi.txt legacy-multi-r$r
  sh scripts/r11h-run-i386vm.sh "$OUT" "$LEGNEG" tests/r11h/seq-neg.txt legacy-neg-r$r
  sh scripts/r11i-run-i386vm-block.sh "$OUT" "$BLK" "$REL" tests/r11h/seq-multi.txt block-multi-r$r
  sh scripts/r11i-run-i386vm-block.sh "$OUT" "$BLK" "$NEG" tests/r11h/seq-neg.txt block-neg-r$r
  sh scripts/r11h-run-tiny386.sh "$OUT" "$REL" tests/r11h/seq-multi.txt tiny-multi-r$r
  sh scripts/r11h-run-tiny386.sh "$OUT" "$NEG" tests/r11h/seq-neg.txt tiny-neg-r$r
done
hash_all > "$OUT/volumes-after.txt"
if cmp -s "$OUT/volumes-before.txt" "$OUT/volumes-after.txt"; then echo "VOLUMES UNCHANGED"; else echo "VOLUME CHANGED"; fi
echo "MATRIX DONE"
