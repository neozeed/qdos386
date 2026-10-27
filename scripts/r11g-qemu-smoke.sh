#!/bin/sh
# SCRIPTED QEMU VGA run (R11G).  Keystrokes are injected with xdotool into the QEMU GTK window on
# an X display; this is NOT a human session.  Screenshots are taken after each stage so the
# on-screen Zork text can be checked.  The kernel is the VGA build (build/pc-r11f-vga/r9shell-vga.img,
# the same R11F kernel), the volume is build/fat16/win1-r11g.img on the second IDE drive.
#   usage: DISPLAY=:99 FW=<dir with bios.bin> sh scripts/r11g-qemu-smoke.sh OUTDIR
set -eu
cd "$(dirname "$0")/.."
OUT=${1:?usage: r11g-qemu-smoke.sh OUTDIR}
: "${DISPLAY:?set DISPLAY}" "${FW:?set FW to the directory with bios.bin}"
W=$(mktemp -d "${TMPDIR:-/tmp}/r11g-qemu.XXXXXX")
cp build/pc-r11f-vga/r9shell-vga.img "$W/hda.img"
cp build/fat16/win1-r11g.img "$W/hdb.img"
VOL_BEFORE=$(sha256sum "$W/hdb.img" | cut -c1-64)
mkdir -p "$OUT"
qemu-system-i386 -machine pc -cpu pentium -m 16 \
  -bios "$FW/bios.bin" -vga std -display gtk \
  -monitor tcp:127.0.0.1:4455,server,nowait \
  -drive file="$W/hda.img",format=raw,if=ide,index=0,media=disk \
  -drive file="$W/hdb.img",format=raw,if=ide,index=1,media=disk \
  -boot order=c -no-reboot >"$OUT/qemu-stderr.txt" 2>&1 &
QPID=$!
sleep 30
WID=$(xdotool search --name 'QEMU' | head -1)
xdotool windowfocus --sync "$WID" 2>/dev/null || true
send() { xdotool type --delay 60 "$1"; xdotool key Return; sleep "${2:-12}"; }
send "run infocom.aou" 25
import -window root "$OUT/qemu-01-opening.png"
for cmd in look inventory "open mailbox" "take leaflet" "read leaflet" north look; do
  send "$cmd" 12
done
import -window root "$OUT/qemu-02-after-commands.png"
send "quit" 6
send "y" 12
send "dir" 12
import -window root "$OUT/qemu-03-shell-after-quit.png"
# R11F regressions on the same graphical kernel: PHCHK (heap/channel check) and PHOON (moon).
send "run phchk.aou" 40
import -window root "$OUT/qemu-04-phchk.png"
send "run phoon.aou" 40
import -window root "$OUT/qemu-05-phoon.png"
kill $QPID 2>/dev/null || true
wait $QPID 2>/dev/null || true
VOL_AFTER=$(sha256sum "$W/hdb.img" | cut -c1-64)
echo "SCRIPTED keystrokes (xdotool), not human-operated"
echo "volume sha256 before: $VOL_BEFORE"
echo "volume sha256 after:  $VOL_AFTER"
[ "$VOL_BEFORE" = "$VOL_AFTER" ] && echo "volume unchanged by the run" || echo "VOLUME CHANGED"
