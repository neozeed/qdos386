#!/bin/sh
# R12C example system volume: the R12B layout (scripts/r12b-install.sh) with the R12C shell (TASKS, SYNC) and the
# R12C test programs; fsck and the VFAT walker are run on the result.  usage: sh scripts/r12c-sysvol.sh OUT.img [KIB]
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd); cd "$ROOT"
export MTOOLS_SKIP_CHECK=1
OUT=${1:?usage}; KIB=${2:-4096}
E=sdk/examples
X=""
for n in MAPTEST BIGBSS HEAPT STKT FAULTT SPAWNT CHILDT PINFO ENVT NEGDYN; do [ -f "$E/$n.aout" ] && X="$X $n.AOU=$E/$n.aout"; done
R11M_SHELL_AOU=build/apps/shell-c.aout R12_EXTRA="$X ${R12_EXTRA:-}" sh scripts/r12-vol.sh "$OUT" "$KIB" >/dev/null
R12B_STARTUP=${R12C_STARTUP:-fs/r12c-system/ETC/STARTUP.CMD} R12B_BIN_EXTRA="TITLET.AOU=$E/TITLET.aout SYNCT.AOU=$E/SYNCT.aout ${R12C_BIN_EXTRA:-}" sh scripts/r12b-install.sh "$OUT" >/dev/null
fsck.fat -n "$OUT" >/dev/null
python3 tools/r11m-fatwalk.py "$OUT" --vfat --strict | tail -1
echo "system volume $OUT: fsck clean"
