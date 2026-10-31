#!/bin/sh
# R14: boot QEMU (with its normal display window, or -display none if R14_HEADLESS=1) on a COPY of a QDOS/386 volume
# with COM1 exported to the host, for interactive use with TERM.AOU, XFER.AOU or QLTERM.
#   usage: sh scripts/r14-qemu-serial.sh VOL.img [tcp|tcp:PORT|pty] [KERNEL.img]
#   then, on the host:  tcp  -> e.g. `socat - TCP:127.0.0.1:PORT` (the port is printed)
#                       pty  -> QEMU prints "char device redirected to /dev/pts/N (label r14com1)"; then e.g.
#                               `socat - /dev/pts/N,raw,echo=0` or `screen /dev/pts/N 115200`
# Writes nothing outside ./r14-serial/ (the volume and kernel copies).
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd)
V=${1:?usage}; M=${2:-tcp}; K=${3:-$ROOT/build/r14/pc-vga/r14-vga.img}
D=./r14-serial; mkdir -p "$D"
cp "$V" "$D/hdb.img"; cp "$K" "$D/hda.img"
A=$(sh "$ROOT/scripts/r14-serial-args.sh" "$M" "$D")
[ "$M" = pty ] && echo "COM1: pseudo-terminal; QEMU prints its /dev/pts name below"
DISP=""; [ "${R14_HEADLESS:-0}" = 1 ] && DISP="-display none"
exec qemu-system-i386 -machine pc -cpu pentium -m 16 -vga std $DISP -drive file="$D/hda.img",format=raw,if=ide,index=0 \
  -drive file="$D/hdb.img",format=raw,if=ide,index=1 -boot order=c $A
