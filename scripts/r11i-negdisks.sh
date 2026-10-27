#!/bin/sh
# R11I malformed-disk cases for the block harness.  Each case boots the block kernel (or is refused
# by the harness before boot) and must end deterministically: no hang, no write to any file.
#   usage: I386VM_BLOCK_HARNESS=<bin> sh scripts/r11i-negdisks.sh <outdir> <block image> <volume>
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd); cd "$ROOT"
: "${I386VM_BLOCK_HARNESS:?}"
OUT=${1:?}; IMG=${2:?}; VOL=${3:?}; mkdir -p "$OUT"
H0=$(sha256sum "$VOL" | cut -c1-64)
printf 'DIR\n' > "$OUT/seq-dir.txt"
: > "$OUT/empty.img"
head -c 511 "$VOL" > "$OUT/odd511.img"
head -c 3145728 /dev/zero > "$OUT/zero3m.img"
head -c 1048576 "$VOL" > "$OUT/trunc1m.img"
head -c $(( $(wc -c < "$VOL") - 512 )) "$VOL" > "$OUT/cut1sec.img"
run() {  # run TAG [disk]
  TAG=$1; shift
  timeout 120 "$I386VM_BLOCK_HARNESS" "$IMG" "$@" < "$OUT/seq-dir.txt" > "$OUT/out-$TAG.txt" 2>&1 && rc=0 || rc=$?
  echo "$TAG rc=$rc: $(tr -d '\r' < "$OUT/out-$TAG.txt" | grep -E 'refused|mounted|files' | head -1)"
}
run nodisk
run missing "$OUT/nope.img"
run empty "$OUT/empty.img"
run odd511 "$OUT/odd511.img"
run zero3m "$OUT/zero3m.img"
run trunc1m "$OUT/trunc1m.img"
run cut1sec "$OUT/cut1sec.img"
run good "$VOL"
for f in empty odd511 zero3m trunc1m cut1sec; do sha256sum "$OUT/$f.img"; done > "$OUT/after.sha"
echo "production volume $H0 -> $(sha256sum "$VOL" | cut -c1-64)"
