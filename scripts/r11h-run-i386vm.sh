#!/bin/sh
# R11H scripted i386VM run (SCRIPTED: the sequence is piped to the i386vm-r9 harness on stdin).
# The "!" and "@" prefixes are for the PC harness only and are removed here.
#   usage: I386VM_HARNESS=<i386vm-r9 binary> sh scripts/r11h-run-i386vm.sh <outdir> <image.bin> <sequence> <tag>
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd); cd "$ROOT"
: "${I386VM_HARNESS:?set I386VM_HARNESS to the i386vm-r9 binary}"
OUT=${1:?usage}; IMG=${2:?usage}; SEQ=${3:?usage}; TAG=${4:?usage}
mkdir -p "$OUT"; IN=$OUT/i386vm-$TAG-input.txt
sed -e 's/^[!@]//' "$SEQ" > "$IN"
timeout 1500 "$I386VM_HARNESS" "$IMG" < "$IN" > "$OUT/i386vm-$TAG.txt" 2>&1 || true
echo "done: $OUT/i386vm-$TAG.txt"
