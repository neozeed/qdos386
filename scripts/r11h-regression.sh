#!/bin/sh
# R11H regression (SCRIPTED keys, both emulators): the R11F regression and pinned-clock sequences
# (tests/r11f/seq-regression.txt, seq-pinned.txt) on the R11H release volume, one run per mode.
# Expected differences from the R11F logs (logs/r11f): the DIR listing has six more entries
# (INFOCOM and the five R11H programs MULTI, MULTIA, MULTIB, COLLIDE, NEG), the file count changes
# accordingly, the free-heap figures shift by 2,608 bytes (kernel growth), and the timing counters
# of the FPMULT program vary.  Anything else must match the R11F log.
#   usage: TINY386=<bin> I386VM_HARNESS=<bin> sh scripts/r11h-regression.sh <outdir>
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd); cd "$ROOT"
: "${TINY386:?}" "${I386VM_HARNESS:?}"
OUT=${1:?usage: r11h-regression.sh OUTDIR}; VOL=build/fat16/win1-r11h.img
mkdir -p "$OUT"
H0=$(sha256sum "$VOL" | cut -c1-64)
tiny() {  # tiny MODE SEQ TAG
  MODE=$1; SEQ=$2; TAG=$3; D=build/pc-r11h-$MODE-regrun; rm -rf "$D"; mkdir -p "$D"
  cp build/pc-r11f-$MODE-run/bios.bin build/pc-r11f-$MODE-run/vgabios.bin "$D/"
  cp build/pc-r11h-$MODE/r11h-$MODE.img "$D/hda.img"
  printf '[pc]\nbios = bios.bin\nvga_bios = vgabios.bin\nmem_size = 8M\nhda = hda.img\nhdb = win1-r11h.img\n\n[cpu]\nfpu = 1\n' > "$D/run.ini"
  cp "$VOL" "$D/win1-r11h.img"
  set --
  while IFS= read -r line; do set -- "$@" "$line"; done < "$SEQ"
  PC_CMD_TIMEOUT=300 python3 tests/interactive/run_pc_multi.py "$TINY386" "$D" "$D/win1-r11h.img" "$OUT/tiny386-$TAG-$MODE.txt" "$@" \
    > "$OUT/tiny386-$TAG-$MODE.stdout" 2>&1 || echo "tiny $MODE $TAG: harness rc!=0"
}
i386() {  # i386 MODE SEQ TAG
  MODE=$1; SEQ=$2; TAG=$3; IN=$OUT/i386vm-$TAG-$MODE-input.txt
  sed -e 's/^[!@]//' "$SEQ" > "$IN"
  timeout 1500 "$I386VM_HARNESS" build/i386vm-r11h-$MODE/qdos386-r11h-$MODE.bin < "$IN" > "$OUT/i386vm-$TAG-$MODE.txt" 2>&1 || true
}
tiny none tests/r11f/seq-regression.txt regression
i386 none tests/r11f/seq-regression.txt regression
tiny pin tests/r11f/seq-pinned.txt pinned
i386 pin tests/r11f/seq-pinned.txt pinned
H1=$(sha256sum "$VOL" | cut -c1-64)
echo "volume before: $H0"; echo "volume after:  $H1"
[ "$H0" = "$H1" ] && echo "VOLUME UNCHANGED" || echo "VOLUME CHANGED"
echo "REGRESSION DONE"
