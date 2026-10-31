#!/bin/sh
# KCTX-FIX2: the PUBLIC-SAFE system volume for the current kernel: the R14A example volume (scripts/r14a-sysvol.sh:
# shell, SH, EDIT, QLTerm, the SDK, the test programs, /GAMES/HACK) WITHOUT the restricted material:
#   /BIN/BASIC.AOU and /BASTEST.AOU   SuperBASIC - private research, not freely redistributable (apps/superbasic/docs)
#   /INFOCOM.AOU                      the Task Force '87 interpreter - no licence granted (third_party/infocom)
# No story file is ever put on a volume by these scripts.  Kept, with their notices in the source tree: QLTerm
# (GPL-3.0-or-later, /BIN/QLTERM.AOU), the optional Minerva 1.98 font (GPL-2.0, /SYSTEM/FONTS/MINERVA.FNT + MINERVA.TXT),
# Hack 1.0.3 (BSD-style, /GAMES/HACK), phoon, the R11 fixtures.  The result is checked: none of the restricted names
# is left, fsck clean.
#   usage: sh scripts/qdos386-public-vol.sh OUT.img [KIB]
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd); cd "$ROOT"
export MTOOLS_SKIP_CHECK=1 LC_ALL=C.UTF-8
OUT=${1:?usage}; KIB=${2:-8192}
sh scripts/r14a-sysvol.sh "$OUT" "$KIB" > /dev/null
for f in ::/BIN/BASIC.AOU ::/BASTEST.AOU ::/INFOCOM.AOU; do mdel -i "$OUT" "$f" 2>/dev/null || true; done
L=$(mdir -/ -b -i "$OUT" ::)
if echo "$L" | grep -i -q -E '/BASIC\.AOU$|BASTEST|INFOCOM|ZORK|\.Z[3-8]$|STORY'; then
	echo "restricted file still on $OUT" >&2; exit 3
fi
fsck.fat -n "$OUT" > /dev/null
python3 tools/r11m-fatwalk.py "$OUT" --vfat --strict | tail -1
echo "public-safe system volume $OUT: no SuperBASIC, no Infocom interpreter, no story file; fsck clean"
