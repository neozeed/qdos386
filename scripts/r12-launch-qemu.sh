#!/bin/sh
# R12: open the QDOS/386 R12 system in an interactive QEMU window (VGA, PS/2 keyboard, two IDE disks).
#   usage: sh scripts/r12-launch-qemu.sh [VOLUME.img] [KERNEL.img]
# The volume is COPIED to a scratch file first (default volumes/r12-s4-initial.img, build/r12/pc-pg-vga/r12-pg-vga.img): the session
# can write to it; the copy is left as build/r12/launch-volume.img.  Alt+F1..F4 switch consoles.  Type EXIT (twice if jobs run) to end.
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd); cd "$ROOT"
VOL=${1:-volumes/r12-s4-initial.img}; KRN=${2:-build/r12/pc-pg-vga/r12-pg-vga.img}
mkdir -p build/r12; cp "$VOL" build/r12/launch-volume.img
BIOS=${QEMU_BIOS:-/usr/share/seabios/bios.bin}
exec qemu-system-i386 -machine pc -cpu pentium -m 16 -bios "$BIOS" -vga std \
  -drive file="$KRN",format=raw,if=ide,index=0,media=disk \
  -drive file=build/r12/launch-volume.img,format=raw,if=ide,index=1,media=disk -boot order=c
