#!/bin/sh
# R11J verification matrix.  Every run is SCRIPTED (keys piped or sent by the run drivers); no
# human session.  Results are judged afterwards by scripts/r11j-assert.py from the transcripts.
#
#   POC (paging builds), ROUNDS rounds each:
#     i386VM block   SAMELNCH main/N/R; SAMELNCH F on the faultdump harness (the CPU core ends
#                    the run at the first guest exception); MULTI and NEG under the POC kernel
#     Tiny386 PC     SAMELNCH main/F/N/R and MULTI; NEG under the POC kernel
#     QEMU (VGA)     SAMELNCH main/F/N/R (QEMU_ROUNDS rounds)
#   POC test-only images, once each: failed copy (i386VM, PC), 2 MiB pool cap (i386VM), interrupt
#     around every CR3 load (i386VM, PC), shared-frames NEGATIVE CONTROL (i386VM, must FAIL);
#     R11F regression sequence under the i386VM POC kernel
#   Default (non-paging) images, unchanged from R11I, ROUNDS rounds: MULTI and NEG on i386VM
#     legacy, i386VM block and Tiny386; the R11H regression script once; host unit tests
#   Every FAT16 volume used is hashed before and after.
#
#   usage: I386VM=<tree> TINY386=<tiny386> HARNESS_DIR=<dir with i386vm-r9, -block, -block-fd> \
#          [ROUNDS=3] [QEMU_ROUNDS=3] sh scripts/r11j-matrix.sh <outdir>
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd); cd "$ROOT"
: "${TINY386:?}" "${HARNESS_DIR:?}"
OUT=${1:?usage: r11j-matrix.sh OUTDIR}; mkdir -p "$OUT"
ROUNDS=${ROUNDS:-3}; QEMU_ROUNDS=${QEMU_ROUNDS:-3}
HD=$(cd "$HARNESS_DIR" && pwd)
R11J=build/fat16/win1-r11j.img; REL=build/fat16/win1-r11h.img; NEGV=build/fat16/win1-r11h-neg.img
VMPOC=build/r11j/vm-poc/qdos386-r11j-poc.bin; PCPOC=build/r11j/pc-poc/r11j-poc.img
VGAPOC=build/r11j/pc-poc-vga/r11j-poc-vga.img
LEG=build/i386vm-r11h-rel/qdos386-r11h-rel.bin; LEGNEG=build/i386vm-r11h-neg/qdos386-r11h-neg.bin
BLK=build/r11i/block/qdos386-r11i-block.bin; PCDEF=build/pc-r11h/r11h-shell.img
hash_all() { for v in $R11J $REL $NEGV; do echo "$v $(sha256sum "$v" | cut -c1-64)"; done; }
hash_all > "$OUT/volumes-before.txt"
S=tests/r11j
printf 'RUN SAMELNCH.AOU\nRUN SAMELNCH.AOU N\nRUN SAMELNCH.AOU R\n' > "$S/seq-vm.txt"
printf 'RUN SAMELNCH.AOU\nRUN SAMELNCH.AOU F\nRUN SAMELNCH.AOU N\nRUN SAMELNCH.AOU R\nRUN MULTI.AOU\n' > "$S/seq-pc.txt"

vmb() {  # vmb HARNESS IMAGE VOLUME SEQ TAG
  I386VM_BLOCK_HARNESS=$1 sh scripts/r11i-run-i386vm-block.sh "$OUT" "$2" "$3" "$4" "$5" >/dev/null
}
vml() {  # vml IMAGE SEQ TAG  (one-argument harness: RAM-disk images)
  I386VM_HARNESS=$HD/i386vm-r9 sh scripts/r11h-run-i386vm.sh "$OUT" "$1" "$2" "$3" >/dev/null
}
pc() {   # pc KERNEL_DISK VOLUME SEQ TAG
  SHELLIMG=$1 sh scripts/r11h-run-tiny386.sh "$OUT" "$2" "$3" "$4" >/dev/null
}
qemu() { # qemu TAG
  W=build/r11j/qemu-$1; rm -rf "$W"; mkdir -p "$W/out"
  cp "$VGAPOC" "$W/hda.img"; cp "$R11J" "$W/hdb.img"
  timeout 3000 python3 scripts/r11j-qemu-capture.py "$W/out" "$W" "$ROOT" "run samelnch.aou" \
    "run samelnch.aou F" "run samelnch.aou N" "run samelnch.aou R" "run multi.aou" > "$W/capture.log" 2>&1 || true
  cp "$W/out/lines.txt" "$OUT/qemu-$1.txt"; cp "$W/capture.log" "$OUT/qemu-$1.log"
}

