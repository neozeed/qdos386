#!/bin/sh
# R12B example system volume: the R12B scratch volume (scripts/r12b-vol.sh) with the system layout installed
# (scripts/r12b-install.sh), checked with fsck.fat and the VFAT walker.  usage: sh scripts/r12b-sysvol.sh OUT.img [KIB]
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd); cd "$ROOT"
export MTOOLS_SKIP_CHECK=1
OUT=${1:?usage}; KIB=${2:-4096}
sh scripts/r12b-vol.sh "$OUT" "$KIB" >/dev/null
sh scripts/r12b-install.sh "$OUT" >/dev/null
fsck.fat -n "$OUT" >/dev/null
python3 tools/r11m-fatwalk.py "$OUT" --vfat --strict | tail -1
echo "system volume $OUT: fsck clean"
