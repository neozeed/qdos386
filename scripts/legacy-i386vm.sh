#!/bin/sh
# R11M-FIX1: the CPU-only i386VM harness is a LEGACY DIAGNOSTIC target.  It is not part of any release gate or default run
# (docs/R11M-FIX1-TEST-POLICY.md).  This wrapper is the one separate command that still runs it: the R11M demonstration
# (phases A and B with reboot, judged) on the i386VM block harness.  Sources, patches/i386vm-*.patch, the harness and all historic logs are kept.
#   usage: sh scripts/legacy-i386vm.sh OUTDIR        (needs I386VM / QDOS386_TOOLS / HARNESS_DIR as scripts/r11m-demo.sh)
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd); cd "$ROOT"
echo "LEGACY DIAGNOSTIC: CPU-only i386VM harness (not a release gate)"
exec sh scripts/r11m-demo.sh vm "${1:?usage}"
