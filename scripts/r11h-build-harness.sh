#!/bin/sh
# R11H-FIX1: build the i386VM harnesses from the historical emulator sources and the patches in
# patches/.  Output: $OUT/i386vm-input-only (checked against the recorded hash) and $OUT/i386vm-r9
# (input-status patch plus the R9 16 MiB RAM patch, built with -DRAM_MB=16).
#   usage: I386VM=<historical tree> sh scripts/r11h-build-harness.sh <outdir>
# The synthetic-timer patch does not apply on top of the other two (hunks fail); the R9 harness is
# built without it and its MULTI transcript matches the shipped R11H one (docs/R11H-FIX1-MEMORY-MAP.md).
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd)
: "${I386VM:?set I386VM to the historical tree}"
OUT=${1:?usage: r11h-build-harness.sh OUTDIR}
mkdir -p "$OUT"
CFLAGS="-O2 -DI386_ENABLE_FPU -fwrapv -fno-strict-aliasing"
build() {  # build NAME [extra cflags] with the patches listed after
  name=$1; shift
  W=$(mktemp -d "$OUT/w.XXXX")
  cp -r "$I386VM/emulator-tiny386" "$W/"
  cd "$W"
  for p in "$@"; do patch -p1 -s < "$ROOT/patches/$p.patch" || exit 4; done
  cd "$ROOT"
  ABS=$(cd "$OUT" && pwd)
  ( cd "$W" && cc $CFLAGS $EXTRA emulator-tiny386/i386.c emulator-tiny386/fpu.c emulator-tiny386/machine.c -lm -o "$ABS/$name" )
  rm -rf "$W"
  echo "$name sha256 $(sha256sum "$OUT/$name" | cut -c1-16)"
}
EXTRA= build i386vm-input-only i386vm-input-status
GOT=$(sha256sum "$OUT/i386vm-input-only" | cut -c1-16)
[ "$GOT" = 2d2f8beba177592e ] || { echo "input-only harness hash mismatch: $GOT" >&2; exit 3; }
EXTRA="-DRAM_MB=16" build i386vm-r9 i386vm-input-status i386vm-r9-ram16
