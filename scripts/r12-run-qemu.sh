#!/bin/sh
# R12 (derived from r11m-run-qemu.sh): scripted QEMU run (VGA text-buffer capture through the monitor) on a WRITABLE scratch volume (copied).
#   usage: sh scripts/r11m-run-qemu.sh VOL.img SEQ OUTDIR TAG [TIMEOUT_S]
# Environment: R12_QKERNEL (default build/r12/pc-pg-vga/r12-pg-vga.img), QEMU_EXTRA, QEMU_MON_PORT, QEMU_CMD_TIMEOUT
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd); cd "$ROOT"
V=${1:?usage}; S=${2:?}; O=${3:?}; TAG=${4:?}; T=${5:-3000}
W=build/r12/qemu-$TAG${QEMU_MON_PORT:+-$QEMU_MON_PORT}; rm -rf "$W"; mkdir -p "$W/out" "$O"   # R13: the port keeps parallel lanes apart
cp "${R12_QKERNEL:-${R11M_QKERNEL:-build/r12/pc-pg-vga/r12-pg-vga.img}}" "$W/hda.img"; cp "$V" "$W/hdb.img"
timeout "$T" python3 scripts/r12-qemu-capture.py "$W/out" "$W" "$ROOT" --seq "$S" > "$O/qemu-$TAG.log" 2>&1 || true
cp "$W/out/lines.txt" "$O/qemu-$TAG.txt"; cp "$W/hdb.img" "$O/qemu-$TAG.final.img"
for f in "$W"/out/*.ppm; do if [ -f "$f" ]; then cp "$f" "$O/"; fi; done      # R12B: %screendump pictures
echo "qemu $TAG: transcript $O/qemu-$TAG.txt, image $O/qemu-$TAG.final.img"
