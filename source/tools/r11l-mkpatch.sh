#!/bin/sh
# R11L: regenerate port/patches/*.patch from the edits made in the staged guest tree (build/r11l/src) relative to the
# untouched LF copy (build/r11l/src-host).  The patches are the only guest-specific change to the legacy sources; tools/
# r11l-stage-src.sh applies them in order.  Usage: sh tools/r11l-mkpatch.sh
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd); cd "$ROOT/build/r11l"; mkdir -p "$ROOT/port/patches"
for d in gcc-1.40 gas-1.38 binutils-1.94; do
	diff -ruN src-host/$d src/$d > "$ROOT/port/patches/$d.patch" || true
	[ -s "$ROOT/port/patches/$d.patch" ] || rm -f "$ROOT/port/patches/$d.patch"
done
ls -l "$ROOT"/port/patches
