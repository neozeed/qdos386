#!/bin/sh
# R14 example system volume: the R13B volume plus QLTERM.AOU, TERM.AOU, XFER.AOU and SERTEST.AOU in /BIN.
#   usage: sh scripts/r14-sysvol.sh OUT.img [KIB]
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd); cd "$ROOT"
OUT=${1:?usage}; KIB=${2:-8192}
R13B_BIN_EXTRA="QLTERM.AOU=build/r14/qlterm/QLTERM.aout TERM.AOU=sdk/examples/TERM.aout XFER.AOU=sdk/examples/XFER.aout SERTEST.AOU=build/r14/sertest.aout ${R14_BIN_EXTRA:-}" \
  sh scripts/r13b-sysvol.sh "$OUT" "$KIB"
