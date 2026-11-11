#!/bin/sh
# Apollo 16 Phase 2B test volume: a copy of BASE (BASE itself is not modified) with WIN1:/PAT2.BIN (1572864 bytes,
# P(i) = (i*37 + (i>>8)*11 + (i>>16)) & 0xff) and the protected R20GS, R20GW, R20GL, R20C, R20P in /BIN.
#   usage: sh scripts/a16-p2b-sysvol.sh OUT.img BASE.img
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd); cd "$ROOT"; export MTOOLS_SKIP_CHECK=1
OUT=${1:?}; BASE=${2:?}; cp "$BASE" "$OUT"
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
python3 -c "
import sys
open(sys.argv[1],'wb').write(bytes(((i*37)+((i>>8)*11)+(i>>16))&0xff for i in range(1572864)))" "$T/pat2.bin"
mcopy -o -m -i "$OUT" "$T/pat2.bin" "::/PAT2.BIN"
for p in R20GS R20GW R20GL R20C R20P; do
	cp build/a16/${p}P.aout "$T/x"; touch -d '2026-10-31 08:00:00' "$T/x"; mcopy -o -m -i "$OUT" "$T/x" "::/BIN/$p.AOU"
done
fsck.fat -n "$OUT" > /dev/null
echo "$OUT"
