#!/bin/sh
# R11L: scripted Tiny386 run on a WRITABLE scratch volume.  The volume is copied; the final image is left next to the
# transcript.  PC_CMD_TIMEOUT (seconds per command, default 900).  Sequence lines as scripts/r11h-run-tiny386.sh.
#   usage: TINY386=<tiny386_headless> sh scripts/r11l-run-pc.sh VOL.img SEQ OUTDIR TAG [KERNEL_IMG]
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd); cd "$ROOT"
V=${1:?usage}; S=${2:?}; O=${3:?}; TAG=${4:?}; K=${5:-build/r11l/pc-fs/r11l-fs.img}
mkdir -p "$O"
SHELLIMG=$K PC_KEY_DELAY=${PC_KEY_DELAY:-0.02} PC_CMD_TIMEOUT=${PC_CMD_TIMEOUT:-900} sh scripts/r11h-run-tiny386.sh "$O" "$V" "$S" "$TAG" > /dev/null
cp build/pc-r11h-$TAG-run/win1-r11h.img "$O/tiny386-$TAG.final.img"
echo "tiny386 $TAG: transcript $O/tiny386-$TAG.txt, image $O/tiny386-$TAG.final.img"
