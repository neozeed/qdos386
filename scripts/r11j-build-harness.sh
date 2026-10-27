#!/bin/sh
# R11J: build the i386VM harnesses.  Runs the R11I script (i386vm-r9, i386vm-r9-block), then adds
#   i386vm-r9-block-fd  the block harness plus patches/i386vm-faultdump.patch: with
#                       I386VM_FAULTDUMP set, the harness prints CR0/CR2/CR3/EIP at exit.  The CPU
#                       core ends the run on any guest exception before IDT delivery, so this is
#                       the only page-fault report available on i386VM.  machine.c only.
#   usage: I386VM=<historical tree> [STRICT=1] sh scripts/r11j-build-harness.sh <outdir>
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd)
: "${I386VM:?set I386VM to the historical tree}"
OUT=${1:?usage: r11j-build-harness.sh OUTDIR}
sh "$ROOT/scripts/r11i-build-harness.sh" "$OUT"
ABS=$(cd "$OUT" && pwd)
W=$(mktemp -d "$ABS/w.XXXX")
trap 'rm -rf "$W"' EXIT
cp -r "$I386VM/emulator-tiny386" "$W/"
( cd "$W" && patch -p1 -s < "$ROOT/patches/i386vm-input-status.patch" && patch -p1 -s < "$ROOT/patches/i386vm-r9-ram16.patch" \
  && patch -p1 -s < "$ROOT/patches/i386vm-block.patch" && patch -p1 -s < "$ROOT/patches/i386vm-faultdump.patch" \
  && cp "$ROOT/harness/i386vm-block/blockdev.c" "$ROOT/harness/i386vm-block/blockdev.h" emulator-tiny386/ \
  && cc -O2 -DI386_ENABLE_FPU -fwrapv -fno-strict-aliasing -DRAM_MB=16 emulator-tiny386/i386.c emulator-tiny386/fpu.c \
       emulator-tiny386/machine.c emulator-tiny386/blockdev.c -lm -o "$ABS/i386vm-r9-block-fd" 2>/dev/null ) \
  || { echo "faultdump harness build failed" >&2; exit 4; }
echo "i386vm-r9-block-fd sha256 $(sha256sum "$ABS/i386vm-r9-block-fd" | cut -c1-16)"
