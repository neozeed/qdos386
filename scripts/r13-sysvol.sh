#!/bin/sh
# R13 example system volume: the R12B layout (scripts/r12b-install.sh) with the R13 shell (sessions) and the R12C/R13
# test and demo programs; fsck and the VFAT walker are run on the result.  usage: sh scripts/r13-sysvol.sh OUT.img [KIB]
# R13_BIN_EXTRA="NAME.AOU=path ..." adds programs to /BIN.
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd); cd "$ROOT"
export MTOOLS_SKIP_CHECK=1
OUT=${1:?usage}; KIB=${2:-4096}
E=sdk/examples
X=""
for n in MAPTEST BIGBSS HEAPT STKT FAULTT SPAWNT CHILDT PINFO ENVT NEGDYN; do [ -f "$E/$n.aout" ] && X="$X $n.AOU=$E/$n.aout"; done
R11M_SHELL_AOU=build/apps/shell-d.aout R12_EXTRA="$X ${R12_EXTRA:-}" sh scripts/r12-vol.sh "$OUT" "$KIB" >/dev/null
B=""
for n in TITLET SYNCT SURFT COLOUR BOUNCE; do [ -f "$E/$n.aout" ] && B="$B $n.AOU=$E/$n.aout"; done
R12B_STARTUP=${R13_STARTUP:-fs/r13-system/ETC/STARTUP.CMD} R12B_BIN_EXTRA="$B ${R13_BIN_EXTRA:-}" sh scripts/r12b-install.sh "$OUT" >/dev/null
fsck.fat -n "$OUT" >/dev/null
python3 tools/r11m-fatwalk.py "$OUT" --vfat --strict | tail -1
echo "system volume $OUT: fsck clean"
