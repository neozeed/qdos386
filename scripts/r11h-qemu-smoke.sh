#!/bin/sh
# SCRIPTED QEMU VGA run (R11H).  Keystrokes are injected with xdotool into the QEMU window on an
# X display; this is NOT a human session.  Screenshots are taken after each stage.  The kernel is
# build/pc-r11h-vga/r9shell-vga.img (the R11H kernel, VGA console, pinned clock).  The volume is the
# story volume build/fat16/win1-r11h-smoke.img on the second IDE drive (private test volume).
# The smoke volume is the Zork volume plus the ten malformed NEG images (build/r11h-neg/*.AOU), so
# NEG sees the same inputs as the harness matrix.  Built with R11H_VOLUME_OUT=... ZORK_STORY=... and the
# malformed images as NAME.AOU=path arguments to scripts/r11h-volume.sh.
#   usage: DISPLAY=:99 FW=<dir with bios.bin> sh scripts/r11h-qemu-smoke.sh OUTDIR
set -eu
cd "$(dirname "$0")/.."
OUT=${1:?usage: r11h-qemu-smoke.sh OUTDIR}
: "${DISPLAY:?set DISPLAY}" "${FW:?set FW to the directory with bios.bin}"
W=$(mktemp -d "${TMPDIR:-/tmp}/r11h-qemu.XXXXXX")
cp build/pc-r11h-vga/r9shell-vga.img "$W/hda.img"
cp build/fat16/win1-r11h-smoke.img "$W/hdb.img"
VOL_BEFORE=$(sha256sum "$W/hdb.img" | cut -c1-64)
mkdir -p "$OUT"
qemu-system-i386 -machine pc -cpu pentium -m 16 \
  -bios "$FW/bios.bin" -vga std -display gtk \
  -monitor tcp:127.0.0.1:4456,server,nowait \
  -drive file="$W/hda.img",format=raw,if=ide,index=0,media=disk \
  -drive file="$W/hdb.img",format=raw,if=ide,index=1,media=disk \
  -serial file:"$OUT/qemu-serial.txt" \
  -boot order=c -no-reboot >"$OUT/qemu-stderr.txt" 2>&1 &
QPID=$!
sleep 30
WID=$(xdotool search --name 'QEMU' | head -1)
xdotool windowfocus --sync "$WID" 2>/dev/null || true
send() { xdotool type --delay 60 "$1"; xdotool key Return; sleep "${2:-12}"; }
send "run multi.aou" 45
import -window root "$OUT/qemu-01-multi.png"
send "run neg.aou" 60
import -window root "$OUT/qemu-02-neg.png"
send "dir" 12
import -window root "$OUT/qemu-03-dir.png"
send "run infocom.aou" 25
for cmd in look inventory "open mailbox" "take leaflet" "read leaflet" north look; do
  send "$cmd" 12
done
import -window root "$OUT/qemu-04-zork.png"
send "quit" 6
send "y" 12
send "dir" 12
import -window root "$OUT/qemu-05-shell-after-quit.png"
kill $QPID 2>/dev/null || true
wait $QPID 2>/dev/null || true
VOL_AFTER=$(sha256sum "$W/hdb.img" | cut -c1-64)
echo "SCRIPTED keystrokes (xdotool), not human-operated"
echo "serial (COM1) transcript: $OUT/qemu-serial.txt"
echo "volume sha256 before: $VOL_BEFORE"
echo "volume sha256 after:  $VOL_AFTER"
[ "$VOL_BEFORE" = "$VOL_AFTER" ] && echo "volume unchanged by the run" || echo "VOLUME CHANGED"
