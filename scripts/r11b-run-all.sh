#!/bin/sh
# R11B runtime matrix: build the R11B volume, then run the guest tests on
# Tiny386 (scripted PS/2 keys, tests/interactive/run_pc_multi.py) and on i386VM
# (scripted piped input), ROUNDS times each (default 3).  Every key is SCRIPTED;
# this is not a human-operated session.
#
#   usage: QDOS386_TOOLS=<tree> TINY386=<tiny386_headless> I386VM=<tree with bin/i386vm>
#          FW=<dir with bios.bin, vgabios.bin> I386VM_HARNESS=<i386vm-r9 binary>
#          [LGTEST=<LGTEST4.aout>] [ROUNDS=3] sh scripts/r11b-run-all.sh
# Output: logs/r11b/
set -eu
cd "$(dirname "$0")/.."
: "${QDOS386_TOOLS:?}" "${TINY386:?}" "${FW:?}" "${I386VM:?}" "${I386VM_HARNESS:?}"
ROUNDS=${ROUNDS:-3}
OUT=logs/r11b
mkdir -p $OUT build/pc-r11b
sh scripts/r11a-volume.sh ${LGTEST:+$LGTEST} | tee $OUT/volume.txt
cp build/fat16/win1-r11b.img build/pc-r11b/win1-r11b.img
make r9-pc-shell QDOS386_TOOLS="$QDOS386_TOOLS" I386VM="$QDOS386_TOOLS" >/dev/null
cp build/pc-r9/r9shell.img build/pc-r11b/hda.img
cp "$FW/bios.bin" "$FW/vgabios.bin" build/pc-r11b/
printf '[pc]\nbios = bios.bin\nvga_bios = vgabios.bin\nmem_size = 8M\nhda = hda.img\nhdb = win1-r11b.img\n\n[cpu]\nfpu = 1\n' > build/pc-r11b/run.ini

# The command sequence.  CONSOL reads the two lines that follow its RUN: they
# are typed into the program's console input, not run as shell commands.
# Prefixes: "!" = no prompt wait, "@" = short prompt wait (run_pc_multi.py).
set -- "DIR" "RUN STREAM.AOU" "RUN FORMAT.AOU" "RUN LIBQDO.AOU" "RUN REPEAT.AOU" \
	"RUN REPEAT.AOU" "!RUN CONSOL.AOU" "!abc" "@hello world" \
	"RUN EXITFL.AOU" "RUN CTYPE.AOU" "RUN STRTST.AOU" "RUN FTEST.AOU" "RUN LIBHEL.AOU" "RUN CATFIL.AOU" \
	"RUN MEMSTR.AOU" "RUN HEAPEX.AOU" "RUN HELLO.AOU" "RUN FPJOB.AOU"

# i386VM input: the same sequence, one line each, prefixes removed.
I386_INPUT=$OUT/i386vm-input.txt
for c in "$@"; do
	case "$c" in
	!*|@*) printf '%s\n' "${c#?}" ;;
	*) printf '%s\n' "$c" ;;
	esac
done > $I386_INPUT

make i386vm I386VM="$I386VM" I386D=build/i386vm-r11b I386FLAGS="-DR7_SHELL_MODE -DQDOS_LAYOUT_R9" \
	I386IMG=qdos386-r11b-shell.bin I386WIN=build/fat16/win1-r11b.img IMGENV="QDOS_LAYOUT=r9" >/dev/null

# FAT16 must be unchanged by the runs (no writes exist; this proves it).
HASH_BEFORE=$(sha256sum build/pc-r11b/win1-r11b.img | cut -c1-64)
r=1
while [ $r -le $ROUNDS ]; do
	PC_CMD_TIMEOUT=120 python3 tests/interactive/run_pc_multi.py "$TINY386" build/pc-r11b build/pc-r11b/win1-r11b.img \
		$OUT/tiny386-round$r.txt "$@"
	timeout 1200 "$I386VM_HARNESS" build/i386vm-r11b/qdos386-r11b-shell.bin < $I386_INPUT \
		> $OUT/i386vm-round$r.txt 2>&1 || true
	r=$((r + 1))
done

HASH_AFTER=$(sha256sum build/pc-r11b/win1-r11b.img | cut -c1-64)
echo "FAT16 volume sha256 before: $HASH_BEFORE"
echo "FAT16 volume sha256 after:  $HASH_AFTER"
[ "$HASH_BEFORE" = "$HASH_AFTER" ] && echo "FAT16 volume unchanged by the matrix" || echo "FAT16 VOLUME CHANGED"

for f in $OUT/tiny386-round*.txt $OUT/i386vm-round*.txt; do
	echo "$(basename $f): $(grep -c 'CHECK.*ok$' $f) checks ok, $(grep -c 'PASS$' $f) PASS lines, $(grep -c 'FAIL' $f) FAIL"
done