r=1
while [ $r -le "$ROUNDS" ]; do
  vmb "$HD/i386vm-r9-block" $VMPOC $R11J $S/seq-vm.txt poc-r$r
  I386VM_FAULTDUMP=1 vmb "$HD/i386vm-r9-block-fd" $VMPOC $R11J $S/seq-fault.txt pocfault-r$r
  vmb "$HD/i386vm-r9-block" $VMPOC $R11J tests/r11h/seq-multi.txt pocmulti-r$r
  vmb "$HD/i386vm-r9-block" $VMPOC $NEGV tests/r11h/seq-neg.txt pocneg-r$r
  pc $PCPOC $R11J $S/seq-pc.txt poc-r$r
  pc $PCPOC $NEGV tests/r11h/seq-neg.txt pocneg-r$r
  echo "poc round $r done"
  r=$((r + 1))
done
q=1
while [ $q -le "$QEMU_ROUNDS" ]; do qemu poc-r$q; echo "qemu round $q done"; q=$((q + 1)); done

vmb "$HD/i386vm-r9-block" build/r11j/test-vm-flaunch/qdos386-r11j-test-flaunch.bin $R11J $S/seq-flaunch.txt test-flaunch
pc build/r11j/test-pc-flaunch/r11j-test-flaunch.img $R11J $S/seq-flaunch.txt test-flaunch
vmb "$HD/i386vm-r9-block" build/r11j/test-vm-cap/qdos386-r11j-test-cap.bin $R11J $S/seq-main.txt test-cap
vmb "$HD/i386vm-r9-block" build/r11j/test-vm-intcr3/qdos386-r11j-test-intcr3.bin $R11J $S/seq-main.txt test-intcr3
pc build/r11j/test-pc-intcr3/r11j-test-intcr3.img $R11J $S/seq-main.txt test-intcr3
vmb "$HD/i386vm-r9-block" $VMPOC $R11J tests/r11f/seq-regression.txt pocregression
# NEGATIVE CONTROL: every copy maps the first copy's frames; the proof must FAIL.
vmb "$HD/i386vm-r9-block" build/r11j/test-vm-share/qdos386-r11j-test-share.bin $R11J $S/seq-main.txt control-share
echo "poc test images done"

r=1
while [ $r -le "$ROUNDS" ]; do
  vml $LEG tests/r11h/seq-multi.txt def-legacy-multi-r$r
  vml $LEGNEG tests/r11h/seq-neg.txt def-legacy-neg-r$r
  vmb "$HD/i386vm-r9-block" $BLK $REL tests/r11h/seq-multi.txt def-block-multi-r$r
  vmb "$HD/i386vm-r9-block" $BLK $NEGV tests/r11h/seq-neg.txt def-block-neg-r$r
  pc $PCDEF $REL tests/r11h/seq-multi.txt def-multi-r$r
  pc $PCDEF $NEGV tests/r11h/seq-neg.txt def-neg-r$r
  echo "default round $r done"
  r=$((r + 1))
done
I386VM_HARNESS=$HD/i386vm-r9 sh scripts/r11h-regression.sh "$OUT/def-regression" > "$OUT/def-regression.log" 2>&1 || true
sh tests/host/r11i/run.sh > "$OUT/host-tests.txt" 2>&1 || true

hash_all > "$OUT/volumes-after.txt"
if cmp -s "$OUT/volumes-before.txt" "$OUT/volumes-after.txt"; then echo "VOLUMES UNCHANGED"; else echo "VOLUME CHANGED"; fi
echo "MATRIX DONE"
