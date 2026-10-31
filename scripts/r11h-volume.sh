#!/bin/sh
# R11H test volume: the R11F application set, the R11G interpreter, and the R11H
# concurrency programs (MULTI, MULTIA, MULTIB, COLLIDE).  The story is optional:
# with ZORK_STORY set (and checked against the R11G hash) it is added as ZORK1_Z3;
# without it the volume is story-free, as in the R11G release.
#   usage: QDOS386_TOOLS=<historical tree> [ZORK_STORY=<zork1_z3>] [R11H_VOLUME_OUT=<img>] sh scripts/r11h-volume.sh [extra NAME.AOU=path ...]
#   The default output is build/fat16/win1-r11h.img, the release volume (story-free).
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd)
cd "$ROOT"
: "${QDOS386_TOOLS:?set QDOS386_TOOLS to the GCC 1.40 / GAS / i386-ld tree}"
[ -f build/fat16/win1-r11f.img ] || { echo "run scripts/r11f-volume.sh first" >&2; exit 6; }
[ -f build/r11g/infocom.aout ] || { echo "run scripts/r11g-build.sh first" >&2; exit 6; }
for f in MULTI MULTIA MULTIB COLLIDE NEG; do
	[ -f "build/r11h/$f.AOU" ] || { echo "missing build/r11h/$f.AOU (build the R11H programs first)" >&2; exit 6; }
done
ARGS=$(sed -n '/^ARGS="\\$/,/PHDATE.AOU=build\/r11f\/PHDATE.aout/p' scripts/r11f-volume.sh \
  | sed -e '1s/^ARGS="\\$//' -e 's/ \$LG"$//' -e 's/\\$//')
EXTRA="INFOCOM.AOU=build/r11g/infocom.aout MULTI.AOU=build/r11h/MULTI.AOU MULTIA.AOU=build/r11h/MULTIA.AOU MULTIB.AOU=build/r11h/MULTIB.AOU COLLIDE.AOU=build/r11h/COLLIDE.AOU NEG.AOU=build/r11h/NEG.AOU"
if [ -n "${ZORK_STORY:-}" ]; then
	GOT=$(sha256sum "$ZORK_STORY" | cut -c1-64)
	[ "$GOT" = 37084966477dff679282de42974b2077156b1bd68fad92a65d4ea94d8eb64d79 ] || { echo "story hash mismatch: $GOT" >&2; exit 5; }
	EXTRA="$EXTRA ZORK1_Z3=$ZORK_STORY"
fi
OUT=${R11H_VOLUME_OUT:-build/fat16/win1-r11h.img}
mkdir -p build/fat16
QDOS_LAYOUT=r9 python3 tools/make-fat16.py "$OUT" $ARGS $EXTRA "$@"
sha256sum "$OUT"
