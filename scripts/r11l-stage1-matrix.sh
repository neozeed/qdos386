#!/bin/sh
# R11L Stage 1 matrix (writable FAT16).  Every run is scripted; the judge is the guest's own PASS/FAIL
# lines AND the independent host checks (tools/r11l-fatcheck.py, fsck.fat -n) of the resulting image.
#   i386VM block (rw harness): full suite on a 3 MiB volume (1 sector/cluster) and a 10 MiB volume
#     (4 sectors/cluster, 128-entry root), disk-full on a small volume, read-only device, I/O error
#     injection, Gate 1 persistence across three emulator restarts, interrupted-operation sweep
#   Tiny386 PC and QEMU PC: full suite with root-full
#   usage: I386VM=<tree> TINY386=<tiny386_headless> sh scripts/r11l-stage1-matrix.sh OUTDIR
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd); cd "$ROOT"
: "${TINY386:?}"
OUT=${1:?usage}; mkdir -p "$OUT"
K=build/r11l/vm-fs/qdos386-r11l-fs.bin
PCI=build/r11l/pc-fs/r11l-fs.img; QI=build/r11l/pc-fs-vga/r11l-fs-vga.img
: > "$OUT/summary.txt"
note() { echo "$*" | tee -a "$OUT/summary.txt"; }
judge() {  # judge TAG IMG TRANSCRIPT [EXPECT_FAIL=0]
  # the guest's own summary line is the count (a screen capture repeats lines, so counting PASS lines would not do)
  P=$(grep 'WRTTEST SUMMARY' "$3" | tail -1 | sed 's/.*pass=\([0-9]*\).*/\1/'); F=$(grep 'WRTTEST SUMMARY' "$3" | tail -1 | sed 's/.*fail=\([0-9]*\).*/\1/')
  [ -n "$P" ] || P=none; [ -n "$F" ] || F=none
  FL=$(grep -c '^FAIL' "$3" || true)
  S=$(grep -c 'WRTTEST DONE' "$3" || true)
  python3 tools/r11l-fatcheck.py "$2" > "$OUT/$1.check.txt" 2>&1 && C=clean || C=ERRORS
  fsck.fat -n "$2" > "$OUT/$1.fsck.txt" 2>&1 && FS=clean || FS=ERRORS
  note "$1: guest pass=$P fail=$F fail-lines=$FL done=$S | fatcheck=$C fsck.fat=$FS | $(sha256sum "$2" | cut -c1-16)"
}
vm() {  # vm TAG KIB SPC ROOT SEQLINE [MODE] [FAULT]
  TAG=$1; sh scripts/r11l-s1vol.sh "$OUT/$TAG.initial.img" "$2" "$3" "$4"
  cp "$OUT/$TAG.initial.img" "$OUT/$TAG.final.img"
  printf '%s\n' "$5" > "$OUT/$TAG.seq.txt"
  MODE=${6:-rw} QDOS_BLK_FAULT=${7:-} sh scripts/r11l-run-vm.sh "$K" "$OUT/$TAG.final.img" "$OUT/$TAG.seq.txt" "$OUT/$TAG.txt"
  judge "$TAG" "$OUT/$TAG.final.img" "$OUT/$TAG.txt"
}
note "== i386VM block, writable harness"
vm vm-3m-spc1 3072 1 512 'RUN WRTTEST.AOU rootN=453'
vm vm-10m-spc4 10240 4 128 'RUN WRTTEST.AOU rootN=69'
vm vm-full-small 2100 1 512 'RUN WRTTEST.AOU full'
vm vm-full-spc2 4300 2 512 'RUN WRTTEST.AOU full'
vm vm-readonly 3072 1 512 'RUN WRTTEST.AOU ro' ro
cmp "$OUT/vm-readonly.initial.img" "$OUT/vm-readonly.final.img" && note "vm-readonly: image byte-identical before/after"
vm vm-ioerr 3072 1 512 'RUN WRTTEST.AOU ioerr' rw io:6
note "== Gate 1 persistence (three separate emulator processes)"
sh scripts/r11l-persist.sh "$K" "$OUT/persist"
for r in run1 run2; do grep -c '^PASS' "$OUT/persist/$r.txt" | sed "s/^/persist $r pass=/" | tee -a "$OUT/summary.txt"; done
note "persist: after1 $(python3 tools/r11l-fatcheck.py $OUT/persist/persist-after1.img | tail -1)"
note "persist: after2 $(python3 tools/r11l-fatcheck.py $OUT/persist/persist-after2.img | tail -1)"
note "== interrupted operations (power cut at every write of each operation)"
sh scripts/r11l-interrupt.sh "$K" "$OUT/interrupt" | tail -1 | tee -a "$OUT/summary.txt"
python3 - "$OUT/interrupt/sweep.csv" "$OUT/summary.txt" <<'PY'
import csv, sys, collections
rows = list(csv.DictReader(open(sys.argv[1])))
c = collections.Counter()
bad = 0
for r in rows:
    key = 'completed' if r['completed'] == '1' else ('consistent' if r['fatcheck_errors'] == '0' and r['fatcheck_warnings'] == '0'
          else ('leak-only' if r['fatcheck_errors'] == '0' else 'detected-inconsistent'))
    c[key] += 1
    if r['semantic'] == 'broken' or r['repaired_clean'] != 'yes':
        bad += 1
    # the independent checker must agree with ours on clean vs not clean
    if (r['fsck_rc'] == '0') != (r['fatcheck_errors'] == '0' and r['fatcheck_warnings'] == '0'):
        bad += 1
line = 'sweep: %d cuts: %s; broken-or-unrepairable-or-checker-disagreement=%d' % (len(rows), dict(c), bad)
print(line)
open(sys.argv[2], 'a').write(line + '\n')
PY
note "== Tiny386 PC"

sh scripts/r11l-s1vol.sh "$OUT/pc.initial.img"
printf 'RUN WRTTEST.AOU rootN=453\n' > "$OUT/pc.seq.txt"
SHELLIMG=$PCI sh scripts/r11h-run-tiny386.sh "$OUT" "$OUT/pc.initial.img" "$OUT/pc.seq.txt" pc >/dev/null
cp build/pc-r11h-pc-run/win1-r11h.img "$OUT/pc.final.img"
judge pc "$OUT/pc.final.img" "$OUT/tiny386-pc.txt"
note "== QEMU PC (VGA console, monitor capture)"
W=build/r11l/qemu-s1; rm -rf "$W"; mkdir -p "$W/out"
cp $QI "$W/hda.img"; sh scripts/r11l-s1vol.sh "$W/hdb.img"; cp "$W/hdb.img" "$OUT/qemu.initial.img"
printf 'RUN WRTTEST.AOU rootN=453\n' > "$OUT/qemu.seq.txt"
timeout 1500 python3 scripts/r11l-qemu-capture.py "$W/out" "$W" "$ROOT" --seq "$OUT/qemu.seq.txt" > "$OUT/qemu.log" 2>&1 || true
cp "$W/out/lines.txt" "$OUT/qemu.txt"; cp "$W/hdb.img" "$OUT/qemu.final.img"
judge qemu "$OUT/qemu.final.img" "$OUT/qemu.txt"
echo "stage1 matrix finished"
