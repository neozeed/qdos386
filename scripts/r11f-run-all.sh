#!/bin/sh
# R11F runtime matrix.  Every key is SCRIPTED (Tiny386 via tests/interactive/run_pc_multi.py,
# i386VM via a piped input file).  Sequences are in tests/r11f/.  Expected values are never
# altered to make a run pass; results are logged in logs/r11f/.
#   regression  : the R11E sequence on the no-clock image, plus the new no-clock checks
#   pinned      : the fixed clock (Phoon default, the 24-case date battery, 12 repeated runs)
#   advancing   : the advancing clock (TMABII on i386VM, TMABIA on Tiny386)
#   control     : TMABII (must PASS) then TMABIW (test-only control volume and image), a deliberately wrong-rate build assuming 110 ticks/s (must FAIL), on the i386VM advancing image.
#                 The natural i386VM/PC rate difference (0.015%) is below what the advance check
#                 resolves, so it is not used as a control.  TMABIW assumes 10% more ticks per
#                 second; the advance check must detect that (FAIL).  Expected: TMABII PASS, TMABIW FAIL.
#
#   usage: QDOS386_TOOLS=<tree> I386VM=<same tree> TINY386=<tiny386_headless> FW=<dir with bios.bin,
#          vgabios.bin> I386VM_HARNESS=<i386vm-r9 binary> [LGTEST=<LGTEST4.aout>] [ROUNDS=3]
#          sh scripts/r11f-run-all.sh
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd)
cd "$ROOT"
: "${QDOS386_TOOLS:?}" "${I386VM:?}" "${TINY386:?}" "${FW:?}" "${I386VM_HARNESS:?}"
export QDOS386_TOOLS I386VM
ROUNDS=${ROUNDS:-3}
OUT=logs/r11f
VOL=build/fat16/win1-r11f.img
mkdir -p $OUT

sh scripts/r11f-volume.sh ${LGTEST:+$LGTEST} > $OUT/volume.txt
sh scripts/r11f-images.sh > $OUT/images.txt

# Tiny386 work directories: one per clock mode, each with its own kernel image as hda.
for MODE in none pin adv; do
	D=build/pc-r11f-$MODE-run
	mkdir -p $D
	cp "$FW/bios.bin" "$FW/vgabios.bin" $D/
	cp build/pc-r11f-$MODE/r11f-$MODE.img $D/hda.img
	printf '[pc]\nbios = bios.bin\nvga_bios = vgabios.bin\nmem_size = 8M\nhda = hda.img\nhdb = win1-r11f.img\n\n[cpu]\nfpu = 1\n' > $D/run.ini
done

# i386 input: the same lines with the Tiny386 prefixes (! and @) removed.
i386_input() {
	while IFS= read -r line; do
		case "$line" in
		!*|@*) printf '%s\n' "${line#?}" ;;
		*) printf '%s\n' "$line" ;;
		esac
	done < "$1" > "$2"
}

hash_vol() { sha256sum "$VOL" | cut -c1-64; }

run_tiny() {  # run_tiny MODE SEQFILE TAG ROUND
	m=$1; seq=$2; tag=$3; round=$4
	set --                                  # the sequence lines become the commands
	while IFS= read -r line; do
		set -- "$@" "$line"
	done < "$seq"
	PC_CMD_TIMEOUT=300 python3 tests/interactive/run_pc_multi.py "$TINY386" build/pc-r11f-$m-run "$VOL" \
		$OUT/tiny386-$tag-round$round.txt "$@"
}
run_i386() {  # run_i386 MODE IMAGE SEQFILE TAG ROUND
	i386_input "$3" $OUT/i386vm-$4-input.txt
	timeout 1500 "$I386VM_HARNESS" "$2" < $OUT/i386vm-$4-input.txt > $OUT/i386vm-$4-round$5.txt 2>&1 || true
}

HASH0=$(hash_vol)
echo "FAT16 volume sha256 before: $HASH0"

r=1
while [ $r -le $ROUNDS ]; do
	run_tiny none tests/r11f/seq-regression.txt regression $r
	run_i386 none build/i386vm-r11f-none/qdos386-r11f-none.bin tests/r11f/seq-regression.txt regression $r
	run_tiny pin tests/r11f/seq-pinned.txt pinned $r
	run_i386 pin build/i386vm-r11f-pin/qdos386-r11f-pin.bin tests/r11f/seq-pinned.txt pinned $r
	run_tiny adv tests/r11f/seq-adv-pc.txt advancing $r
	run_i386 adv build/i386vm-r11f-adv/qdos386-r11f-adv.bin tests/r11f/seq-adv-i386.txt advancing $r
	r=$((r + 1))
done
# Control (one round): the i386VM advancing image with the PC-rate test binary.
run_i386 control build/i386vm-r11f-control/qdos386-r11f-control.bin tests/r11f/seq-control-i386-rate.txt control 1

HASH1=$(hash_vol)
echo "FAT16 volume sha256 after:  $HASH1"
if [ "$HASH0" = "$HASH1" ]; then echo "FAT16 volume unchanged by the matrix"; else echo "FAT16 VOLUME CHANGED"; fi

for f in $OUT/tiny386-*-round*.txt $OUT/i386vm-*-round*.txt; do
	echo "$(basename $f): $(grep -c '^CHECK.*: ok$' $f) checks ok, $(grep -c '^CHECK.*: FAIL$' $f) checks FAIL, $(grep -c 'PASS$' $f) PASS banners, $(grep -c '^QDOS/386 R11F .* FAIL$' $f) FAIL banners"
done
