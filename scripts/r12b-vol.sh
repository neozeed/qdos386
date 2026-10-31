#!/bin/sh
# R12B Stage 1 scratch volume: the R12A scratch volume with the R12B shell (SET/UNSET/ECHO/%NAME%, CALL, STARTUP.CMD)
# and the R12B test programs that exist.  usage: sh scripts/r12b-vol.sh OUT.img [KIB]   (R12_EXTRA as for r12-vol.sh)
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd); cd "$ROOT"
E=${QDOS_BIN_DIR:-sdk/examples}       # R12C: QDOS_BIN_DIR=build/frozen-r12b runs the R12B suites with the frozen R12B binaries
X=""
for n in MAPTEST BIGBSS HEAPT STKT FAULTT SPAWNT CHILDT PINFO ENVT; do [ -f "$E/$n.aout" ] && X="$X $n.AOU=$E/$n.aout"; done
R11M_SHELL_AOU=${R12B_SHELL_AOU:-build/apps/shell-b.aout} R12_EXTRA="$X ${R12_EXTRA:-}" sh scripts/r12-vol.sh "$@"
