#!/bin/sh
# R11M: scripts/r11l-run-pc.sh plus a "flush read".  Tiny386 writes the disk image through a stdio FILE without fflush, and the
# harness ends the emulator with SIGKILL, so the LAST sector write of a run can stay in the stdio buffer and never reach the image
# file (seen as "FAT copies differ in 1 entries": copy 1 written, copy 2 lost).  A later disk READ makes the emulator fseeko(),
# which flushes the pending write; so a harmless read of a data file (TYPE WIN1_ABCDEFGH.TXT) is inserted before the final EXIT
# (or appended) of every sequence.  The guest kernel itself is write-through; this is a property of the emulator run.
#   usage: TINY386=<tiny386_headless> sh scripts/r11m-run-pc.sh VOL.img SEQ OUTDIR TAG [KERNEL_IMG]
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd); cd "$ROOT"
V=${1:?usage}; S=${2:?}; O=${3:?}; TAG=${4:?}; K=${5:-build/r11m/pc/r11m.img}
mkdir -p "$O"
P=$O/seq-$TAG.pc.txt
last=$(tail -n 1 "$S" | tr -d '\r')
if [ "$last" = EXIT ]; then
  sed '$d' "$S" > "$P"; echo "TYPE WIN1_ABCDEFGH.TXT" >> "$P"; echo EXIT >> "$P"
else
  cp "$S" "$P"; echo "TYPE WIN1_ABCDEFGH.TXT" >> "$P"
fi
sh scripts/r11l-run-pc.sh "$V" "$P" "$O" "$TAG" "$K"
