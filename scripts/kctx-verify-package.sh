#!/bin/sh
# KCTX-FIX1: verify a FRESH extraction of qdos386-KCTX-FIX1.tar.gz.
#   usage: sh scripts/kctx-verify-package.sh EXTRACTED_PKG_DIR OUTDIR      (run from any tree; uses only the package)
# 1 manifest (SHA256SUMS); 2 rebuild the KFIX1 and R14 kernels and the probes from the package source and compare with
# the packaged files; 3 smoke on the packaged kfix1-vga.img + packaged volume (supplied controls, ALIGNT residues 0-3
# in spin/trap/yield); 4 A/B control: the packaged unchanged r14-vga.img must still #DF on spin_misaligned.
set -u
P=$(cd "${1:?}" && pwd); O=$(mkdir -p "${2:?}" && cd "$2" && pwd); : "${QDOS386_TOOLS:?}"
cd "$P"; L=$O/package-verify.txt; : > "$L"; n=0; f=0
say() { echo "$*" | tee -a "$L"; }
ok() { n=$((n+1)); if [ "$1" = 0 ]; then say "PASS $2"; else f=$((f+1)); say "FAIL $2"; fi; }
sha256sum -c --quiet SHA256SUMS > "$O/manifest.txt" 2>&1; ok $? "manifest: $(wc -l < SHA256SUMS) files match SHA256SUMS"
make kfix1-pc r14-pc > "$O/make.log" 2>&1; ok $? "make kfix1-pc r14-pc"
sh tests/kctx/build.sh build/kctx > "$O/probe-build.log" 2>&1; ok $? "tests/kctx/build.sh"
for p in build/kfix1/pc-vga/kfix1-vga.img:kernels/kfix1-vga.img build/kfix1/pc/kfix1.img:kernels/kfix1.img \
         build/r14/pc-vga/r14-vga.img:kernels/r14-vga.img build/r14/pc/r14.img:kernels/r14.img \
         build/kctx/ALIGNT.aout:probe/ALIGNT.aout build/kctx/spin_misaligned.aout:probe/spin_misaligned.aout \
         build/kctx/spin_aligned.aout:probe/spin_aligned.aout; do
	a=${p%%:*}; b=${p#*:}; cmp -s "$a" "$b"; ok $? "rebuild identical: $b ($(sha256sum < "$b" | cut -c1-16))"
done
run() {   # KERNEL TAG CMD...
	K=$1; T=$2; shift 2; cp volumes/kfix1-system.img "$O/$T.vol.img"
	: > "$O/$T.seq"; for c in LEAK "$@" LEAK; do echo "$c" >> "$O/$T.seq"; done
	MTOOLS_SKIP_CHECK=1 QEMU_NO_SHADOW=1 R12_QKERNEL=$K QEMU_MON_PORT=4991 QEMU_CMD_TIMEOUT=90 \
	  sh scripts/r12-run-qemu.sh "$O/$T.vol.img" "$O/$T.seq" "$O" "$T" 900 > /dev/null 2>&1
	rm -f "$O/$T.vol.img" "$O/qemu-$T.final.img"
}
run kernels/kfix1-vga.img smoke SPALI SPMIS "ALIGNT 0 1000000000 0" "ALIGNT 1 1000000000 0" "ALIGNT 2 200000 1" \
    "ALIGNT 3 300 2" "ALIGNT 3 1000000000 0" "ALIGNT 1 200000 1" "ALIGNT 2 300 2"
python3 tests/kctx/judge_probe.py "$O/qemu-smoke.txt" > "$O/smoke-judge.txt" 2>&1; ok $? "smoke on packaged kfix1-vga.img: $(tail -1 "$O/smoke-judge.txt")"
sed 's/^/    /' "$O/smoke-judge.txt" >> "$L"
run kernels/r14-vga.img control SPALI SPMIS
grep -aq "exit status 0" "$O/qemu-control.txt" && grep -aq "PGFAULT: #DF double fault" "$O/qemu-control.txt"
ok $? "A/B control on packaged unchanged r14-vga.img: spin_aligned exits 0, spin_misaligned -> #DF (defect still in R14)"
say "PACKAGE VERIFY: $((n-f))/$n PASS"
[ $f = 0 ]
