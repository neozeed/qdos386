#!/bin/sh
# R11K: build the SDK libraries, the R11K programs and the R11K test volume (win1-r11k.img):
# the R11H release set (exact frozen bytes) + the R11J proof programs + the R11K programs + BIG.BIN.
#   usage: QDOS386_TOOLS=<historical tree> sh scripts/r11k-build.sh
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd); cd "$ROOT"
: "${QDOS386_TOOLS:?set QDOS386_TOOLS to the GCC 1.40 / GAS / i386-ld tree}"
export QDOS386_TOOLS
W=sdk/bin/qdos386-gcc
for d in sdk/lib sdk/libc sdk/libm; do make -C "$d" QDOS386_TOOLS="$QDOS386_TOOLS" >/dev/null; done
mkdir -p build/r11k/apps build/r11k/vol
for a in twice pgrun twicet; do
  [ -f sdk/examples/r11k/$a.c ] || continue
  "$W" -slot 1 -O -o build/r11k/apps/$(echo $a | tr a-z A-Z).AOU sdk/examples/r11k/$a.c >/dev/null
done
# SAME/SAMELNCH are the R11J programs, rebuilt with the R11K libraries: must equal the R11J bytes.
for a in same samelnch; do
  N=$(echo $a | tr a-z A-Z).AOU
  "$W" -slot 1 -O -o build/r11k/apps/$N sdk/examples/r11j/$a.c >/dev/null
  cmp build/r11k/apps/$N build/r11j/apps/$N
done
python3 tools/r11k-make-bigbin.py build/r11k/vol/BIG.BIN > build/r11k/vol/BIG.SUM
FROZEN=${R11H_RELEASE_VOLUME:-build/fat16/win1-r11h.img}
GOT=$(sha256sum "$FROZEN" | cut -c1-64)
[ "$GOT" = 672806ad2c2ca337c45368dc6a16b71f2a8e61886a011bfbbf49d4cba5f1477b ] || { echo "not the R11H release volume: $GOT" >&2; exit 7; }
rm -rf build/r11k/vol/files
ARGS=$(python3 tools/r11j-extract-volume.py "$FROZEN" build/r11k/vol/files)
EXTRA=
for f in build/r11k/apps/*.AOU; do EXTRA="$EXTRA $(basename $f)=$f"; done
QDOS_LAYOUT=r9 python3 tools/make-fat16.py build/fat16/win1-r11k.img $ARGS $EXTRA BIG.BIN=build/r11k/vol/BIG.BIN >/dev/null
python3 tools/r11k-make-bigbin.py --sum-of build/r11k/vol/files/README.TXT > build/r11k/vol/README.SUM
sha256sum build/r11k/apps/*.AOU build/fat16/win1-r11k.img
echo "BIG.BIN $(cat build/r11k/vol/BIG.SUM)  README.TXT $(cat build/r11k/vol/README.SUM)"
