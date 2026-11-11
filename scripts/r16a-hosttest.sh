#!/bin/sh
# R16A host unit test: build the fixtures, compile the locator + FAT test against the real kernel sources, run it, and
# confirm the fixture images are byte-identical afterwards (the test opens them read-only).
#   usage: sh scripts/r16a-hosttest.sh [OUTDIR]
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd); cd "$ROOT"
O=${1:-logs/r16a/host}; mkdir -p "$O"
sh scripts/r16a-mkdata.sh "$O/fix" >/dev/null
before=$(sha256sum "$O/fix/data-mbr.img" "$O/fix/data-superfloppy.img")
cc -std=gnu99 -Ikernel tests/host/r16a/test_mbr.c kernel/mbr.c kernel/fat16.c -o "$O/test_mbr"
"$O/test_mbr" "$O/fix"; rc=$?
after=$(sha256sum "$O/fix/data-mbr.img" "$O/fix/data-superfloppy.img")
if [ "$before" = "$after" ]; then echo "fixtures byte-identical after the test (read-only)"; else echo "FAIL: a fixture image changed"; rc=1; fi
exit $rc
