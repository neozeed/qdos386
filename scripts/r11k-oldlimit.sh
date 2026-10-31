#!/bin/sh
# R11K investigation: where does the ORIGINAL (R11J) boot sector stop?  Builds disks with the unchanged
# legacy boot sector and forced sector counts (tools/r11k-oldlimit-image.py bypasses make-image.py's
# checks), boots each on Tiny386 and QEMU, and records boot / hang.  Used for docs/R11K-BOOT-AUDIT.md §2.
#   usage: TINY386=.. FW=.. sh scripts/r11k-oldlimit.sh [outdir=logs/r11k/boot]
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd); cd "$ROOT"
OUT=${1:-logs/r11k/boot}; mkdir -p "$OUT" build/r11k/oldlimit
VOL=${VOLUME:-build/fat16/win1-r11j.img}
BOOT=build/pc-r9/boot.bin; XFER=build/xfer.bin
PLAIN=${PLAIN_KERNEL:-build/r11j/pc-poc/kernel.raw}; VGA=${VGA_KERNEL:-build/r11j/pc-poc-vga/kernel.raw}
: > "$OUT/oldlimit-summary.txt"
for T in 119 120 121 124 127 128 129 160; do
  python3 tools/r11k-oldlimit-image.py $BOOT $PLAIN $XFER $T build/r11k/oldlimit/plain-$T.img > /dev/null
  r=$(python3 scripts/r11k-pcboot.py "$TINY386" "$FW" build/r11k/oldlimit/plain-$T.img "$VOL" 25 2>/dev/null | head -1)
  echo "tiny386 $T $r" | tee -a "$OUT/oldlimit-summary.txt"
  if python3 tools/r11k-oldlimit-image.py $BOOT $VGA $XFER $T build/r11k/oldlimit/vga-$T.img > /dev/null 2>&1; then
    r=$(python3 scripts/r11k-qemuboot.py build/r11k/oldlimit/vga-$T.img "$VOL" 30 2>/dev/null | head -1)
    echo "qemu $T $r" | tee -a "$OUT/oldlimit-summary.txt"
  fi
done
