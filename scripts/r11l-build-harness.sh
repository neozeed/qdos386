#!/bin/sh
# R11L: build the writable-block i386VM harness (i386vm-r9-blockrw).  The R11I read-only harnesses
# (i386vm-r9, i386vm-r9-block) are NOT touched; scripts/r11i-build-harness.sh still builds them.
#   i386vm-r9-blockrw = R9 harness + patches/i386vm-block.patch + patches/i386vm-block-rw.patch,
#                       harness/i386vm-block/blockdev.{c,h} compiled with -DBLK_RW.
# Usage: i386vm-r9-blockrw image.bin disk.img rw      (without "rw" the disk is opened read-only)
# Environment: QDOS_BLK_FAULT=io:N   after N sector writes every write fails (BLK_WRIOERR)
#              QDOS_BLK_FAULT=exit:N the harness exits 77 when write N+1 arrives (power cut)
#   usage: I386VM=<historical tree> sh scripts/r11l-build-harness.sh <outdir>
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd)
: "${I386VM:?set I386VM to the historical tree}"
OUT=${1:?usage: r11l-build-harness.sh OUTDIR}
mkdir -p "$OUT"
ABS=$(cd "$OUT" && pwd)
CFLAGS="-O2 -DI386_ENABLE_FPU -fwrapv -fno-strict-aliasing -DRAM_MB=16"
W=$(mktemp -d "$ABS/w.XXXX")
trap 'rm -rf "$W"' EXIT
cp -r "$I386VM/emulator-tiny386" "$W/"
( cd "$W" && patch -p1 -s < "$ROOT/patches/i386vm-input-status.patch" && patch -p1 -s < "$ROOT/patches/i386vm-r9-ram16.patch" \
  && patch -p1 -s < "$ROOT/patches/i386vm-block.patch" && patch -p1 -s < "$ROOT/patches/i386vm-block-rw.patch" \
  && cp "$ROOT/harness/i386vm-block/blockdev.c" "$ROOT/harness/i386vm-block/blockdev.h" emulator-tiny386/ \
  && cc $CFLAGS -DBLK_RW emulator-tiny386/i386.c emulator-tiny386/fpu.c emulator-tiny386/machine.c emulator-tiny386/blockdev.c -lm -o "$ABS/i386vm-r9-blockrw" 2>/dev/null ) \
  || { echo "blockrw harness build failed" >&2; exit 4; }
echo "i386vm-r9-blockrw sha256 $(sha256sum "$ABS/i386vm-r9-blockrw" | cut -c1-64)"
