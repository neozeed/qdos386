#!/bin/sh
# R15: verify a FRESH extraction of qdos386-R15-AMP0.tar.gz with only the package (and the host's QEMU, mtools,
# dosfstools, objdump and the toolchain at QDOS386_TOOLS).
#   usage: sh scripts/r15-verify-package.sh EXTRACTED_PKG_DIR OUTDIR
set -u
P=$(cd "${1:?}" && pwd); O=$(mkdir -p "${2:?}" && cd "$2" && pwd); : "${QDOS386_TOOLS:?}"
cd "$P"; L=$O/package-verify.txt; : > "$L"; n=0; f=0; export MTOOLS_SKIP_CHECK=1 QEMU_NO_SHADOW=1
say() { echo "$*" | tee -a "$L"; }
ok() { n=$((n+1)); if [ "$1" = 0 ]; then say "PASS $2"; else f=$((f+1)); say "FAIL $2"; fi; }
same() { cmp -s "$1" "$2"; ok $? "rebuild identical: $2 ($(sha256sum < "$2" | cut -c1-16))"; }
sha256sum -c --quiet SHA256SUMS > "$O/manifest.txt" 2>&1; ok $? "manifest: $(wc -l < SHA256SUMS) files match SHA256SUMS"
make > "$O/make.log" 2>&1; ok $? "make (default goal: the ordinary UP kernels)"
same build/qdos386/pc-vga/qdos386-vga.img kernels/qdos386-vga.img
same build/qdos386/pc/qdos386.img kernels/qdos386.img
[ "$(sha256sum < kernels/qdos386-vga.img | cut -c1-16)" = f8e5d5e1f7e5c698 ] && [ "$(sha256sum < kernels/qdos386.img | cut -c1-16)" = 7c67597862f24de2 ]
ok $? "the ordinary kernels are the frozen KCTX-FIX2 kernels (f8e5d5e1..., 7c675978...)"
make qdos386-amp-pc > "$O/make-amp.log" 2>&1; ok $? "make qdos386-amp-pc"
same build/qdos386-amp/pc-vga/qdos386-amp-vga.img kernels/qdos386-amp-vga.img
same build/qdos386-amp/pc/qdos386-amp.img kernels/qdos386-amp.img
for v in "nosipi -DQDOS_AMP_TEST_NOSIPI" "late -DQDOS_AMP_TEST_LATE=400000000u" "burn -DQDOS_AMP_TEST_BURN"; do
	set -- $v; make qdos386-amp-variant AMPV=$1 AMP_EXTRA="$2" > "$O/make-$1.log" 2>&1
	same build/qdos386-amp-$1/pc-vga/qdos386-amp-vga.img kernels/test-only/qdos386-amp-$1-vga.img
done
sh scripts/r15-build-guest.sh > "$O/guest.log" 2>&1; ok $? "scripts/r15-build-guest.sh"
same build/r15/AMPT.aout guest/AMPT.aout
same build/r15/WORKER.aout guest/WORKER.aout
same build/r14a/SPINM.aout guest/SPINM.aout
python3 tests/r15/check-ap-objects.py build/qdos386-amp/pc-vga > "$O/ap-static.txt" 2>&1; ok $? "worker static check: $(tail -1 "$O/ap-static.txt")"
mdir -/ -b -i volumes/r15-public.img :: > "$O/public-vol.txt" 2>&1
! grep -i -q -E '/BASIC\.AOU$|BASTEST|INFOCOM|ZORK|\.Z[3-8]$|STORY' "$O/public-vol.txt" && grep -q 'AMPT.AOU' "$O/public-vol.txt" && fsck.fat -n volumes/r15-public.img > /dev/null 2>&1
ok $? "public volume: AMPT/WORKER present, no SuperBASIC / Infocom / story file, fsck clean"
run() {   # TAG SMP KERNEL CMDS...
	tag=$1; smp=$2; k=$3; shift 3; cp volumes/r15-public.img "$O/$tag.img"; : > "$O/$tag.seq"
	for c in "$@"; do echo "$c" >> "$O/$tag.seq"; done
	QEMU_MEM=32 QEMU_EXTRA="-smp $smp" R12_QKERNEL=$k QEMU_MON_PORT=4997 QEMU_CMD_TIMEOUT=300 \
	  sh scripts/r12-run-qemu.sh "$O/$tag.img" "$O/$tag.seq" "$O" "$tag" 1200 > /dev/null 2>&1
	rm -f "$O/$tag.img" "$O/qemu-$tag.final.img"
}
run smp2 2 kernels/qdos386-amp-vga.img LEAK "AMPT INFO" "AMPT RUN 300 11" "AMPT PROOF 20000000 3" "WORKER VECTOR 9" "WORKER FILE /ETC/STARTUP.CMD" LEAK
T=$(tr -d '\n' < "$O/qemu-smp2.txt")
echo "$T" | grep -q "worker CPU LAPIC id 1 READY" && echo "$T" | grep -q "n=300 ok=300 bad=0" && echo "$T" | grep -q "AMPT PROOF: 3 passed, 0 failed" \
  && echo "$T" | grep -q "result verified" && ! grep -q PGFAULT "$O/qemu-smp2.txt"
ok $? "QEMU -smp 2 smoke (packaged AMP kernel + public volume): worker READY, 300 checked requests, PROOF 3/3, WORKER vector verified"
python3 tests/r15/oracle.py "$O/qemu-smp2.txt" > "$O/oracle.txt" 2>&1; ok $? "host oracle: $(tail -1 "$O/oracle.txt")"
run smp1 1 kernels/qdos386-amp-vga.img "AMPT INFO" "WORKER VECTOR 9" "DIR /BIN" LEAK
T=$(tr -d '\n' < "$O/qemu-smp1.txt")
echo "$T" | grep -q "only one enabled processor" && echo "$T" | grep -q "no worker CPU service" && echo "$T" | grep -q "SH.AOU" && ! grep -q PGFAULT "$O/qemu-smp1.txt"
ok $? "QEMU -smp 1 smoke: service unavailable (one processor), WORKER exits 3, shell normal"
mkdir -p build/pc-r11f-none-run; cp kernels/bios.bin kernels/vgabios.bin build/pc-r11f-none-run/
cp volumes/r15-public.img "$O/t.img"
printf '%s\n' "RUN LEAK" "AMPT INFO" "DIR /BIN" "RUN LEAK" "!RUN SHUTDOWN" > "$O/t.seq"
TINY386=$P/emulator/bin/tiny386_headless sh scripts/fix1-tiny-run.sh "$O/t.img" "$O/t.seq" "$O" t wait kernels/qdos386-amp.img > "$O/tiny.txt" 2>&1
grep -q "service UNAVAILABLE" "$O/tiny386-t.txt" && grep -q "SH.AOU" "$O/tiny386-t.txt" && grep -q "exited=1" "$O/tiny386-t.txt.end" && grep -q "IMAGE PASS" "$O/tiny.txt"
ok $? "Tiny386 smoke (packaged emulator, AMP headless kernel): service unavailable, shell, shutdown, clean image"
rm -f "$O"/*.img
say "PACKAGE VERIFY: $((n-f))/$n PASS"
[ $f = 0 ]
