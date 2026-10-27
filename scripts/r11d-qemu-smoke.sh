#!/bin/sh
# SCRIPTED QEMU VGA smoke test (R11D).  Keystrokes are injected with xdotool into
# the QEMU GTK window on an X display (Xvfb).  This is NOT a human session.
#   usage: DISPLAY=:99 FW=<dir with bios.bin> sh scripts/r11d-qemu-smoke.sh OUTDIR
# Needs: qemu-system-i386, xdotool, ImageMagick (import), a running X server,
# build/pc-r10vga/r9shell-vga.img and build/fat16/win1-r11d.img (scripts/r11d-volume.sh).
set -eu
cd "$(dirname "$0")/.."
OUT=${1:?usage: r11d-qemu-smoke.sh OUTDIR}
: "${DISPLAY:?set DISPLAY}" "${FW:?set FW to the directory with bios.bin}"
W=$(mktemp -d "${TMPDIR:-/tmp}/r11d-qemu.XXXXXX")
cp build/pc-r10vga/r9shell-vga.img "$W/hda.img"
cp build/fat16/win1-r11d.img "$W/hdb.img"
VOL_BEFORE=$(sha256sum "$W/hdb.img" | cut -c1-64)
mkdir -p "$OUT"
qemu-system-i386 -machine pc -cpu pentium -m 16 \
  -bios "$FW/bios.bin" -vga std -display gtk \
  -monitor tcp:127.0.0.1:4455,server,nowait \
  -drive file="$W/hda.img",format=raw,if=ide,index=0,media=disk \
  -drive file="$W/hdb.img",format=raw,if=ide,index=1,media=disk \
  -chardev file,id=dbg,path="$W/debug-0x402.txt" \
  -device isa-debugcon,iobase=0x402,chardev=dbg \
  -boot order=c -no-reboot >"$OUT/qemu-stderr.txt" 2>&1 &
QPID=$!
sleep 30
WID=$(xdotool search --name 'QEMU' | head -1)
xdotool windowfocus --sync "$WID" 2>/dev/null || true
for cmd in "dir" "run c89cor.aou" "run jmptst.aou" "run mathx.aou"; do
  xdotool type --delay 60 "$cmd"
  xdotool key Return
  sleep 15
done
# SCANTS reads two integers from its input line.
xdotool type --delay 60 "run scants.aou"
xdotool key Return
sleep 10
xdotool type --delay 60 "3 4"
xdotool key Return
sleep 15
i=0
while ! grep -q "MATHX PASS\|MATHX FAIL" "$W/debug-0x402.txt" 2>/dev/null && [ $i -lt 200 ]; do
  sleep 3; i=$((i + 1))
done
sleep 5
import -window root "$OUT/qemu-r11d-scripted.png"
kill $QPID 2>/dev/null || true
wait $QPID 2>/dev/null || true
cp "$W/debug-0x402.txt" "$OUT/debug-0x402.txt" 2>/dev/null || true
VOL_AFTER=$(sha256sum "$W/hdb.img" | cut -c1-64)
echo "volume sha256 before: $VOL_BEFORE"
echo "volume sha256 after:  $VOL_AFTER"
[ "$VOL_BEFORE" = "$VOL_AFTER" ] && echo "volume unchanged by the run" || echo "VOLUME CHANGED"
echo "banners in debug log:"
grep -E 'PASS|FAIL' "$OUT/debug-0x402.txt" 2>/dev/null || echo "(none)"
