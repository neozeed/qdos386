#!/bin/sh
# Apollo 14 (R18) test volume: the R17 volume (scripts/a13-sysvol.sh: public-safe, R16T TOP CPUSET BOUNCEP MEMINFO
# RAMCHK SMPT) plus PLANAR (protected) in /BIN (GFXA, COLOUR, SURFT, BOUNCE are already there).
#   usage: sh scripts/a14-sysvol.sh OUT.img [KIB]
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd); cd "$ROOT"; export MTOOLS_SKIP_CHECK=1
OUT=${1:?}; sh scripts/a13-sysvol.sh "$OUT" ${2:-} > /dev/null
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
for p in PLANAR:build/a14/PLANARP.aout VIEW:build/a14/VIEWP.aout; do
	cp "${p#*:}" "$T/x"; touch -d '2026-10-31 08:00:00' "$T/x"; mcopy -o -m -i "$OUT" "$T/x" "::/BIN/${p%%:*}.AOU"
done
fsck.fat -n "$OUT" > /dev/null
echo "$OUT"
