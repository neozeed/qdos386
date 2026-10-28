#!/bin/sh
# R11M-FIX1 canonical bounded gate (replaces the multi-hour three-round matrices as the default acceptance run).
#   QEMU full-machine (VGA)     PRIMARY    : Stage 4 VFAT gate (reboots, host tools judge) and the bounded regression's QEMU groups
#   Tiny386 full-machine        SECONDARY  : shutdown/persistence matrix (patched emulator), one round by default (FIX1_ROUNDS=3 for three)
#   CPU-only i386VM             NOT RUN    : legacy diagnostic, see scripts/legacy-i386vm.sh
# Every part has bounded timeouts and prints its own PASS/FAIL marker; this script prints "FIX1 GATE PASS" only if every part ran and passed.
#   usage: TINY386=<patched tiny386_headless> sh scripts/fix1-gate.sh OUTDIR [qemu|tiny386|all]
set -u
ROOT=$(cd "$(dirname "$0")/.." && pwd); cd "$ROOT"
OUT=${1:?usage}; WHAT=${2:-all}; mkdir -p "$OUT"
BAD=0
if [ "$WHAT" = qemu ] || [ "$WHAT" = all ]; then
  sh scripts/r11m-vfat-gate.sh qemu "$OUT/qemu-vfat" > "$OUT/qemu-vfat.log" 2>&1; tail -1 "$OUT/qemu-vfat.log"
  grep -q "^GATE qemu PASS" "$OUT/qemu-vfat.log" || BAD=$((BAD + 1))
fi
if [ "$WHAT" = tiny386 ] || [ "$WHAT" = all ]; then
  : "${TINY386:?set TINY386 to the patched tiny386_headless}"
  sh scripts/fix1-matrix.sh "$OUT/tiny386" "${FIX1_ROUNDS:-1}" > "$OUT/tiny386.log" 2>&1; tail -1 "$OUT/tiny386.log"
  grep -q "^FIX1 TINY386 MATRIX PASS" "$OUT/tiny386.log" || BAD=$((BAD + 1))
fi
echo "i386VM: NOT RUN (legacy diagnostic; scripts/legacy-i386vm.sh)"
if [ $BAD -eq 0 ]; then echo "FIX1 GATE PASS ($WHAT)"; else echo "FIX1 GATE FAIL ($BAD part(s))"; fi
[ $BAD -eq 0 ]
