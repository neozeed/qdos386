#!/bin/sh
# R11L Stage 3: host oracle for the negative corpus: exit status and diagnostics of the host tools for malformed input.
#   usage: QDOS386_TOOLS=... sh scripts/r11l-oracle-neg.sh OUTFILE
set -u
ROOT=$(cd "$(dirname "$0")/.." && pwd); T=${QDOS386_TOOLS:?}; OUT=${1:?usage}; : > "$OUT"
W=$(mktemp -d); cd "$W"
for h in "$ROOT"/sdk/include/*.h; do cp "$h" "$(basename "$h")"; done
for n in BADC BADINC BADERR; do
	cp "$ROOT/tests/r11l/src/$n.C" "$n.C"
	"$T/bin/cccp" -nostdinc -undef -Di386 -D__i386__ -I. "$n.C" "$n.I" > cccp.txt 2>&1; r1=$?
	"$T/bin/cc1" -quiet -O "$n.I" -o "$n.S" > cc1.txt 2>&1; r2=$?
	{ echo "== $n: cccp rc=$r1"; cat cccp.txt; echo "== $n: cc1 rc=$r2 (run on whatever cccp left behind)"; cat cc1.txt; } >> "$OUT"
done
cd /; rm -rf "$W"
