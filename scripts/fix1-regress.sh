#!/bin/sh
# R11M-FIX1 bounded regression of the R11K / R11H / R11F sequences on the R11M kernels.  Same sequences and judge as
# scripts/r11m-regress.sh, but WITHOUT the CPU-only i386VM groups (retired from gating, see docs/R11M-FIX1-TEST-POLICY.md):
#   QEMU VGA (primary)       pg-r1 (paging, R11K sequence), defk-r1 (non-paging default kernel)
#   Tiny386 full-machine     pg (seq-pc), pgneg (NEG), pgregression (R11F), defk-multi, defk-neg, defk-nopaging  - one round, patched emulator
# Volumes are the frozen read-only R11K/R11H volumes (copied by the runners); hashes are compared before/after.
#   usage: TINY386=<patched tiny386_headless> sh scripts/fix1-regress.sh <outdir>
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd); cd "$ROOT"
: "${TINY386:?}"
OUT=${1:?usage}; mkdir -p "$OUT"
V11K=build/fat16/win1-r11k.img; REL=build/fat16/win1-r11h.img; NEGV=build/fat16/win1-r11h-neg.img
PCPG=build/r11m/pc-pg/r11m-pg.img; VGAPG=build/r11m/pc-pg-vga/r11m-pg-vga.img
PCDEFK=build/r11m/pc/r11m.img; VGADEFK=build/r11m/pc-vga/r11m-vga.img
hash_all() { for v in $V11K $REL $NEGV; do echo "$v $(sha256sum "$v" | cut -c1-64)"; done; }
hash_all > "$OUT/volumes-before.txt"
S=tests/r11k
pc()  { SHELLIMG=$1 sh scripts/r11h-run-tiny386.sh "$OUT" "$2" "$3" "$4" >/dev/null; }
qemu() { W=build/r11m/qemu-rg-$1; rm -rf "$W"; mkdir -p "$W/out"
  cp "$2" "$W/hda.img"; cp "$3" "$W/hdb.img"
  timeout 3000 python3 scripts/r11k-qemu-capture.py "$W/out" "$W" "$ROOT" --seq "$4" > "$W/capture.log" 2>&1 || true
  cp "$W/out/lines.txt" "$OUT/qemu-$1.txt"; cp "$W/capture.log" "$OUT/qemu-$1.log"; }
( qemu pg-r1 $VGAPG $V11K $S/seq-qemu.txt; echo "qemu pg done"; qemu defk-r1 $VGADEFK $REL tests/r11k/seq-qemu-default.txt; echo "qemu defk done" ) &
pc $PCPG $V11K $S/seq-pc.txt pg-r1
pc $PCPG $NEGV tests/r11h/seq-neg.txt pgneg-r1
pc $PCPG $V11K tests/r11f/seq-regression.txt pgregression
pc $PCDEFK $REL tests/r11h/seq-multi.txt defk-multi-r1
pc $PCDEFK $NEGV tests/r11h/seq-neg.txt defk-neg-r1
pc $PCDEFK $V11K $S/seq-nopaging.txt defk-nopaging-r1
echo "tiny386 groups done"
wait
hash_all > "$OUT/volumes-after.txt"
if cmp -s "$OUT/volumes-before.txt" "$OUT/volumes-after.txt"; then echo "VOLUMES UNCHANGED"; else echo "VOLUME CHANGED"; fi
python3 scripts/r11k-assert.py "$OUT" 1 1 > "$OUT/assert.txt" 2>&1 || true
python3 tests/fix1/regress_filter.py "$OUT/assert.txt" > "$OUT/filter.txt" 2>&1 && echo "REGRESS FILTER OK" || echo "REGRESS FILTER: REAL FAILURES"
cat "$OUT/filter.txt"
echo "FIX1 REGRESS DONE"
