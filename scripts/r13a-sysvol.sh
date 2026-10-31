#!/bin/sh
# R13A example system volume: the R13 volume with the R13A shell (shell-e), /SYSTEM/FONTS (RECOVERY.FNT, CON8X16.FNT
# and the optional GPL MINERVA.FNT with its notice), the R13A programs, and fs/r13a-system/ETC/STARTUP.CMD.
#   usage: sh scripts/r13a-sysvol.sh OUT.img [KIB]      R13A_NO_MINERVA=1 leaves MINERVA.FNT out
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd); cd "$ROOT"
export MTOOLS_SKIP_CHECK=1 LC_ALL=C.UTF-8
OUT=${1:?usage}; KIB=${2:-4096}
E=sdk/examples
X=""
for n in GFXA; do [ -f "$E/$n.aout" ] && X="$X $n.AOU=$E/$n.aout"; done
R13_STARTUP=${R13A_STARTUP:-fs/r13a-system/ETC/STARTUP.CMD} R13_BIN_EXTRA="$X ${R13A_BIN_EXTRA:-}" \
  sh scripts/r13-sysvol.sh "$OUT" "$KIB" >/dev/null
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
put() { cp "$1" "$T/f"; touch -d '2026-10-29 08:00:00' "$T/f"; mcopy -o -m -i "$OUT" "$T/f" "$2"; }
put build/apps/shell-e.aout ::/SHELL.AOU
mmd -i "$OUT" ::/SYSTEM 2>/dev/null || true
mmd -i "$OUT" ::/SYSTEM/FONTS 2>/dev/null || true
put fonts/RECOVERY.FNT ::/SYSTEM/FONTS/RECOVERY.FNT
put fonts/CON8X16.FNT ::/SYSTEM/FONTS/CON8X16.FNT
put fs/r13a-system/SYSTEM/FONTS/README.TXT ::/SYSTEM/FONTS/README.TXT
if [ -z "${R13A_NO_MINERVA:-}" ]; then
  put third_party/minerva/MINERVA.FNT ::/SYSTEM/FONTS/MINERVA.FNT
  put fs/r13a-system/SYSTEM/FONTS/MINERVA.TXT ::/SYSTEM/FONTS/MINERVA.TXT
fi
for f in ${R13A_FONTS_EXTRA:-}; do put "${f#*=}" "::/SYSTEM/FONTS/${f%%=*}"; done
fsck.fat -n "$OUT" >/dev/null
python3 tools/r11m-fatwalk.py "$OUT" --vfat --strict | tail -1
echo "R13A system volume $OUT: fsck clean"
