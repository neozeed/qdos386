#!/bin/sh
# R11L final evidence run: every stage's tests, on the final build, on every emulator, judged independently.
#   usage: I386VM=... QDOS386_TOOLS=... TINY386=... HARNESS_DIR=... sh scripts/r11l-final.sh OUTDIR
# Each step writes its own directory under OUTDIR and one line "STEP name: PASS|FAIL" to OUTDIR/summary.txt.
set -u
ROOT=$(cd "$(dirname "$0")/.." && pwd); cd "$ROOT"
OUT=${1:?usage}; mkdir -p "$OUT"; : > "$OUT/summary.txt"
: "${I386VM:?}" "${QDOS386_TOOLS:?}" "${TINY386:?}" "${HARNESS_DIR:?}"
step() { # step NAME COMMAND...
  n=$1; shift
  if "$@" > "$OUT/$n.log" 2>&1; then echo "STEP $n: PASS" | tee -a "$OUT/summary.txt"; else echo "STEP $n: FAIL (see $OUT/$n.log)" | tee -a "$OUT/summary.txt"; fi
}
VMK=build/r11l/vm-fs-pg/qdos386-r11l-fs-pg.bin
PGVGA=build/r11l/pc-fs-pg-vga/r11l-fs-pg-vga.img; PGPC=build/r11l/pc-fs-pg/r11l-fs-pg.img

step default-images sh scripts/r11l-default-check.sh "$OUT/default-images.txt"
step host-tests sh tests/host/r11l/run.sh
step stage1-matrix sh scripts/r11l-stage1-matrix.sh "$OUT/stage1"
for p in vm pc qemu; do step gate23-$p sh scripts/r11l-gate23.sh $p "$OUT/gate23"; done

# Stage 4 (build driver) on the i386VM paging kernel: 108 checks
s4() {
  mkdir -p "$OUT/stage4"
  python3 tests/r11l/stage4.py seq "$OUT/stage4/vm.seq.txt" s4 || return 1
  sh scripts/r11l-pipevol.sh "$OUT/stage4/vm.initial.img" BADINC.C=tests/r11l/src/BADINC.C BADERR.C=tests/r11l/src/BADERR.C || return 1
  cp "$OUT/stage4/vm.initial.img" "$OUT/stage4/vm.final.img"
  sh scripts/r11l-run-vm.sh $VMK "$OUT/stage4/vm.final.img" "$OUT/stage4/vm.seq.txt" "$OUT/stage4/vm.txt" 1800 || true
  python3 -I tests/r11l/stage4.py judge "$OUT/stage4/vm.txt" "$OUT/stage4/vm.final.img" | tee "$OUT/stage4/vm.judge.txt" | tail -3
  grep -q 'SUMMARY.*fail=0' "$OUT/stage4/vm.judge.txt"
}
step stage4-vm s4

