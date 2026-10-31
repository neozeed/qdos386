#!/bin/sh
# R11I: build the i386VM harnesses.  Output in OUTDIR:
#   i386vm-r9        the frozen R9 harness (input-status + R9 16 MiB RAM patches); must hash a4724f79b26a716e
#   i386vm-r9-block  the same plus the R11I block device (patches/i386vm-block.patch and
#                    harness/i386vm-block/blockdev.{c,h}); a separate binary, never a replacement
# The historical emulator sources are not in the repository: point I386VM at the tree.
#   usage: I386VM=<historical tree> [STRICT=1] sh scripts/r11i-build-harness.sh <outdir>
# The binary hashes depend on the host C compiler.  a4724f79b26a716e is the reference host's
# value; with STRICT=1 a different hash is an error, otherwise a warning.
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd)
: "${I386VM:?set I386VM to the historical tree}"
OUT=${1:?usage: r11i-build-harness.sh OUTDIR}
mkdir -p "$OUT"
ABS=$(cd "$OUT" && pwd)
CFLAGS="-O2 -DI386_ENABLE_FPU -fwrapv -fno-strict-aliasing -DRAM_MB=16"
W=$(mktemp -d "$ABS/w.XXXX")
trap 'rm -rf "$W"' EXIT

# Frozen R9 harness.
cp -r "$I386VM/emulator-tiny386" "$W/"
( cd "$W" && patch -p1 -s < "$ROOT/patches/i386vm-input-status.patch" && patch -p1 -s < "$ROOT/patches/i386vm-r9-ram16.patch" \
  && cc $CFLAGS emulator-tiny386/i386.c emulator-tiny386/fpu.c emulator-tiny386/machine.c -lm -o "$ABS/i386vm-r9" 2>/dev/null )
GOT=$(sha256sum "$ABS/i386vm-r9" | cut -c1-16)
if [ "$GOT" != a4724f79b26a716e ]; then
  echo "note: i386vm-r9 hash $GOT differs from the reference host (a4724f79b26a716e)" >&2
  [ "${STRICT:-0}" = 1 ] && exit 3
fi
echo "i386vm-r9 sha256 $GOT"
rm -rf "$W"

# Block harness: the R9 harness with the block patch and the two device sources.
W=$(mktemp -d "$ABS/w.XXXX")
cp -r "$I386VM/emulator-tiny386" "$W/"
( cd "$W" && patch -p1 -s < "$ROOT/patches/i386vm-input-status.patch" && patch -p1 -s < "$ROOT/patches/i386vm-r9-ram16.patch" \
  && patch -p1 -s < "$ROOT/patches/i386vm-block.patch" \
  && cp "$ROOT/harness/i386vm-block/blockdev.c" "$ROOT/harness/i386vm-block/blockdev.h" emulator-tiny386/ \
  && cc $CFLAGS emulator-tiny386/i386.c emulator-tiny386/fpu.c emulator-tiny386/machine.c emulator-tiny386/blockdev.c -lm -o "$ABS/i386vm-r9-block" 2>/dev/null ) \
  || { echo "block harness build failed" >&2; exit 4; }
echo "i386vm-r9-block sha256 $(sha256sum "$ABS/i386vm-r9-block" | cut -c1-16)"
