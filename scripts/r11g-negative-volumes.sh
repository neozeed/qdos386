#!/bin/sh
# R11G negative-test volumes (test only, never a release image).  Each is the R11G volume with a
# different ZORK1_Z3 (or none).  The original story is never modified: variants are written to
# <outdir>.  Usage: ZORK_STORY=<zork1_z3> sh scripts/r11g-negative-volumes.sh <outdir>
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd); cd "$ROOT"
: "${ZORK_STORY:?}"; OUT=${1:?usage: r11g-negative-volumes.sh OUTDIR}
mkdir -p "$OUT/stories"
python3 -I - "$ZORK_STORY" "$OUT/stories" <<'PY'
import sys, os
src, out = sys.argv[1], sys.argv[2]
b = open(src, 'rb').read()
open(os.path.join(out, 'trunc.dat'), 'wb').write(b[:40000])                    # truncated
c = bytearray(b); c[0] = 0x09; c[10:12] = b'\xff\xff'                           # bad version, bad object table
open(os.path.join(out, 'corrupt.dat'), 'wb').write(bytes(c))
open(os.path.join(out, 'oversized.dat'), 'wb').write(b + bytes(range(256)) * 800) # 205 KB of trailing data
open(os.path.join(out, 'empty.dat'), 'wb').write(b'')
import random
r = random.Random(1988)
pc = (b[6] << 8) | b[7]                                                        # initial PC
c = bytearray(b); c[pc:] = bytes(r.randrange(256) for _ in range(len(b) - pc))  # valid header, junk code
open(os.path.join(out, 'badcode.dat'), 'wb').write(bytes(c))
PY
ARGS=$(sed -n '/^ARGS="\\$/,/PHDATE.AOU=build\/r11f\/PHDATE.aout/p' scripts/r11f-volume.sh \
  | sed -e '1s/^ARGS="\\$//' -e 's/ \$LG"$//' -e 's/\\$//')
for v in missing:none trunc:trunc.dat corrupt:corrupt.dat oversized:oversized.dat empty:empty.dat badcode:badcode.dat; do
  name=${v%%:*}; f=${v#*:}
  if [ "$f" = none ]; then EXTRA=""; else EXTRA="ZORK1_Z3=$OUT/stories/$f"; fi
  QDOS_LAYOUT=r9 python3 tools/make-fat16.py "$OUT/win1-r11g-$name.img" $ARGS INFOCOM.AOU=build/r11g/infocom.aout $EXTRA >/dev/null
  echo "$OUT/win1-r11g-$name.img $(sha256sum "$OUT/win1-r11g-$name.img" | cut -c1-16)"
done
