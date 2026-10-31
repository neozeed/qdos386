#!/bin/sh
# Build the R11F kernel images for the three clock modes, on i386VM and PC.
#   none: no clock (regression image: time is unavailable).
#   pin : pinned clock at 1793102400 (2026-10-27 12:00:00 UTC) for Phoon and the fixed-clock tests.
#   adv : advancing clock from 946684800 (2000-01-01 00:00:00 UTC).
# The i386VM images embed build/fat16/win1-r11f.img: rebuild them after any volume change.
#   usage: QDOS386_TOOLS=<tree> I386VM=<same tree> sh scripts/r11f-images.sh
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd)
cd "$ROOT"
: "${I386VM:?set I386VM to the historical tree}"
R9="-DR7_SHELL_MODE -DQDOS_LAYOUT_R9"
for MODE in none pin adv; do
	case $MODE in
	none) CF="" ;;
	pin)  CF="-DQDOS_CLOCK_EPOCH=1793102400 -DQDOS_CLOCK_PINNED" ;;
	adv)  CF="-DQDOS_CLOCK_EPOCH=946684800" ;;
	esac
	make i386vm I386VM="$I386VM" I386D=build/i386vm-r11f-$MODE I386FLAGS="$R9 $CF" \
		I386IMG=qdos386-r11f-$MODE.bin I386WIN=build/fat16/win1-r11f.img IMGENV="QDOS_LAYOUT=r9" >/dev/null
	make pc PCD=build/pc-r11f-$MODE PCFLAGS="$R9 $CF" PCIMG=r11f-$MODE.img \
		PCBOOT=build/pc-r9/boot.bin PCWIN=build/fat16/win1-r11f.img >/dev/null
	echo "built $MODE: build/i386vm-r11f-$MODE/qdos386-r11f-$MODE.bin build/pc-r11f-$MODE/r11f-$MODE.img"
done
# Test-only control image: the advancing i386VM kernel on the control volume (which alone
# contains TMABIW).  Never part of the release images.  See the control in r11f-run-all.sh.
make i386vm I386VM="$I386VM" I386D=build/i386vm-r11f-control I386FLAGS="$R9 -DQDOS_CLOCK_EPOCH=946684800" \
	I386IMG=qdos386-r11f-control.bin I386WIN=build/fat16/win1-r11f-control.img IMGENV="QDOS_LAYOUT=r9" >/dev/null
echo "built control (test only): build/i386vm-r11f-control/qdos386-r11f-control.bin"
