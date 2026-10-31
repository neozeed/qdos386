#!/bin/sh
# R11G scripted i386VM run (SCRIPTED: the command lines are piped to the harness on stdin).
# The i386VM image embeds build/fat16/win1-r11g.img (build it with the I386WIN override shown in
# docs/R11G-RUNTIME-REPORT.md).  The sequence is the same as the Tiny386 run.
#   usage: I386VM_HARNESS=<i386vm-r9 binary> sh scripts/r11g-run-i386vm.sh <outdir> [image.bin]
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd); cd "$ROOT"
: "${I386VM_HARNESS:?set I386VM_HARNESS to the i386vm-r9 binary}"
OUT=${1:?usage: r11g-run-i386vm.sh OUTDIR}; IMG=${2:-build/i386vm-r11g-none/qdos386-r11g-none.bin}
mkdir -p "$OUT"
IN=$OUT/i386vm-game-input.txt
{ echo "RUN INFOCOM.AOU"; cat tests/r11g/seq-zork-game.txt; echo "DIR"; echo "RUN INFOCOM.AOU"; \
  echo "look"; echo "quit"; echo "y"; echo "DIR"; } > "$IN"
timeout 1500 "$I386VM_HARNESS" "$IMG" < "$IN" > "$OUT/i386vm-game.txt" 2>&1 || true
echo "done: $OUT/i386vm-game.txt"
