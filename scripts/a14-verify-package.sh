#!/bin/sh
# Apollo 14 (R18): verify a FRESH extraction of qdos386-APOLLO14-R18-GRAPHICS.tar.gz using only the package (plus the
# host's QEMU, mtools, fsck.fat, Python 3 + Pillow and the toolchain at QDOS386_TOOLS).
#   usage: sh scripts/a14-verify-package.sh EXTRACTED_PKG_DIR OUTDIR
# 1 manifest  2 clean `make qdos386-r18-pc` = packaged R18 kernels  3 frozen R17/R16B/R16A/KCTX-FIX2 rebuilds identical
# 4 guest tools rebuild identical  5 example volume: no restricted file, fsck  6 short QEMU boot to the shell with the
# PLANAR demo shown in 640x480x16, checked pixel for pixel by tests/a14/oracle.py
set -u
P=$(cd "${1:?}" && pwd); O=$(mkdir -p "${2:?}" && cd "$2" && pwd); : "${QDOS386_TOOLS:?}"
cd "$P"; L=$O/package-verify.txt; : > "$L"; n=0; f=0; export MTOOLS_SKIP_CHECK=1
say() { echo "$*" | tee -a "$L"; }
ok() { n=$((n+1)); if [ "$1" = 0 ]; then say "PASS $2"; else f=$((f+1)); say "FAIL $2"; fi; }
sha256sum -c --quiet SHA256SUMS > "$O/manifest.txt" 2>&1; ok $? "manifest: $(wc -l < SHA256SUMS) files match SHA256SUMS"
[ ! -e build ]; ok $? "fresh tree: no build/ directory before the rebuild"
make qdos386-r18-pc > "$O/make-r18.log" 2>&1; ok $? "make qdos386-r18-pc"
for p in build/qdos386-r18/pc-vga/qdos386-r18-vga.img:kernels/qdos386-r18-vga.img build/qdos386-r18/pc/qdos386-r18.img:kernels/qdos386-r18.img; do
	cmp -s "${p%%:*}" "${p#*:}"; ok $? "rebuild identical: ${p#*:} ($(sha256sum < "${p#*:}" | cut -c1-16))"
done
make qdos386-r17-pc qdos386-r16b-pc qdos386-smp-pc qdos386-pc > "$O/make-frozen.log" 2>&1; ok $? "make qdos386-r17-pc qdos386-r16b-pc qdos386-smp-pc qdos386-pc"
for p in build/qdos386-r17/pc-vga/qdos386-r17-vga.img:kernels/r17-unchanged/qdos386-r17-vga.img \
         build/qdos386-r16b/pc-vga/qdos386-r16b-vga.img:kernels/r16b-unchanged/qdos386-r16b-vga.img \
         build/qdos386-smp/pc-vga/qdos386-smp-vga.img:kernels/r16a-unchanged/qdos386-smp-vga.img \
         build/qdos386/pc-vga/qdos386-vga.img:kernels/default-unchanged/qdos386-vga.img \
         build/qdos386/pc/qdos386.img:kernels/default-unchanged/qdos386.img; do
	cmp -s "${p%%:*}" "${p#*:}"; ok $? "frozen rebuild identical: ${p#*:} ($(sha256sum < "${p#*:}" | cut -c1-8))"
done
sh scripts/a14-build-guest.sh > "$O/guest.log" 2>&1; ok $? "guest tools build"
for g in a14/PLANARP a14/VIEWP a13/MEMINFOP a13/RAMCHKP a13/SMPTP a13/TOP17P a13/CPUSET17P r16/R16TP r16b/BOUNCEP; do
	cmp -s build/$g.aout guest/${g#*/}.aout; ok $? "guest rebuild identical: ${g#*/}.aout"
done
mdir -/ -b -i examples/system-r18-superfloppy.img :: > "$O/vol.txt" 2>&1
! grep -i -q -E '/BASIC\.AOU$|BASTEST|INFOCOM|ZORK|\.Z[3-8]$|STORY|ELITE' "$O/vol.txt" && fsck.fat -n examples/system-r18-superfloppy.img > /dev/null 2>&1
ok $? "example volume: no SuperBASIC / Infocom / Elite file ($(wc -l < "$O/vol.txt") entries), fsck clean"
cp examples/system-r18-superfloppy.img "$O/smoke.img"
printf '%s\n' "%sleep 3" "PLANAR CAPS" "!START PLANAR CARD 256 256 VGA12 8 /TMP/SMOKE.RAW /NEW" "%sleep 5" "%screendump smoke" \
  "%sleep 6" "%key alt-f1" "%sleep 1" "LEAK" "ECHO smoke-done" > "$O/smoke.seq"
R12_QKERNEL=kernels/qdos386-r18-vga.img QEMU_MEM=64 QEMU_EXTRA="-smp 2 -accel tcg,thread=multi" QEMU_CMD_TIMEOUT=120 \
  sh scripts/r12-run-qemu.sh "$O/smoke.img" "$O/smoke.seq" "$O" smoke 600 > /dev/null 2>&1
S=$O/qemu-smoke.txt
grep -q "SHELL: running" "$S" && grep -q "smoke-done" "$S"; ok $? "smoke: boots to the shell and runs the sequence"
grep -q "PLANAR CAPS caps=.* vga12=yes" "$S"; ok $? "smoke: SURF_CAP_VGA12 reported"
mcopy -o -i "$O/qemu-smoke.final.img" ::/TMP/SMOKE.RAW "$O/SMOKE.RAW" > /dev/null 2>&1
python3 tests/a14/oracle.py "$O/SMOKE.RAW" "$O/smoke.ppm" > "$O/oracle.txt" 2>&1; ok $? "smoke: PLANAR 256x256 in 640x480x16 pixel-exact ($(cat "$O/oracle.txt" | cut -c1-90))"
! grep -q PGFAULT "$S"; ok $? "smoke: no PGFAULT"
say "$((n - f)) of $n checks passed"
[ "$f" = 0 ]
