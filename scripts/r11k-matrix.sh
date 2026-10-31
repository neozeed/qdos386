#!/bin/sh
# R11K verification matrix.  Every run is SCRIPTED (keys piped or sent by the run drivers); no human
# session.  Judged afterwards by scripts/r11k-assert.py from the transcripts.
#
#   PAGING (R11K kernels, volume win1-r11k.img), ROUNDS rounds each:
#     i386VM block   tests/r11k/seq-vm.txt (+ R11J SAME/SAMELNCH, MULTI and NEG under the paging kernel)
#     Tiny386 PC     tests/r11k/seq-pc.txt (+ NEG under the paging kernel)
#     QEMU (VGA)     tests/r11k/seq-qemu.txt, QEMU_ROUNDS rounds
#   LARGE paging kernels (pgplain/pgvga + BIGK, 400 and 896 sectors, the R11K loader): Tiny386, QEMU, once
#   PAGING TEST images, once each: failed copy, 2 MiB pool cap, interrupt around every CR3 load,
#     kernel tasks, shared-frames NEGATIVE CONTROL (must FAIL)
#   Paging kernels under the R11F regression sequence (Phoon, time, FPU, SDK programs)
#   DEFAULT images (non-paging; original loader and R11K loader), ROUNDS rounds: MULTI and NEG on
#     i386VM legacy, i386VM block, Tiny386; QEMU VGA smoke with the R11K loader; the R11H regression script
#     once; host unit tests
#   Every FAT16 volume used is hashed before and after.
#   usage: I386VM=<tree> TINY386=<tiny386> FW=<firmware dir> HARNESS_DIR=<dir with i386vm-r9, -block, -block-fd> \
#          [ROUNDS=3] [QEMU_ROUNDS=3] sh scripts/r11k-matrix.sh <outdir>
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
hash_all > "$OUT/volumes-before.txt"
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
# Large paging kernels with the R11K loader (kernel + transfer 400 and 896 sectors).
for t in 400 896; do
  pc build/r11k/bootimg/pgplain-new-$t.img $V11K $S/seq-large.txt large-$t
  qemu large-$t build/r11k/bootimg/pgvga-new-$t.img $V11K $S/seq-qemu.txt
done
echo "large kernels done"
vmb "$HD/i386vm-r9-block" build/r11k/test-vm-flaunch/qdos386-r11k-test-flaunch.bin $V11K $S/seq-t.txt test-flaunch
pc build/r11k/test-pc-flaunch/r11k-test-flaunch.img $V11K $S/seq-t.txt test-flaunch
vmb "$HD/i386vm-r9-block" build/r11k/test-vm-cap/qdos386-r11k-test-cap.bin $V11K $S/seq-e.txt test-cap
vmb "$HD/i386vm-r9-block" build/r11k/test-vm-intcr3/qdos386-r11k-test-intcr3.bin $V11K $S/seq-m-same.txt test-intcr3
pc build/r11k/test-pc-intcr3/r11k-test-intcr3.img $V11K $S/seq-m-same.txt test-intcr3
vmb "$HD/i386vm-r9-block" build/r11k/test-vm-tasks/qdos386-r11k-test-tasks.bin $V11K $S/seq-k.txt test-tasks
pc build/r11k/test-pc-tasks/r11k-test-tasks.img $V11K $S/seq-k.txt test-tasks
vmb "$HD/i386vm-r9-block" build/r11k/test-vm-share/qdos386-r11k-test-share.bin $V11K $S/seq-m.txt control-share
echo "paging test images done"
# review regression: an unpaged task fault must halt, not kill the innocent paged copy (ends with a machine halt)
PC_CMD_TIMEOUT=45 pc $PCPG $V11K $S/seq-y.txt pg-Y
vmb "$HD/i386vm-r9-block" $VMPG $V11K tests/r11f/seq-regression.txt pgregression
pc $PCPG $V11K tests/r11f/seq-regression.txt pgregression
r=1
while [ $r -le "$ROUNDS" ]; do
  vml $LEG tests/r11h/seq-multi.txt def-legacy-multi-r$r
  vml $LEGNEG tests/r11h/seq-neg.txt def-legacy-neg-r$r
  vmb "$HD/i386vm-r9-block" $BLK $REL tests/r11h/seq-multi.txt def-block-multi-r$r
  vmb "$HD/i386vm-r9-block" $BLK $NEGV tests/r11h/seq-neg.txt def-block-neg-r$r
  pc $PCDEF $REL tests/r11h/seq-multi.txt def-multi-r$r
  pc $PCDEF $NEGV tests/r11h/seq-neg.txt def-neg-r$r
  pc $PCDEFK $REL tests/r11h/seq-multi.txt defk-multi-r$r
  pc $PCDEFK $NEGV tests/r11h/seq-neg.txt defk-neg-r$r
  pc $PCDEFK $V11K $S/seq-nopaging.txt defk-nopaging-r$r
  vmb "$HD/i386vm-r9-block" $BLK $V11K $S/seq-nopaging.txt def-block-nopaging-r$r
  echo "default round $r done"
  r=$((r + 1))
done
q=1
while [ $q -le "$QEMU_ROUNDS" ]; do qemu defk-r$q $VGADEFK $REL tests/r11k/seq-qemu-default.txt; q=$((q + 1)); done
I386VM_HARNESS=$HD/i386vm-r9 sh scripts/r11h-regression.sh "$OUT/def-regression" > "$OUT/def-regression.log" 2>&1 || true
sh tests/host/r11i/run.sh > "$OUT/host-tests.txt" 2>&1 || true
python3 tests/host/r11k/test_boot_images.py > "$OUT/host-tests-r11k.txt" 2>&1 || true
hash_all > "$OUT/volumes-after.txt"
if cmp -s "$OUT/volumes-before.txt" "$OUT/volumes-after.txt"; then echo "VOLUMES UNCHANGED"; else echo "VOLUME CHANGED"; fi
echo "MATRIX DONE"
