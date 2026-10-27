#!/bin/sh
# R11H verification matrix (SCRIPTED keys, both emulators).  Runs, in order:
#   MULTI x3 (release volume), NEG x3 (negative volume), PHOON battery x1 (release volume),
#   Zork x3 (story volume, private test only; skipped and reported when the volume is absent).  Volume hashes are taken before and after; the
#   matrix fails if the release volume changes.
#   usage: TINY386=<tiny386_headless> I386VM_HARNESS=<i386vm-r9> sh scripts/r11h-matrix.sh <outdir>
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd); cd "$ROOT"
: "${TINY386:?}" "${I386VM_HARNESS:?}"
OUT=${1:?usage: r11h-matrix.sh OUTDIR}; mkdir -p "$OUT"
REL=build/fat16/win1-r11h.img; NEG=build/fat16/win1-r11h-neg.img; ZRK=build/fat16/win1-r11h-zork.img
IREL=build/i386vm-r11h-rel/qdos386-r11h-rel.bin; INEG=build/i386vm-r11h-neg/qdos386-r11h-neg.bin
IZRK=build/i386vm-r11h-zork/qdos386-r11h-zork.bin
hash_all() { for v in $REL $NEG $ZRK; do [ -f "$v" ] && echo "$v $(sha256sum "$v" | cut -c1-64)"; done; return 0; }
hash_all > "$OUT/volumes-before.txt"
for r in 1 2 3; do
  sh scripts/r11h-run-tiny386.sh "$OUT" "$REL" tests/r11h/seq-multi.txt multi-r$r
  sh scripts/r11h-run-i386vm.sh "$OUT" "$IREL" tests/r11h/seq-multi.txt i386-multi-r$r
done
for r in 1 2 3; do
  sh scripts/r11h-run-tiny386.sh "$OUT" "$NEG" tests/r11h/seq-neg.txt neg-r$r
  sh scripts/r11h-run-i386vm.sh "$OUT" "$INEG" tests/r11h/seq-neg.txt i386-neg-r$r
done
sh scripts/r11h-run-tiny386.sh "$OUT" "$REL" tests/r11h/seq-phoon.txt phoon
sh scripts/r11h-run-i386vm.sh "$OUT" "$IREL" tests/r11h/seq-phoon.txt i386-phoon
if [ -f "$ZRK" ] && [ -f "$IZRK" ]; then
  for r in 1 2 3; do
    sh scripts/r11h-run-tiny386.sh "$OUT" "$ZRK" tests/r11h/seq-zork.txt zork-r$r
    sh scripts/r11h-run-i386vm.sh "$OUT" "$IZRK" tests/r11h/seq-zork.txt i386-zork-r$r
  done
else
  echo "ZORK: UNAVAILABLE (private story volume not present; not run)" | tee "$OUT/zork-unavailable.txt"
fi
hash_all > "$OUT/volumes-after.txt"
if cmp -s "$OUT/volumes-before.txt" "$OUT/volumes-after.txt"; then echo "VOLUMES UNCHANGED"; else echo "VOLUME CHANGED"; fi
echo "MATRIX DONE"
