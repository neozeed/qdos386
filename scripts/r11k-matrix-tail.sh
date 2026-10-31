#!/bin/sh
# R11K: re-run of only the final matrix stages (QEMU default-kernel smoke, R11H regression, host tests, volume hashes)
# after the QEMU capture completion test was corrected.  Same preamble as r11k-matrix.sh; volumes-before.txt is kept from the full run.
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd); cd "$ROOT"
: "${TINY386:?}" "${HARNESS_DIR:?}"
OUT=${1:?usage: r11k-matrix.sh OUTDIR}; mkdir -p "$OUT"
ROUNDS=${ROUNDS:-3}; QEMU_ROUNDS=${QEMU_ROUNDS:-3}
HD=$(cd "$HARNESS_DIR" && pwd)
V11K=build/fat16/win1-r11k.img; REL=build/fat16/win1-r11h.img; NEGV=build/fat16/win1-r11h-neg.img
VMPG=build/r11k/vm-paging/qdos386-r11k-paging.bin; PCPG=build/r11k/pc-paging/r11k-paging.img
VGAPG=build/r11k/pc-paging-vga/r11k-paging-vga.img
LEG=build/i386vm-r11h-rel/qdos386-r11h-rel.bin; LEGNEG=build/i386vm-r11h-neg/qdos386-r11h-neg.bin
BLK=build/r11i/block/qdos386-r11i-block.bin; PCDEF=build/pc-r11h/r11h-shell.img
PCDEFK=build/r11k/pc-default/r11k-default.img; VGADEFK=build/r11k/pc-default-vga/r11k-default-vga.img
hash_all() { for v in $V11K $REL $NEGV; do echo "$v $(sha256sum "$v" | cut -c1-64)"; done; }
S=tests/r11k
vmb() { I386VM_BLOCK_HARNESS=$1 sh scripts/r11i-run-i386vm-block.sh "$OUT" "$2" "$3" "$4" "$5" >/dev/null; }
vml() { I386VM_HARNESS=$HD/i386vm-r9 sh scripts/r11h-run-i386vm.sh "$OUT" "$1" "$2" "$3" >/dev/null; }
pc()  { SHELLIMG=$1 sh scripts/r11h-run-tiny386.sh "$OUT" "$2" "$3" "$4" >/dev/null; }
qemu() { # qemu TAG HDA VOLUME SEQ-FILE
  W=build/r11k/qemu-$1; rm -rf "$W"; mkdir -p "$W/out"
  cp "$2" "$W/hda.img"; cp "$3" "$W/hdb.img"
  timeout 3000 python3 scripts/r11k-qemu-capture.py "$W/out" "$W" "$ROOT" --seq "$4" > "$W/capture.log" 2>&1 || true
  cp "$W/out/lines.txt" "$OUT/qemu-$1.txt"; cp "$W/capture.log" "$OUT/qemu-$1.log"
}
q=1
while [ $q -le "$QEMU_ROUNDS" ]; do qemu defk-r$q $VGADEFK $REL tests/r11k/seq-qemu-default.txt; q=$((q + 1)); done
I386VM_HARNESS=$HD/i386vm-r9 sh scripts/r11h-regression.sh "$OUT/def-regression" > "$OUT/def-regression.log" 2>&1 || true
sh tests/host/r11i/run.sh > "$OUT/host-tests.txt" 2>&1 || true
python3 tests/host/r11k/test_boot_images.py > "$OUT/host-tests-r11k.txt" 2>&1 || true
hash_all > "$OUT/volumes-after.txt"
if cmp -s "$OUT/volumes-before.txt" "$OUT/volumes-after.txt"; then echo "VOLUMES UNCHANGED"; else echo "VOLUME CHANGED"; fi
echo "MATRIX DONE"
