#!/bin/sh
# Interactive VGA mode in a windowed Tiny386 (the local build of the patched dev tree).
# usage: launch-tiny386-vga.sh DIR TINY386_BINARY
# DIR must contain bios.bin, vgabios.bin, hda.img, hdb.img and interactive.ini.
D=$1; B=$2
cd "$D" && exec "$B" interactive.ini
