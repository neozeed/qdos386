#!/bin/sh
# R11M: copy the final evidence run into logs/r11m and derive SUMMARY.txt and the demonstration screens.
#   usage: sh scripts/r11m-evidence.sh FINAL_RUN_DIR [NOFLUSH_DIR]     (writes logs/r11m/)
# Scratch images: the final images of round 1 of every emulator are kept (they are what fsck.fat / fatwalk / mtools judged); in rounds 2 and 3 and for the
# initial per-case volumes only the judge output is kept (the initial volumes are rebuilt by scripts/r11m-demovol.sh and shipped in volumes/).
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd); cd "$ROOT"
SRC=${1:?usage}; NF=${2:-}
DST=logs/r11m; rm -rf "$DST"; mkdir -p "$DST"
cp -a "$SRC"/. "$DST"/
for r in 2 3; do find "$DST"/vm-r$r "$DST"/pc-r$r "$DST"/qemu-r$r -name '*.img' -delete 2>/dev/null || true; done
find "$DST" -name 'vol-*.img' -delete 2>/dev/null || true
find "$DST" -name 'demo-initial.img' -delete 2>/dev/null || true
[ -n "$NF" ] && [ -d "$NF" ] && { mkdir -p "$DST/noflush"; cp -a "$NF"/. "$DST/noflush/"; }
rm -f "$DST"/*.done
python3 scripts/r11m-screens.py "$DST/qemu-r1/demo/qemu-a.txt" > "$DST/demo-screens-qemu.txt"
python3 scripts/r11m-summary.py "$DST" > "$DST/SUMMARY.txt" || echo "SUMMARY reports problems" >&2
cat "$DST/SUMMARY.txt" | tail -n 3
du -sh "$DST"
