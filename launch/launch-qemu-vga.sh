#!/bin/sh
# Interactive VGA mode in QEMU: boot disk on IDE primary master, FAT16 data
# disk on primary slave, graphical window (gtk), x87 present (pentium).
# Port 0x402 (the Tiny386 debug port, kept by the kernel) is captured to a
# text log with QEMU's isa-debugcon device.
# usage: launch-qemu-vga.sh DIR MONITOR_PORT
D=$1; P=$2
exec qemu-system-i386 -machine pc -cpu pentium -m 16 \
  -bios "$D/bios.bin" -vga std -display gtk \
  -monitor tcp:127.0.0.1:$P,server,nowait \
  -drive file="$D/hda.img",format=raw,if=ide,index=0,media=disk \
  -drive file="$D/hdb.img",format=raw,if=ide,index=1,media=disk \
  -chardev file,id=dbg,path="$D/debug-0x402.txt" \
  -device isa-debugcon,iobase=0x402,chardev=dbg \
  -boot order=c -no-reboot
