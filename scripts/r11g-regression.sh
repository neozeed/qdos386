#!/bin/sh
# R11G: the R11F regression sequences on the R11G volume (one round per mode), compared with the
# R11F logged results for the same sequence.  SCRIPTED keys.  Expected difference: the DIR listing
# (two more entries: INFOCOM.AOU and ZORK1_Z3).  Anything else must match the R11F log.
#   usage: TINY386=<bin> I386VM_HARNESS=<bin> sh scripts/r11g-regression.sh <outdir>
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd); cd "$ROOT"
: "${TINY386:?}" "${I386VM_HARNESS:?}"
OUT=${1:?usage: r11g-regression.sh OUTDIR}; VOL=build/fat16/win1-r11g.img
mkdir -p "$OUT"
H0=$(sha256sum "$VOL" | cut -c1-64)
tiny() {  # tiny MODE SEQ TAG
  MODE=$1; SEQ=$2; TAG=$3; D=build/pc-r11g-$MODE-run; rm -rf "$D"; mkdir -p "$D"
  cp build/pc-r11f-$MODE-run/bios.bin build/pc-r11f-$MODE-run/vgabios.bin "$D/"
  cp build/pc-r11f-$MODE/r11f-$MODE.img "$D/hda.img"
  printf '[pc]\nbios = bios.bin\nvga_bios = vgabios.bin\nmem_size = 8M\nhda = hda.img\nhdb = win1-r11g.img\n\n[cpu]\nfpu = 1\n' > "$D/run.ini"
  cp "$VOL" "$D/win1-r11g.img"
  set --
  while IFS= read -r line; do set -- "$@" "$line"; done < "$SEQ"
  PC_CMD_TIMEOUT=300 python3 tests/interactive/run_pc_multi.py "$TINY386" "$D" "$D/win1-r11g.img" "$OUT/tiny386-$TAG.txt" "$@" > "$OUT/tiny386-$TAG.stdout" 2>&1 || echo "tiny $TAG harness rc!=0"
}
i386() {  # i386 IMAGE SEQ TAG
  IMG=$1; SEQ=$2; TAG=$3; IN=$OUT/i386vm-$TAG-input.txt
  sed -e 's/^[!@]//' "$SEQ" > "$IN"
  timeout 1500 "$I386VM_HARNESS" "$IMG" < "$IN" > "$OUT/i386vm-$TAG.txt" 2>&1 || true
}
tiny none tests/r11f/seq-regression.txt regression
i386 build/i386vm-r11g-none/qdos386-r11g-none.bin tests/r11f/seq-regression.txt regression
tiny pin tests/r11f/seq-pinned.txt pinned
i386 build/i386vm-r11g-pin/qdos386-r11g-pin.bin tests/r11f/seq-pinned.txt pinned
echo "volume before: $H0"; echo "volume after:  $(sha256sum "$VOL" | cut -c1-64)"
echo "REGRESSION DONE"
