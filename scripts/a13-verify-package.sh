#!/bin/sh
# Apollo 13 (R17): verify a FRESH extraction of qdos386-APOLLO13-R17-MEM-MULTICORE.tar.gz, using only the package
# (plus the host's QEMU, mtools, fsck.fat and the toolchain at QDOS386_TOOLS).
#   usage: sh scripts/a13-verify-package.sh EXTRACTED_PKG_DIR OUTDIR
# Checks, in order:
#   1. the manifest matches;
#   2. a clean `make qdos386-r17-pc` rebuilds the packaged R17 kernels bit-for-bit;
#   3. the frozen R16B, R16A and KCTX-FIX2 targets rebuild their packaged kernels bit-for-bit;
#   4. the guest tools rebuild identically;
#   5. the example volume holds no restricted file and passes fsck;
#   6. a short QEMU smoke (packaged R17 kernel, -smp 4, -m 64, the example volume):
#      MEMINFO, SMPT INFO/WORK/MIG, TOP, LEAK.
set -u
P=$(cd "${1:?}" && pwd); O=$(mkdir -p "${2:?}" && cd "$2" && pwd); : "${QDOS386_TOOLS:?}"
cd "$P"; L=$O/package-verify.txt; : > "$L"; n=0; f=0; export MTOOLS_SKIP_CHECK=1
say() { echo "$*" | tee -a "$L"; }
ok() { n=$((n+1)); if [ "$1" = 0 ]; then say "PASS $2"; else f=$((f+1)); say "FAIL $2"; fi; }
sha256sum -c --quiet SHA256SUMS > "$O/manifest.txt" 2>&1; ok $? "manifest: $(wc -l < SHA256SUMS) files match SHA256SUMS"
[ ! -e build ]; ok $? "fresh tree: no build/ directory before the rebuild"
make qdos386-r17-pc > "$O/make-r17.log" 2>&1; ok $? "make qdos386-r17-pc"
for p in build/qdos386-r17/pc-vga/qdos386-r17-vga.img:kernels/qdos386-r17-vga.img build/qdos386-r17/pc/qdos386-r17.img:kernels/qdos386-r17.img; do
	cmp -s "${p%%:*}" "${p#*:}"; ok $? "rebuild identical: ${p#*:} ($(sha256sum < "${p#*:}" | cut -c1-16))"
done
make qdos386-r16b-pc qdos386-smp-pc qdos386-pc > "$O/make-frozen.log" 2>&1; ok $? "make qdos386-r16b-pc qdos386-smp-pc qdos386-pc"
for p in build/qdos386-r16b/pc-vga/qdos386-r16b-vga.img:kernels/r16b-unchanged/qdos386-r16b-vga.img \
         build/qdos386-smp/pc-vga/qdos386-smp-vga.img:kernels/r16a-unchanged/qdos386-smp-vga.img \
         build/qdos386/pc-vga/qdos386-vga.img:kernels/default-unchanged/qdos386-vga.img \
         build/qdos386/pc/qdos386.img:kernels/default-unchanged/qdos386.img; do
	cmp -s "${p%%:*}" "${p#*:}"; ok $? "frozen rebuild identical: ${p#*:} ($(sha256sum < "${p#*:}" | cut -c1-8))"
done
sh scripts/a13-build-guest.sh > "$O/guest.log" 2>&1; ok $? "guest tools build"
for g in MEMINFOP RAMCHKP SMPTP TOP17P CPUSET17P; do cmp -s build/a13/$g.aout guest/$g.aout; ok $? "guest rebuild identical: $g.aout"; done
cmp -s build/r16/R16TP.aout guest/R16TP.aout; ok $? "guest rebuild identical: R16TP.aout"
cmp -s build/r16b/BOUNCEP.aout guest/BOUNCEP.aout; ok $? "guest rebuild identical: BOUNCEP.aout"
mdir -/ -b -i examples/system-r17-superfloppy.img :: > "$O/vol.txt" 2>&1
! grep -i -q -E '/BASIC\.AOU$|BASTEST|INFOCOM|ZORK|\.Z[3-8]$|STORY|ELITE' "$O/vol.txt" && fsck.fat -n examples/system-r17-superfloppy.img > /dev/null 2>&1
ok $? "example volume: no SuperBASIC / Infocom / Elite file ($(wc -l < "$O/vol.txt") entries), fsck clean"
cp examples/system-r17-superfloppy.img "$O/smoke.img"
printf '%s\n' "%sleep 3" MEMINFO "SMPT INFO" "SMPT WORK 40" "SMPT MIG 20 3" "TOP 1 1" LEAK > "$O/smoke.seq"
R12_QKERNEL=kernels/qdos386-r17-vga.img QEMU_MEM=64 QEMU_EXTRA="-smp 4 -accel tcg,thread=multi" QEMU_CMD_TIMEOUT=300 \
  sh scripts/r12-run-qemu.sh "$O/smoke.img" "$O/smoke.seq" "$O" smoke 900 > /dev/null 2>&1
S=$O/qemu-smoke.txt
python3 -I tests/a13/joinlines.py "$S" MEMMAP: PAGING: SMP: MEMINFO 'SMPT WORK' 'SMPT MIG' 'SMPT INFO' boot LEAK PGFAULT > "$O/smoke-summary.txt" 2>&1
grep -q '^SMP: 4 of 4 CPU(s) online' "$O/smoke-summary.txt"; ok $? "smoke: 4 of 4 CPUs online"
grep -q '^PAGING: R17 .*(E820), identity 0..64 MiB' "$O/smoke-summary.txt"; ok $? "smoke: E820 map, identity 0..64 MiB"
grep -q '^MEMINFO pool=.*check=OK' "$O/smoke-summary.txt"; ok $? "smoke: MEMINFO pool check OK"
grep -q 'SMPT WORK cpus=4 .*-> PASS' "$O/smoke-summary.txt"; ok $? "smoke: SMPT WORK on 4 CPUs PASS"
grep -q 'SMPT MIG cpus=4 .*badrel=0 -> MATCH' "$O/smoke-summary.txt"; ok $? "smoke: SMPT MIG MATCH, badrel 0"
! grep -q PGFAULT "$S"; ok $? "smoke: no PGFAULT"
sed 's/^/    /' "$O/smoke-summary.txt" >> "$L"
say "$((n - f)) of $n checks passed"
[ "$f" = 0 ]
