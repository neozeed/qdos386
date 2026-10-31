#!/bin/sh
# R11L: interrupted-operation sweep on disposable images.
# For each operation, the harness is told to "cut the power" (exit 77) when write number N+1 arrives,
# for N = 0, 1, 2, ... until the operation runs to completion.  After every cut the image is judged
# by tools/r11l-fatcheck.py, by fsck.fat -n (dosfstools, independent) and by tools/r11l-interrupt-judge.py
# (what the files contain); then it is repaired with the checker and judged again.
#   usage: sh scripts/r11l-interrupt.sh KERNEL.bin OUTDIR
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd); cd "$ROOT"
K=${1:?usage}; OUT=${2:?usage}; mkdir -p "$OUT/img"
BASE=$OUT/base.img
# the base volume: the s1 volume plus PRE.DAT (3000 bytes of pattern 31), written by the host
python3 - "$OUT/PRE.DAT" <<'PY'
import sys
def pat(i, seed): return (i * 131 + seed * 17 + (i >> 8) * 7) & 0xff
open(sys.argv[1], 'wb').write(bytes(pat(i, 31) for i in range(3000)))
PY
touch -d '2026-10-28 08:00:00' "$OUT/PRE.DAT"
ARGS=$(sh scripts/r11l-base-list.sh | grep -v 'BIG.BIN\|^BAD\|^CHAIN\|^EMPTY' | tr '\n' ' ')
find build/r11l/base -type f | xargs touch -d '2026-10-28 08:00:00'
sh scripts/r11l-mkvol.sh "$BASE" 3072 1 512 R11LINT $ARGS WRTTEST.AOU=build/r11l/stage/WRTTEST.AOU PRE.DAT="$OUT/PRE.DAT"
python3 tools/r11l-fatcheck.py "$BASE" > "$OUT/base-check.txt"
: > "$OUT/sweep.csv"
echo "op,cut_after_writes,completed,fatcheck_errors,fatcheck_warnings,fsck_rc,semantic,repaired_clean,repaired_fsck_rc" >> "$OUT/sweep.csv"
for OP in create append overwrite shrink trunc0 remove rename; do
  N=0
  while :; do
    I=$OUT/img/$OP-$N.img
    cp "$BASE" "$I"
    printf 'RUN WRTTEST.AOU op=%s\n' "$OP" > "$OUT/seq.txt"
    QDOS_BLK_FAULT=exit:$N sh scripts/r11l-run-vm.sh "$K" "$I" "$OUT/seq.txt" "$OUT/img/$OP-$N.txt" 120
    if grep -q "^OP-DONE" "$OUT/img/$OP-$N.txt"; then DONE=1; else DONE=0; fi
    python3 tools/r11l-interrupt-judge.py "$OP" "$N" "$DONE" "$I" "$OUT/sweep.csv" "$OUT/img/$OP-$N" || true
    if [ $DONE = 1 ]; then break; fi
    rm -f "$I"                       # keep only the completed image of each operation
    N=$((N + 1))
    if [ $N -gt 80 ]; then echo "sweep did not terminate for $OP" >&2; exit 5; fi
  done
done
echo "sweep done: $(wc -l < "$OUT/sweep.csv") lines"
