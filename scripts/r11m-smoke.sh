#!/bin/sh
# R11M clean-extraction runtime smoke: run from the ROOT OF AN EXTRACTED PACKAGE (qdos386-R11M-OS-USABILITY-STRETCH/).  It uses only
# files of the package (kernels, harness, initial volume, sequences, judge) and the host tools (fsck.fat, mtools, python3):
#   1. every file listed in SHA256SUMS verifies
#   2. i386VM (always): the integrated demonstration on the paging kernel, phase A then phase B after a reboot, judged by
#      scripts/r11m-demo-judge.py, fsck.fat, tools/r11m-fatwalk.py and mtools
#   3. Tiny386 (if TINY386 is set) and QEMU (if qemu-system-i386 exists and SMOKE_QEMU=1): the VFAT sequences s4a then s4b on the plain kernel
#   usage: sh scripts/r11m-smoke.sh OUTDIR
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd); cd "$ROOT"
OUT=${1:?usage}; mkdir -p "$OUT"
export MTOOLS_SKIP_CHECK=1 LC_ALL=C.UTF-8
BAD=0
ok()  { echo "ok   $1"; }
bad() { echo "FAIL $1"; BAD=$((BAD + 1)); }
if sha256sum -c SHA256SUMS --quiet > "$OUT/sha256-check.txt" 2>&1; then ok "SHA256SUMS: $(wc -l < SHA256SUMS) files verify"; else bad "SHA256SUMS"; fi
H=build/r11l/harness/i386vm-r9-blockrw
VOL0=volumes/r11m-demo-initial.img
cp "$VOL0" "$OUT/work-vm.img"
sh scripts/r11l-run-vm.sh build/r11m/vm-pg/qdos386-r11m-pg.bin "$OUT/work-vm.img" tests/r11m/seq-demo-vm-a.txt "$OUT/vm-a.txt" 600
sh scripts/r11l-run-vm.sh build/r11m/vm-pg/qdos386-r11m-pg.bin "$OUT/work-vm.img" tests/r11m/seq-demo-b.txt "$OUT/vm-b.txt" 600
if python3 scripts/r11m-demo-judge.py vm "$OUT/vm-a.txt" "$OUT/vm-b.txt" > "$OUT/vm-judge.txt"; then ok "i386VM demonstration: $(tail -1 "$OUT/vm-judge.txt")"; else bad "i386VM demonstration"; fi
fsck.fat -n "$OUT/work-vm.img" > "$OUT/vm-fsck.txt" 2>&1 && ok "i386VM: fsck.fat clean" || bad "i386VM fsck"
python3 tools/r11m-fatwalk.py "$OUT/work-vm.img" --strict --vfat > "$OUT/vm-walk.txt" 2>&1 && ok "i386VM: $(tail -1 "$OUT/vm-walk.txt")" || bad "i386VM fatwalk"
mcopy -n -i "$OUT/work-vm.img" ::/SRC/SUPERBASIC_INTERPRETER.C "$OUT/mt-long.bin"
mcopy -n -i "$VOL0" ::/LONG.TXT "$OUT/mt-src.bin"
cmp -s "$OUT/mt-long.bin" "$OUT/mt-src.bin" && ok "mtools: SRC/SUPERBASIC_INTERPRETER.C identical to LONG.TXT" || bad "mtools content"
grep -q " volumes/r11m-demo-initial.img$" SHA256SUMS && (grep " volumes/r11m-demo-initial.img$" SHA256SUMS | sha256sum -c --quiet) && ok "initial volume untouched after the runs (runs used copies)" || bad "initial volume"
if [ -n "${TINY386:-}" ] && [ -x "${TINY386:-/nonexistent}" ]; then
  mkdir -p "$OUT/pc"
  PC_CMD_TIMEOUT=300 sh scripts/r11m-run-pc.sh "$VOL0" tests/r11m/seq-s4a.txt "$OUT/pc" s4a build/r11m/pc/r11m.img >/dev/null
  PC_CMD_TIMEOUT=300 sh scripts/r11m-run-pc.sh "$OUT/pc/tiny386-s4a.final.img" tests/r11m/seq-s4b.txt "$OUT/pc" s4b build/r11m/pc/r11m.img >/dev/null
  grep -q "SUPERB~1.C .*SUPERBASIC_INTERPRETER.C" "$OUT/pc/tiny386-s4b.txt" && ok "Tiny386: after reboot the long name and its alias are listed" || bad "Tiny386 listing"
  [ "$(grep -c 'line 60\|the quick brown fox' "$OUT/pc/tiny386-s4b.txt")" -ge 2 ] && ok "Tiny386: file read by long name and by alias" || bad "Tiny386 read"
  fsck.fat -n "$OUT/pc/tiny386-s4b.final.img" >/dev/null 2>&1 && ok "Tiny386: fsck.fat clean" || bad "Tiny386 fsck"
else
  echo "NA   Tiny386 (TINY386 not set)"
fi
if [ "${SMOKE_QEMU:-0}" = 1 ] && command -v qemu-system-i386 >/dev/null; then
  mkdir -p "$OUT/qemu"
  R11M_QKERNEL=build/r11m/pc-vga/r11m-vga.img QEMU_CMD_TIMEOUT=300 sh scripts/r11m-run-qemu.sh "$VOL0" tests/r11m/seq-s4a.txt "$OUT/qemu" s4a 1800 >/dev/null
  R11M_QKERNEL=build/r11m/pc-vga/r11m-vga.img QEMU_CMD_TIMEOUT=300 sh scripts/r11m-run-qemu.sh "$OUT/qemu/qemu-s4a.final.img" tests/r11m/seq-s4b.txt "$OUT/qemu" s4b 1800 >/dev/null
  grep -q "SUPERB~1.C .*SUPERBASIC_INTERPRETER.C" "$OUT/qemu/qemu-s4b.txt" && ok "QEMU: after reboot the long name and its alias are listed" || bad "QEMU listing"
  fsck.fat -n "$OUT/qemu/qemu-s4b.final.img" >/dev/null 2>&1 && ok "QEMU: fsck.fat clean" || bad "QEMU fsck"
else
  echo "NA   QEMU (set SMOKE_QEMU=1)"
fi
[ $BAD = 0 ] && echo "SMOKE PASS" || { echo "SMOKE FAIL ($BAD)"; exit 1; }
