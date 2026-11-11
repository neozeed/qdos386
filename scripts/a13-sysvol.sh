#!/bin/sh
# Apollo 13 (R17) test volume: R16T, BOUNCEP, the R17 TOP and CPUSET (they also understand R16B), MEMINFO, RAMCHK, SMPT
# (protected) in /BIN, on the PUBLIC-SAFE system volume (scripts/qdos386-public-vol.sh: no SuperBASIC, no Infocom).
#   usage: sh scripts/a13-sysvol.sh OUT.img [KIB]
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd); cd "$ROOT"; export MTOOLS_SKIP_CHECK=1
OUT=${1:?}; sh scripts/qdos386-public-vol.sh "$OUT" ${2:-} > /dev/null
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
for p in R16T:build/r16/R16TP.aout TOP:build/a13/TOP17P.aout CPUSET:build/a13/CPUSET17P.aout BOUNCEP:build/r16b/BOUNCEP.aout \
         MEMINFO:build/a13/MEMINFOP.aout RAMCHK:build/a13/RAMCHKP.aout SMPT:build/a13/SMPTP.aout; do
	cp "${p#*:}" "$T/x"; touch -d '2026-10-31 08:00:00' "$T/x"; mcopy -o -m -i "$OUT" "$T/x" "::/BIN/${p%%:*}.AOU"
done
fsck.fat -n "$OUT" > /dev/null
echo "$OUT"
