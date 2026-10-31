#!/bin/sh
# R11M core matrix, one round on one emulator.  Every case boots the kernel afresh on a disposable copy of a deterministic scratch
# volume (scripts/r11m-demovol.sh), writes <OUTDIR>/<emu>-<case>.txt (+ final image where the volume is writable) and does not judge;
# scripts/r11m-matrix-judge.sh judges the transcripts and the final images.
#   usage: sh scripts/r11m-matrix.sh vm|pc|qemu OUTDIR [CASES...]        (default: all cases for the emulator)
# Cases:  fs  (s1a, s1b: directories, paths, reboot, PATHT)     con  (CONT1)     conin (console input / focus)
#         job (s3a JOBT, s3b table)   jobq (qemu: s3q Alt+F keys, KILL/WAIT)   phoon (background Phoon)   stress (vm: console input stress)
#         vfat (scripts/r11m-vfat-gate.sh)   demo (scripts/r11m-demo.sh)
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd); cd "$ROOT"
EMU=${1:?usage}; OUT=${2:?usage}; shift 2
mkdir -p "$OUT"
CASES=${*:-}
if [ -z "$CASES" ]; then
  case $EMU in
    vm)   CASES="fs con conin job phoon stress vfat demo" ;;
    pc)   CASES="fs con conin job vfat demo" ;;
    qemu) CASES="fs con conin job jobq vfat demo" ;;
  esac
fi
VMK=build/r11m/vm/qdos386-r11m.bin; VMP=build/r11m/vm-pg/qdos386-r11m-pg.bin
PCK=build/r11m/pc/r11m.img; PCP=build/r11m/pc-pg/r11m-pg.img
QK=build/r11m/pc-vga/r11m-vga.img; QP=build/r11m/pc-pg-vga/r11m-pg-vga.img
# run EMU KERNEL(default|paging) VOL SEQ TAG  -> $OUT/$EMU-TAG.txt; the volume ends up in $OUT/$EMU-TAG.final.img
run() {
  kind=$1; vol=$2; seq=$3; tag=$4
  case $EMU in
    vm)   [ "$kind" = paging ] && k=$VMP || k=$VMK
          cp "$vol" "$OUT/vm-$tag.final.img"
          sh scripts/r11l-run-vm.sh $k "$OUT/vm-$tag.final.img" "$seq" "$OUT/vm-$tag.txt" 600 ;;
    pc)   [ "$kind" = paging ] && k=$PCP || k=$PCK
          PC_CMD_TIMEOUT=300 sh scripts/r11m-run-pc.sh "$vol" "$seq" "$OUT" "$tag" $k >/dev/null ;;
    qemu) [ "$kind" = paging ] && k=$QP || k=$QK
          R11M_QKERNEL=$k QEMU_CMD_TIMEOUT=300 sh scripts/r11m-run-qemu.sh "$vol" "$seq" "$OUT" "$tag" 1800 >/dev/null ;;
  esac
}
fin() { case $EMU in vm) echo "$OUT/vm-$1.final.img" ;; pc) echo "$OUT/tiny386-$1.final.img" ;; qemu) echo "$OUT/qemu-$1.final.img" ;; esac; }
new() { sh scripts/r11m-demovol.sh "$OUT/vol-$1.img" 3072; echo "$OUT/vol-$1.img"; }
for c in $CASES; do
  echo "== $EMU case $c"
  case $c in
    fs)    v=$(new fs); run default "$v" tests/r11m/seq-s1a.txt fs-a; run default "$(fin fs-a)" tests/r11m/seq-s1b.txt fs-b ;;
    con)   printf 'RUN CONT1.AOU\nEXIT\n' > "$OUT/seq-con.txt"; v=$(new con); run default "$v" "$OUT/seq-con.txt" con ;;
    conin) v=$(new conin)
           case $EMU in
             vm)   run default "$v" tests/r11m/seq-s2i-vm.txt conin ;;
             pc)   run default "$v" tests/r11m/seq-s2t.txt conin ;;
             qemu) run default "$v" tests/r11m/seq-s2i.txt conin ;;
           esac ;;
    job)   v=$(new job); run paging "$v" tests/r11m/seq-s3a.txt job-a
           v=$(new job2); case $EMU in vm) run paging "$v" tests/r11m/seq-s3b.txt job-b ;; esac ;;
    jobq)  v=$(new jobq); run paging "$v" tests/r11m/seq-s3q.txt jobq ;;
    phoon) v=$(new phoon); run paging "$v" tests/r11m/seq-s3p.txt phoon ;;
    stress) v=$(new stress); run default "$v" tests/r11m/seq-s2stress-vm.txt stress ;;
    vfat)  sh scripts/r11m-vfat-gate.sh $EMU "$OUT/vfat" > "$OUT/$EMU-vfat.txt" 2>&1 || true ;;
    demo)  sh scripts/r11m-demo.sh $EMU "$OUT/demo" > "$OUT/$EMU-demo.txt" 2>&1 || true ;;
    *) echo "unknown case $c" >&2; exit 2 ;;
  esac
done
echo "MATRIX $EMU DONE"
