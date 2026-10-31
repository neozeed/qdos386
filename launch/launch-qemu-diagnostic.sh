#!/bin/sh
# Diagnostic mode in QEMU: same images and devices as the interactive launcher,
# but headless (-display none) and with the 0x402 debug channel captured, so the
# automated transcript can be compared with the Tiny386 diagnostic log.
# usage: launch-qemu-diagnostic.sh DIR MONITOR_PORT
D=$1; P=$2
exec qemu-system-i386 -machine pc -cpu pentium -m 16 \
  -bios "$D/bios.bin" -vga std -display none \
  -monitor tcp:127.0.0.1:$P,server,nowait \
  -drive file="$D/hda.img",format=raw,if=ide,index=0,media=disk \
  -drive file="$D/hdb.img",format=raw,if=ide,index=1,media=disk \
  -chardev file,id=dbg,path="$D/debug-0x402.txt" \
  -device isa-debugcon,iobase=0x402,chardev=dbg \
  -device isa-debug-exit,iobase=0xf4,iosize=0x04 \
  -boot order=c -no-reboot
