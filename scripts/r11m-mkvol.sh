#!/bin/sh
# R11M: make a scratch FAT16 volume for the usability tests.  It holds the R11K SDK programs (the volume
# the R11L volumes start from, extracted once into build/r11m/base) with SHELL.AOU replaced by the R11M
# shell, plus any NAME=path arguments.  Deterministic: fixed volume id and file times.
#   usage: sh scripts/r11m-mkvol.sh OUT.img [KIB [SPC [ROOT [NAME=path ...]]]]   (defaults 3072 1 512)
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd); cd "$ROOT"
OUT=${1:?usage}; KIB=${2:-3072}; SPC=${3:-1}; RT=${4:-512}
[ $# -ge 4 ] && shift 4 || shift $#
SH=${R11M_SHELL_AOU:-build/apps/shell-m.aout}
if [ ! -d build/r11m/base ]; then
  mkdir -p build/r11m
  python3 tools/r11j-extract-volume.py build/fat16/win1-r11k.img build/r11m/base
fi
ARGS=""
for f in build/r11m/base/*; do
  n=$(basename "$f")
  case "$n" in BIG.BIN|BAD*|CHAIN*|EMPTY*|SHELL.AOU) continue;; esac
  ARGS="$ARGS $n=$f"
done
find build/r11m/base -type f | xargs touch -d '2026-10-28 08:00:00'
touch -d '2026-10-28 08:00:00' "$SH"
sh scripts/r11l-mkvol.sh "$OUT" "$KIB" "$SPC" "$RT" R11M $ARGS SHELL.AOU="$SH" "$@"
