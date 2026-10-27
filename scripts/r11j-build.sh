#!/bin/sh
# R11J: build the paging proof programs with the unchanged SDK (qdos386-gcc, slot 1, -O) and the
# R11J test volume: the R11H release set plus SAME.AOU and SAMELNCH.AOU.  The R11H volume script
# is reused; its own output (win1-r11h.img) is rebuilt to a scratch path first and must match the
# frozen release volume byte for byte, so the R11J volume differs only by the two added files.
#   usage: QDOS386_TOOLS=<historical tree> sh scripts/r11j-build.sh
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd)
cd "$ROOT"
: "${QDOS386_TOOLS:?set QDOS386_TOOLS to the GCC 1.40 / GAS / i386-ld tree}"
export QDOS386_TOOLS
W=sdk/bin/qdos386-gcc
# The SDK libraries are build products (not in git): build them first on a fresh tree.
for d in sdk/lib sdk/libc sdk/libm; do make -C "$d" QDOS386_TOOLS="$QDOS386_TOOLS" >/dev/null; done
mkdir -p build/r11j/apps build/r11j/vol
"$W" -slot 1 -O -o build/r11j/apps/SAME.AOU sdk/examples/r11j/same.c
"$W" -slot 1 -O -o build/r11j/apps/SAMELNCH.AOU sdk/examples/r11j/samelnch.c
sha256sum build/r11j/apps/SAME.AOU build/r11j/apps/SAMELNCH.AOU
# The R11H release volume's own files (exact bytes, in directory order): the R11F application
# rebuilds differ by a few bytes when their original libraries are unavailable (documented in
# R11H-FIX1), so the volume is built from the frozen files, not from a rebuild.
FROZEN=${R11H_RELEASE_VOLUME:-build/fat16/win1-r11h.img}
GOT=$(sha256sum "$FROZEN" | cut -c1-64)
[ "$GOT" = 672806ad2c2ca337c45368dc6a16b71f2a8e61886a011bfbbf49d4cba5f1477b ] || { echo "not the R11H release volume: $GOT" >&2; exit 7; }
rm -rf build/r11j/vol/files
ARGS=$(python3 tools/r11j-extract-volume.py "$FROZEN" build/r11j/vol/files)
QDOS_LAYOUT=r9 python3 tools/make-fat16.py build/r11j/vol/check.img $ARGS >/dev/null
GOT=$(sha256sum build/r11j/vol/check.img | cut -c1-64)
[ "$GOT" = 672806ad2c2ca337c45368dc6a16b71f2a8e61886a011bfbbf49d4cba5f1477b ] || { echo "R11H volume not reproduced from its files: $GOT" >&2; exit 7; }
echo "R11H release volume reproduced from its own files: $GOT"
rm -f build/r11j/vol/check.img
QDOS_LAYOUT=r9 python3 tools/make-fat16.py build/fat16/win1-r11j.img $ARGS \
  SAME.AOU=build/r11j/apps/SAME.AOU SAMELNCH.AOU=build/r11j/apps/SAMELNCH.AOU >/dev/null
sha256sum build/fat16/win1-r11j.img
