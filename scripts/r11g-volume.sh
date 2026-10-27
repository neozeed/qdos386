#!/bin/sh
# R11G FAT16 volume: the R11F volume contents (build/fat16/win1-r11f.img, from scripts/r11f-volume.sh)
# plus INFOCOM.AOU (the Task Force '87 interpreter, port build) and ZORK1_Z3 (the story file).
#   usage: ZORK_STORY=<path to zork1_z3> QDOS386_TOOLS=<historical tree> sh scripts/r11g-volume.sh
# The story is refused unless its SHA-256 matches the supplied, verified file.  The story is
# NOT copied into the repository or the package (licence not established for this asset).
# The story name is ZORK1_Z3 (eight characters, no extension); the interpreter opens WIN1_ZORK1_Z3.
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd)
cd "$ROOT"
: "${QDOS386_TOOLS:?set QDOS386_TOOLS to the GCC 1.40 / GAS / i386-ld tree}"
: "${ZORK_STORY:?set ZORK_STORY to the zork1_z3 file}"
STORY_SHA=37084966477dff679282de42974b2077156b1bd68fad92a65d4ea94d8eb64d79
GOT=$(sha256sum "$ZORK_STORY" | cut -c1-64)
[ "$GOT" = "$STORY_SHA" ] || { echo "story hash mismatch: $GOT" >&2; exit 5; }
[ -f build/fat16/win1-r11f.img ] || { echo "run scripts/r11f-volume.sh first" >&2; exit 6; }
sh scripts/r11g-build.sh "$ROOT/sdk" "$QDOS386_TOOLS" "$ROOT/build/r11g" >/dev/null
mkdir -p build/fat16
# The R11F argument list, taken from scripts/r11f-volume.sh (the same files, same names).
ARGS=$(sed -n '/^ARGS="\\$/,/PHDATE.AOU=build\/r11f\/PHDATE.aout/p' scripts/r11f-volume.sh \
  | sed -e '1s/^ARGS="\\$//' -e 's/ \$LG"$//' -e 's/\\$//')
QDOS_LAYOUT=r9 python3 tools/make-fat16.py build/fat16/win1-r11g.img $ARGS \
  INFOCOM.AOU=build/r11g/infocom.aout ZORK1_Z3="$ZORK_STORY"
sha256sum build/fat16/win1-r11g.img
