#!/bin/sh
# R11I scripted i386VM run with the host-backed block device (SCRIPTED: the sequence is piped to
# the harness on stdin; "!" and "@" prefixes are for the PC harness and are removed).
#   usage: I386VM_BLOCK_HARNESS=<i386vm-r9-block> sh scripts/r11i-run-i386vm-block.sh \
#            <outdir> <kernel-only image.bin> <disk.img> <sequence> <tag>
# The disk is the host file; the harness opens it read-only.  The image is the kernel only.
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd); cd "$ROOT"
: "${I386VM_BLOCK_HARNESS:?set I386VM_BLOCK_HARNESS to the i386vm-r9-block binary}"
OUT=${1:?usage}; IMG=${2:?usage}; DISK=${3:?usage}; SEQ=${4:?usage}; TAG=${5:?usage}
mkdir -p "$OUT"; IN=$OUT/i386vm-$TAG-input.txt
sed -e 's/^[!@]//' "$SEQ" > "$IN"
timeout 1500 "$I386VM_BLOCK_HARNESS" "$IMG" "$DISK" < "$IN" > "$OUT/i386vm-$TAG.txt" 2>&1 || true
echo "done: $OUT/i386vm-$TAG.txt"
