#!/bin/sh
# R12B: open the QDOS/386 R12B system in an interactive QEMU window (VGA, PS/2 keyboard, two IDE disks).
#   usage: sh scripts/r12b-launch-qemu.sh [VOLUME.img] [KERNEL.img]
# The volume is COPIED first (default volumes/r12b-system.img, kernel build/r12b/pc-vga/r12b-vga.img); the copy is left as
# build/r12b/launch-volume.img.  /ETC/STARTUP.CMD runs at boot (ESC within 2 s skips it).  Alt+F1..F4 switch consoles.
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd); cd "$ROOT"
VOL=${1:-volumes/r12b-system.img}; KRN=${2:-build/r12b/pc-vga/r12b-vga.img}
mkdir -p build/r12b; cp "$VOL" build/r12b/launch-volume.img
BIOS=${QEMU_BIOS:-/usr/share/seabios/bios.bin}
exec qemu-system-i386 -machine pc -cpu pentium -m 16 -bios "$BIOS" -vga std \
  -drive file="$KRN",format=raw,if=ide,index=0,media=disk \
  -drive file=build/r12b/launch-volume.img,format=raw,if=ide,index=1,media=disk -boot order=c
