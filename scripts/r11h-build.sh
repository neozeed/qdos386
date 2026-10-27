#!/bin/sh
# R11H: build the example programs into build/r11h/ with the SDK wrapper.
# Slot 1 is the launcher's own profile; MULTIA and COLLIDE use profile 2 (0x380000), MULTIB
# profile 3 (0x500000).  NEG is the guest negative-test program (slot 1).
#   usage: QDOS386_TOOLS=<historical tree> sh scripts/r11h-build.sh
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd)
cd "$ROOT"
: "${QDOS386_TOOLS:?set QDOS386_TOOLS to the GCC 1.40 / GAS / i386-ld tree}"
export QDOS386_TOOLS
W=sdk/bin/qdos386-gcc
E=sdk/examples/r11h
mkdir -p build/r11h
"$W" -slot 1 -o build/r11h/MULTI.AOU "$E/multi.c"
"$W" -slot 2 -o build/r11h/MULTIA.AOU "$E/multia.c"
"$W" -slot 3 -o build/r11h/MULTIB.AOU "$E/multib.c"
"$W" -slot 2 -o build/r11h/COLLIDE.AOU "$E/collide.c"
"$W" -slot 1 -o build/r11h/NEG.AOU "$E/neg.c"
sha256sum build/r11h/*.AOU
