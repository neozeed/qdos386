#!/bin/sh
# R11A runtime matrix: build the R11A volume, then run the guest tests on
# Tiny386 (scripted PS/2 keys, tests/interactive/session.py) and on i386VM
# (scripted piped input).  All keystrokes are SCRIPTED; this is not a human session.
#
#   usage: QDOS386_TOOLS=<tree> TINY386=<tiny386_headless> I386VM=<tree with bin/i386vm>
#          FW=<dir with bios.bin, vgabios.bin> [LGTEST=<LGTEST4.aout>] sh scripts/r11a-run-all.sh
# Output: logs/r11a/
set -eu
cd "$(dirname "$0")/.."
: "${QDOS386_TOOLS:?}" "${TINY386:?}" "${FW:?}" "${I386VM_HARNESS:?path to the i386VM-r9 harness binary}"
OUT=logs/r11a
mkdir -p $OUT build/pc-r11b
sh scripts/r11a-volume.sh ${LGTEST:+$LGTEST} | tee $OUT/volume.txt
cp build/fat16/win1-r11b.img build/pc-r11b/win1-r11b.img
# The default (diagnostic) boot image, built by the R9 target from the same tree.
make r9-pc-shell QDOS386_TOOLS="$QDOS386_TOOLS" I386VM="$QDOS386_TOOLS" >/dev/null
cp build/pc-r9/r9shell.img build/pc-r11b/hda.img
cp "$FW/bios.bin" "$FW/vgabios.bin" build/pc-r11b/
printf '[pc]\nbios = bios.bin\nvga_bios = vgabios.bin\nmem_size = 8M\nhda = hda.img\nhdb = win1-r11b.img\n\n[cpu]\nfpu = 1\n' > build/pc-r11b/run.ini
# Tiny386: one boot, the commands in order.
python3 tests/interactive/run_pc_multi.py "$TINY386" build/pc-r11b build/pc-r11b/win1-r11b.img $OUT/tiny386-session.txt \
	"DIR" "RUN CTYPE.AOU" "RUN STRTST.AOU" "RUN FTEST.AOU" "RUN LIBHEL.AOU" "RUN CATFIL.AOU" \
	"RUN MEMSTR.AOU" "RUN HEAPEX.AOU" "RUN CTYPE.AOU" "RUN STRTST.AOU" "RUN FTEST.AOU" \
	"RUN HELLO.AOU" "RUN FPJOB.AOU"
# i386VM: the same sequence on a shell image built with this volume.
make i386vm I386VM="$I386VM" I386D=build/i386vm-r11b I386FLAGS="-DR7_SHELL_MODE -DQDOS_LAYOUT_R9" \
	I386IMG=qdos386-r11b-shell.bin I386WIN=build/fat16/win1-r11b.img IMGENV="QDOS_LAYOUT=r9" >/dev/null
printf 'DIR\nRUN CTYPE.AOU\nRUN STRTST.AOU\nRUN FTEST.AOU\nRUN LIBHEL.AOU\nRUN CATFIL.AOU\nRUN MEMSTR.AOU\nRUN HEAPEX.AOU\nRUN CTYPE.AOU\nRUN STRTST.AOU\nRUN FTEST.AOU\nRUN HELLO.AOU\nRUN FPJOB.AOU\n' \
	| timeout 900 "$I386VM_HARNESS" build/i386vm-r11b/qdos386-r11b-shell.bin > $OUT/i386vm-session.txt 2>&1 || true
echo "tiny386: $(grep -c 'CHECK.*ok$' $OUT/tiny386-session.txt) checks ok, $(grep -c 'PASS$' $OUT/tiny386-session.txt) PASS lines, $(grep -c 'FAIL' $OUT/tiny386-session.txt) FAIL"
echo "i386vm:  $(grep -c 'CHECK.*ok$' $OUT/i386vm-session.txt) checks ok, $(grep -c 'PASS$' $OUT/i386vm-session.txt) PASS lines, $(grep -c 'FAIL' $OUT/i386vm-session.txt) FAIL"
