#!/bin/sh
# R11L Stage 1: build the scratch volume used by the WRTTEST runs.
#   usage: sh scripts/r11l-s1vol.sh OUT.img [SIZE_KIB [SPC [ROOT]]]   (default 3072 KiB, 1, 512)
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd); cd "$ROOT"
OUT=${1:?usage}; KIB=${2:-3072}; SPC=${3:-1}; RT=${4:-512}
ARGS=$(sh scripts/r11l-base-list.sh | grep -v 'BIG.BIN\|^BAD\|^CHAIN\|^EMPTY' | tr '\n' ' ')
find build/r11l/base -type f | xargs touch -d '2026-10-28 08:00:00'
sh scripts/r11l-mkvol.sh "$OUT" "$KIB" "$SPC" "$RT" R11LS1 $ARGS WRTTEST.AOU=build/r11l/stage/WRTTEST.AOU
