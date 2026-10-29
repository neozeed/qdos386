#!/bin/sh
# R12: the scratch volume for the interactive stages: the FIX1 demonstration volume (R11M shell, test programs, Phoon, SHUTDOWN.AOU) plus
# R12 programs that exist, and a DOCS tree for DIR /W and /P (host-made VFAT long names, an empty directory, a quoted long directory name).
#   usage: sh scripts/r12-vol.sh OUT.img [KIB]      extra programs: R12_EXTRA="NAME.AOU=path ..." in the environment
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd); cd "$ROOT"
export MTOOLS_SKIP_CHECK=1 LC_ALL=C.UTF-8
E=sdk/examples
OUT=${1:?usage}; KIB=${2:-3072}
EXTRA=""
for n in CONAPI CONDEMO EDIT LEAK CONLOAD BASIC BASTEST; do   # R12 programs, included once they exist
  [ -f "$E/$n.aout" ] && EXTRA="$EXTRA $n.AOU=$E/$n.aout"
done
for kv in ${R12_EXTRA:-}; do EXTRA="$EXTRA $kv"; done
# shellcheck disable=SC2086
sh scripts/r11m-mkvol.sh "$OUT" "$KIB" 1 512 CONT1.AOU=$E/CONT1.aout COUNTER.AOU=$E/COUNTER.aout TICKER.AOU=$E/TICKER.aout \
  CONINP.AOU=$E/CONINP.aout JOBT.AOU=$E/JOBT.aout WRITER.AOU=$E/WRITER.aout PATHT.AOU=$E/PATHT.aout DIRFUL.AOU=$E/DIRFUL.aout \
  VFATT.AOU=$E/VFATT.aout SHUTDOWN.AOU=$E/SHUTDOWN.aout $EXTRA >/dev/null
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
printf 'sample text file\n' > "$T/s.txt"
mmd -i "$OUT" ::/DOCS ::/EMPTY "::/Long Directory Name"
i=1
while [ $i -le 30 ]; do
  n=$(printf 'File %02d with a longish name.txt' $i)
  mcopy -i "$OUT" "$T/s.txt" "::/DOCS/$n"
  i=$((i+1))
done
for n in A.TXT B.C README.MD "Mixed Case Name.Doc" "x" "this.is.a.test.c" "UPPER.TXT"; do mcopy -i "$OUT" "$T/s.txt" "::/DOCS/$n"; done
mmd -i "$OUT" "::/DOCS/Sub Directory One" "::/DOCS/SUBTWO"
mcopy -i "$OUT" "$T/s.txt" "::/Long Directory Name/Inner file name here.txt"
# page-boundary directories: EXACT21 holds 21 files (header + 21 + summary = 23 lines, exactly one /P page), EXACT22 holds 22 (one line over)
mmd -i "$OUT" ::/EXACT21 ::/EXACT22
i=1
while [ $i -le 22 ]; do
  n=$(printf 'F%02d.TXT' $i)
  [ $i -le 21 ] && mcopy -i "$OUT" "$T/s.txt" "::/EXACT21/$n"
  mcopy -i "$OUT" "$T/s.txt" "::/EXACT22/$n"
  i=$((i+1))
done
# WIDE: 200 short names, enough for several pages in the /W listing
mmd -i "$OUT" ::/WIDE
i=1
while [ $i -le 200 ]; do
  mcopy -i "$OUT" "$T/s.txt" "::/WIDE/$(printf 'W%03d.TXT' $i)"
  i=$((i+1))
done
