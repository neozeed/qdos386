#!/bin/sh
# R12A: the R12 scratch volume with the R12A shell (16-entry job table) and the R12A test programs that exist.
#   usage: sh scripts/r12a-vol.sh OUT.img [KIB]      (R12_EXTRA adds NAME.AOU=path entries, as for r12-vol.sh)
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd); cd "$ROOT"
E=sdk/examples
X=""
for n in MAPTEST BIGBSS HEAPT STKT FAULTT SPAWNT CHILDT PINFO; do [ -f "$E/$n.aout" ] && X="$X $n.AOU=$E/$n.aout"; done
R11M_SHELL_AOU=build/apps/shell-a.aout R12_EXTRA="$X ${R12_EXTRA:-}" sh scripts/r12-vol.sh "$@"
