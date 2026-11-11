#!/bin/sh
# Apollo 16 Phase 2B: the performance matrix.  Every run is a fresh boot (cold cache), with the background progress
# program R20P on CPU 0 and one workload (R20GS stream, R20GW small set, R20GL large set) on CPU 0, two passes each.
#   usage: sh scripts/a16-p2b-matrix.sh OUTDIR [SMP ...]     (kernels: build/a16/kernels/*.img; volume: build/a16/p2b-vol.img)
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd); cd "$ROOT"
OUT=${1:?usage: OUTDIR [SMP...]}; shift; SMPS=${*:-1 2}
mkdir -p "$OUT"
for smp in $SMPS; do
	for k in r18 r20-2a r20-2b; do
		for w in s w l; do
			tag=$k-smp$smp-$w
			R11M_QKERNEL=build/a16/kernels/$k-vga.img
			[ -f "$R11M_QKERNEL" ] || R11M_QKERNEL=build/a16/kernels/$k-vga.img
			R11M_QKERNEL=build/a16/kernels/$k-vga.img QEMU_EXTRA="-smp $smp" QEMU_CMD_TIMEOUT=900 \
				timeout 1200 sh scripts/r11m-run-qemu.sh build/a16/p2b-vol.img tests/a16/p2b-w$w.seq "$OUT" "$tag" 1100 \
				> /dev/null 2>&1 || true
			echo "done $tag"
		done
	done
done
echo MATRIX-DONE
