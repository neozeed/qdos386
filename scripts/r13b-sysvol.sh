#!/bin/sh
# R13B example system volume: the R13A volume with the R13B boot shell (shell-f as /SHELL.AOU), the dynamic shell
# /BIN/SH.AOU (the same source linked at 0x240000) and fs/r13b-system/ETC/STARTUP.CMD.
#   usage: sh scripts/r13b-sysvol.sh OUT.img [KIB]     (R13A_NO_MINERVA=1 leaves MINERVA.FNT out, as for R13A)
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd); cd "$ROOT"
export MTOOLS_SKIP_CHECK=1 LC_ALL=C.UTF-8
OUT=${1:?usage}; KIB=${2:-4096}
R13A_STARTUP=${R13B_STARTUP:-fs/r13b-system/ETC/STARTUP.CMD} R13A_BIN_EXTRA="SH.AOU=build/apps/sh.aout ${R13B_BIN_EXTRA:-}" \
  sh scripts/r13a-sysvol.sh "$OUT" "$KIB" >/dev/null
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
put() { cp "$1" "$T/f"; touch -d '2026-10-29 08:00:00' "$T/f"; mcopy -o -m -i "$OUT" "$T/f" "$2"; }
put build/apps/shell-f.aout ::/SHELL.AOU
fsck.fat -n "$OUT" >/dev/null
python3 tools/r11m-fatwalk.py "$OUT" --vfat --strict | tail -1
echo "R13B system volume $OUT: fsck clean"
