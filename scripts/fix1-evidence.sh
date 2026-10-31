#!/bin/sh
# R11M-FIX1: copy the finished runs into logs/fix1 (the evidence directory of the package) and write logs/fix1/SUMMARY.txt.
#   usage: sh scripts/fix1-evidence.sh SRC     SRC holds: rep1 base mx1 rg1 win1 demo1 q1 keep qsmoke(optional) and the *.log files of the background runs
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd); cd "$ROOT"
S=${1:?usage}; D=logs/fix1
rm -rf "$D"; mkdir -p "$D"
cp_ () { [ -e "$S/$1" ] && { mkdir -p "$D/$(dirname "$2")"; cp -r "$S/$1" "$D/$2"; } || true; }
cp_ rep1 phase1/repro-kill-original-sequence
cp_ base phase1/baseline-unmodified-exit-path
cp_ mx1 matrix
cp_ mx1.log matrix.log
cp_ rg1 regress
cp_ win1 window
cp_ win1.log window.log
cp_ demo1 demo
cp_ demo1.log demo.log
cp_ q1 qemu-port-8900
cp_ keep fsize-injection
cp_ qsmoke qemu-control-smoke
cp_ qsmoke.log qemu-control-smoke.log
[ -d logs/r11m/noflush ] && { mkdir -p "$D/r11m-noflush"; cp -r logs/r11m/noflush/. "$D/r11m-noflush/"; }
# inputs that are copies of the initial volume, and the emulator's X server log, are not evidence
find "$D" \( -name 'vol.img' -o -name 'v[0-9].img' -o -name 'f-*.img' -o -name 'e-*.img' -o -name 'initial.img' -o -name 'demo-initial.img' -o -name 'xvfb.log' \) -type f -delete
rm -rf "$D"/window/w-*/hda.img "$D"/window/w-*/win1.img.orig 2>/dev/null || true
python3 scripts/fix1-summary.py "$D" > "$D/SUMMARY.txt" || true
cat "$D/SUMMARY.txt"
