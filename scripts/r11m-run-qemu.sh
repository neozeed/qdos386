#!/bin/sh
# R11M: scripted QEMU run (VGA text-buffer capture through the monitor) on a WRITABLE scratch volume (copied).
#   usage: sh scripts/r11m-run-qemu.sh VOL.img SEQ OUTDIR TAG [TIMEOUT_S]
# Environment: R11M_QKERNEL (default build/r11l/pc-fs-vga/r11l-fs-vga.img), QEMU_EXTRA, QEMU_MON_PORT, QEMU_CMD_TIMEOUT
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd); cd "$ROOT"
V=${1:?usage}; S=${2:?}; O=${3:?}; TAG=${4:?}; T=${5:-3000}
W=build/r11m/qemu-$TAG; rm -rf "$W"; mkdir -p "$W/out" "$O"
cp "${R11M_QKERNEL:-build/r11l/pc-fs-vga/r11l-fs-vga.img}" "$W/hda.img"; cp "$V" "$W/hdb.img"
timeout "$T" python3 scripts/r11m-qemu-capture.py "$W/out" "$W" "$ROOT" --seq "$S" > "$O/qemu-$TAG.log" 2>&1 || true
cp "$W/out/lines.txt" "$O/qemu-$TAG.txt"; cp "$W/hdb.img" "$O/qemu-$TAG.final.img"
echo "qemu $TAG: transcript $O/qemu-$TAG.txt, image $O/qemu-$TAG.final.img"
