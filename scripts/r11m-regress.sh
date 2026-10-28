#!/bin/sh
# R11M regression: the R11K / R11H / R11F test sequences, UNCHANGED, run against the R11M kernels (writable FAT16, VFAT, virtual consoles,
# job control, native paths) on the unchanged R11K volumes.  Same transcripts names as
# scripts/r11k-matrix.sh, so scripts/r11k-assert.py judges them (independent of the programs' own PASS lines where the
# transcript allows).  Groups that need R11K-only kernels (large-transfer loaders, paging test images, legacy RAM-disk
# kernels) are not run here: they are covered by the R11K matrix itself (scripts/r11k-matrix.sh) on the frozen kernels.
#   usage: I386VM=<tree> TINY386=<tiny386> HARNESS_DIR=<dir with i386vm-r9, -block, -block-fd> [ROUNDS=1] [QEMU_ROUNDS=1] \
#          sh scripts/r11m-regress.sh <outdir>
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd); cd "$ROOT"
: "${TINY386:?}" "${HARNESS_DIR:?}"
OUT=${1:?usage}; mkdir -p "$OUT"
ROUNDS=${ROUNDS:-1}; QEMU_ROUNDS=${QEMU_ROUNDS:-1}
HD=$(cd "$HARNESS_DIR" && pwd)
V11K=build/fat16/win1-r11k.img; REL=build/fat16/win1-r11h.img; NEGV=build/fat16/win1-r11h-neg.img
VMPG=build/r11m/vm-pg/qdos386-r11m-pg.bin; PCPG=build/r11m/pc-pg/r11m-pg.img; VGAPG=build/r11m/pc-pg-vga/r11m-pg-vga.img
BLK=build/r11m/vm/qdos386-r11m.bin; PCDEFK=build/r11m/pc/r11m.img; VGADEFK=build/r11m/pc-vga/r11m-vga.img
hash_all() { for v in $V11K $REL $NEGV; do echo "$v $(sha256sum "$v" | cut -c1-64)"; done; }
hash_all > "$OUT/volumes-before.txt"
S=tests/r11k
vmb() { I386VM_BLOCK_HARNESS=$1 sh scripts/r11i-run-i386vm-block.sh "$OUT" "$2" "$3" "$4" "$5" >/dev/null; }
pc()  { SHELLIMG=$1 sh scripts/r11h-run-tiny386.sh "$OUT" "$2" "$3" "$4" >/dev/null; }
qemu() { # qemu TAG HDA VOLUME SEQ-FILE
  W=build/r11m/qemu-rg-$1; rm -rf "$W"; mkdir -p "$W/out"
  cp "$2" "$W/hda.img"; cp "$3" "$W/hdb.img"
  timeout 3000 python3 scripts/r11k-qemu-capture.py "$W/out" "$W" "$ROOT" --seq "$4" > "$W/capture.log" 2>&1 || true
  cp "$W/out/lines.txt" "$OUT/qemu-$1.txt"; cp "$W/capture.log" "$OUT/qemu-$1.log"
}
r=1
while [ $r -le "$ROUNDS" ]; do
  vmb "$HD/i386vm-r9-block" $VMPG $V11K $S/seq-vm.txt pg-r$r
  I386VM_FAULTDUMP=1 vmb "$HD/i386vm-r9-block-fd" $VMPG $V11K tests/r11j/seq-fault.txt pgfault-r$r
  vmb "$HD/i386vm-r9-block" $VMPG $V11K tests/r11h/seq-multi.txt pgmulti-r$r
  vmb "$HD/i386vm-r9-block" $VMPG $NEGV tests/r11h/seq-neg.txt pgneg-r$r
  pc $PCPG $V11K $S/seq-pc.txt pg-r$r
  pc $PCPG $NEGV tests/r11h/seq-neg.txt pgneg-r$r
  echo "paging round $r done"
  r=$((r + 1))
done
q=1
while [ $q -le "$QEMU_ROUNDS" ]; do qemu pg-r$q $VGAPG $V11K $S/seq-qemu.txt; echo "qemu round $q done"; q=$((q + 1)); done
vmb "$HD/i386vm-r9-block" $VMPG $V11K tests/r11f/seq-regression.txt pgregression
pc $PCPG $V11K tests/r11f/seq-regression.txt pgregression
r=1
while [ $r -le "$ROUNDS" ]; do
  vmb "$HD/i386vm-r9-block" $BLK $REL tests/r11h/seq-multi.txt def-block-multi-r$r
  vmb "$HD/i386vm-r9-block" $BLK $NEGV tests/r11h/seq-neg.txt def-block-neg-r$r
  pc $PCDEFK $REL tests/r11h/seq-multi.txt defk-multi-r$r
  pc $PCDEFK $NEGV tests/r11h/seq-neg.txt defk-neg-r$r
  pc $PCDEFK $V11K $S/seq-nopaging.txt defk-nopaging-r$r
  vmb "$HD/i386vm-r9-block" $BLK $V11K $S/seq-nopaging.txt def-block-nopaging-r$r
  echo "non-paging R11M round $r done"
  r=$((r + 1))
done
q=1
while [ $q -le "$QEMU_ROUNDS" ]; do qemu defk-r$q $VGADEFK $REL tests/r11k/seq-qemu-default.txt; q=$((q + 1)); done
# the R11F sequence on the NON-paging R11L kernel: the paging kernel's PASS/FAIL lines must equal it (scripts/r11k-assert.py)
vmb "$HD/i386vm-r9-block" $BLK $V11K tests/r11f/seq-regression.txt regression-none
mkdir -p "$OUT/def-regression"; mv "$OUT/i386vm-regression-none.txt" "$OUT/def-regression/"
hash_all > "$OUT/volumes-after.txt"
if cmp -s "$OUT/volumes-before.txt" "$OUT/volumes-after.txt"; then echo "VOLUMES UNCHANGED"; else echo "VOLUME CHANGED"; fi
echo "REGRESS DONE"