# Stage 5: create/build/run/change/invalid/reboot on three emulators, two sessions each on one disk image
e2e_vol() { sh scripts/r11l-pipevol.sh "$1" > /dev/null; }
e2e_vm() {
  d=$OUT/stage5; mkdir -p $d; e2e_vol $d/vm.initial.img || return 1; cp $d/vm.initial.img $d/vm.img
  python3 tests/r11l/stage4.py seq $d/vm-a.seq.txt e2e-a; python3 tests/r11l/stage4.py seq $d/vm-b.seq.txt e2e-b
  sh scripts/r11l-run-vm.sh $VMK $d/vm.img $d/vm-a.seq.txt $d/vm-a.txt 1800 || true
  python3 -I tests/r11l/stage4.py judge-e2e e2e-a $d/vm-a.txt $d/vm.img > $d/vm-a.judge.txt; cp $d/vm.img $d/vm.after-a.img
  sh scripts/r11l-run-vm.sh $VMK $d/vm.img $d/vm-b.seq.txt $d/vm-b.txt 1800 || true
  python3 -I tests/r11l/stage4.py judge-e2e e2e-b $d/vm-b.txt $d/vm.img $d/vm-a.txt > $d/vm-b.judge.txt
  tail -qn1 $d/vm-a.judge.txt $d/vm-b.judge.txt; grep -q 'fail=0' $d/vm-a.judge.txt && grep -q 'fail=0' $d/vm-b.judge.txt
}
e2e_qemu() {
  d=$OUT/stage5; mkdir -p $d; export R11L_QKERNEL=$PGVGA
  e2e_vol $d/qemu.initial.img || return 1
  sh scripts/r11l-run-qemu.sh $d/qemu.initial.img $d/vm-a.seq.txt $d qemu-a 3000 > /dev/null
  python3 -I tests/r11l/stage4.py judge-e2e e2e-a $d/qemu-qemu-a.txt $d/qemu-qemu-a.final.img > $d/qemu-a.judge.txt
  sh scripts/r11l-run-qemu.sh $d/qemu-qemu-a.final.img $d/vm-b.seq.txt $d qemu-b 3000 > /dev/null
  python3 -I tests/r11l/stage4.py judge-e2e e2e-b $d/qemu-qemu-b.txt $d/qemu-qemu-b.final.img $d/qemu-qemu-a.txt > $d/qemu-b.judge.txt
  tail -qn1 $d/qemu-a.judge.txt $d/qemu-b.judge.txt; grep -q 'fail=0' $d/qemu-a.judge.txt && grep -q 'fail=0' $d/qemu-b.judge.txt
}
e2e_tiny() {
  d=$OUT/stage5; mkdir -p $d; e2e_vol $d/tiny.initial.img || return 1
  python3 tests/r11l/stage4.py seq $d/tiny-a.seq.txt e2e-a; python3 tests/r11l/stage4.py seq $d/tiny-b.seq.txt e2e-b
  PC_CMD_TIMEOUT=1500 PC_INPUT_PAUSE=2 sh scripts/r11l-run-pc.sh $d/tiny.initial.img $d/tiny-a.seq.txt $d tiny-a $PGPC > /dev/null
  python3 -I tests/r11l/stage4.py judge-e2e e2e-a $d/tiny386-tiny-a.txt $d/tiny386-tiny-a.final.img > $d/tiny-a.judge.txt
  PC_CMD_TIMEOUT=900 PC_INPUT_PAUSE=2 sh scripts/r11l-run-pc.sh $d/tiny386-tiny-a.final.img $d/tiny-b.seq.txt $d tiny-b $PGPC > /dev/null
  python3 -I tests/r11l/stage4.py judge-e2e e2e-b $d/tiny386-tiny-b.txt $d/tiny386-tiny-b.final.img $d/tiny386-tiny-a.txt > $d/tiny-b.judge.txt
  tail -qn1 $d/tiny-a.judge.txt $d/tiny-b.judge.txt; grep -q 'fail=0' $d/tiny-a.judge.txt && grep -q 'fail=0' $d/tiny-b.judge.txt
}
step stage5-vm e2e_vm
step stage5-qemu e2e_qemu
step stage5-tiny386 e2e_tiny

# Regressions of the earlier rounds on the R11L (writable-FAT, paging) kernels, judged by the R11K assert script
rg() { mkdir -p "$OUT/regress-r11l-kernels"; ROUNDS=1 QEMU_ROUNDS=1 sh scripts/r11l-regress.sh "$OUT/regress-r11l-kernels"; }
step regress-r11l-kernels rg
python3 scripts/r11k-assert.py "$OUT/regress-r11l-kernels" 1 1 > "$OUT/regress-r11l-kernels.assert.txt" 2>&1 || true
tail -1 "$OUT/regress-r11l-kernels.assert.txt" | tee -a "$OUT/summary.txt"
python3 tests/r11l/regress_filter.py "$OUT/regress-r11l-kernels.assert.txt" > "$OUT/regress-r11l-kernels.filter.txt" 2>&1 \
  && echo "STEP regress-judge: PASS ($(sed -n 1p "$OUT/regress-r11l-kernels.filter.txt"); groups not run here are covered by the R11K matrix)" | tee -a "$OUT/summary.txt" \
  || echo "STEP regress-judge: FAIL" | tee -a "$OUT/summary.txt"
step integrity sh scripts/r11l-integrity.sh "$OUT" "$OUT/integrity.txt"
echo "FINAL RUN DONE" | tee -a "$OUT/summary.txt"
