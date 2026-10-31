#!/bin/sh
# R11M-FIX1: smoke test of the WINDOWED Tiny386 frontend (rawdraw, built as `tiny386`) under a virtual X server (Xvfb) with keys
# typed by xdotool.  Two short runs, each on its own copy of the fix1 volume:
#   guest  : boot, type "RUN SHUTDOWN.AOU" into the window; the emulator must exit by itself with status 0 and report
#            "guest shutdown request; disk images flushed and closed"; the image must pass scripts/fix1-check-image.sh
#   term   : boot, SIGTERM; status 0, "stopped; disk images flushed and closed"
# Timing based (the screen is not read): BOOT_WAIT seconds (default 40) before typing, END_WAIT (default 120) for the exit.
# Labelled in the reports as a smoke test of the shared shutdown path, not as a matrix.
#   usage: TINY386_WIN=<patched windowed tiny386> sh scripts/fix1-window-smoke.sh OUTDIR [KERNEL_IMG]
set -u
ROOT=$(cd "$(dirname "$0")/.." && pwd); cd "$ROOT"
: "${TINY386_WIN:?set TINY386_WIN to the patched windowed tiny386}"
OUT=${1:?usage}; K=${2:-build/r11m/pc-pg/r11m-pg.img}; mkdir -p "$OUT"; OUT=$(cd "$OUT" && pwd)
BOOT_WAIT=${BOOT_WAIT:-40}; END_WAIT=${END_WAIT:-120}
DN=${FIX1_XDISPLAY:-97}
BAD=0
ok()  { echo "  ok   $1"; }
bad() { echo "  FAIL $1"; BAD=$((BAD + 1)); }
sh scripts/fix1-vol.sh "$OUT/initial.img" 3072
Xvfb ":$DN" -screen 0 1024x768x24 >"$OUT/xvfb.log" 2>&1 &
XPID=$!
sleep 2
export DISPLAY=":$DN"
run() {  # run TAG MODE
  tag=$1; mode=$2; W=$OUT/w-$tag; rm -rf "$W"; mkdir -p "$W"
  cp build/pc-r11f-none-run/bios.bin build/pc-r11f-none-run/vgabios.bin "$W/"; cp "$K" "$W/hda.img"; cp "$OUT/initial.img" "$W/win1.img"
  printf '[pc]\nbios = bios.bin\nvga_bios = vgabios.bin\nmem_size = 8M\nhda = hda.img\nhdb = win1.img\n\n[cpu]\nfpu = 1\n' > "$W/run.ini"
  ( cd "$W" && exec "$TINY386_WIN" run.ini > emu.out 2> emu.err ) &
  EP=$!
  sleep "$BOOT_WAIT"
  if ! kill -0 $EP 2>/dev/null; then bad "$tag: emulator not running after boot wait"; return; fi
  if [ "$mode" = guest ]; then
    WID=$(xdotool search --onlyvisible --name "" 2>/dev/null | tail -1)
    [ -n "$WID" ] && xdotool mousemove 200 200 windowfocus "$WID" 2>/dev/null
    sleep 1; xdotool type --delay 120 "RUN SHUTDOWN.AOU" 2>/dev/null; sleep 1; xdotool key Return 2>/dev/null
  else
    kill -TERM $EP
  fi
  n=0; while kill -0 $EP 2>/dev/null && [ $n -lt "$END_WAIT" ]; do sleep 1; n=$((n + 1)); done
  if kill -0 $EP 2>/dev/null; then kill -KILL $EP; wait $EP 2>/dev/null; bad "$tag: emulator did not exit within ${END_WAIT}s"; return; fi
  wait $EP; rc=$?
  [ $rc -eq 0 ] && ok "$tag: exit status 0" || bad "$tag: exit status $rc"
  if [ "$mode" = guest ]; then
    grep -q "guest shutdown request; disk images flushed and closed" "$W/emu.err" && ok "$tag: guest shutdown request reported" || bad "$tag: no guest shutdown report ($(tail -1 "$W/emu.err"))"
  else
    grep -q "stopped; disk images flushed and closed" "$W/emu.err" && ok "$tag: SIGTERM stop reported" || bad "$tag: no stop report ($(tail -1 "$W/emu.err"))"
  fi
  if sh scripts/fix1-check-image.sh "$W/win1.img" "$W/check.fsck.txt" > "$W/check.txt" 2>&1; then ok "$tag: image clean (FAT copies, fsck.fat, fatwalk)"; else bad "$tag: image check: $(grep FAIL "$W/check.txt" | head -1)"; fi
}
run guest guest
run term term
kill $XPID 2>/dev/null
if [ $BAD -eq 0 ]; then echo "FIX1 WINDOW SMOKE PASS"; else echo "FIX1 WINDOW SMOKE FAIL ($BAD)"; fi
[ $BAD -eq 0 ]
