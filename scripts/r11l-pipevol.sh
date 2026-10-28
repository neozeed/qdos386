#!/bin/sh
# R11L Stage 3+: a scratch volume with the R11K base programs, the guest tools, the SDK headers (upper-case 8.3 names),
# the link inputs (CRT0.S, LIBC.A, LIBM.A, LIBGCC.A, LIBQDOS.A, SLOT1.LD) and any extra NAME=path files.
#   usage: sh scripts/r11l-pipevol.sh OUT.img [NAME=path ...]       (R11L_TOOLDIR selects measurement tools)
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd); cd "$ROOT"
OUT=${1:?usage}; shift
O=logs/r11l/oracle/hello; HH=
for h in sdk/include/*.h; do HH="$HH $(basename $h | tr a-z A-Z)=$h"; done
sh scripts/r11l-s2vol.sh "$OUT" -- CRT0.S=$O/CRT0.S LIBC.A=sdk/lib/libc.a LIBM.A=sdk/lib/libm.a LIBGCC.A=sdk/lib/libgcc-qd.a \
  LIBQDOS.A=sdk/lib/libqdos.a SLOT1.LD=sdk/ld/slot1.ld $HH "$@"
