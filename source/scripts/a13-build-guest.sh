#!/bin/sh
# Apollo 13 (R17): build the R16B guest tools (scripts/r16b-build-guest.sh) plus the R17 tools MEMINFO, RAMCHK
# (tests/a13/*.c) and their protected (CPL3-marked) copies.   usage: sh scripts/a13-build-guest.sh
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd); cd "$ROOT"; : "${QDOS386_TOOLS:?}"
if [ ! -f sdk/examples/BOUNCE.aout ]; then                 # untracked build output: a fresh tree builds it (and its libraries)
	make -C sdk/lib > /dev/null; make -C sdk/libc ../lib/libc.a > /dev/null
	make -C sdk/libm ../lib/libm.a > /dev/null; make -C sdk/compiler ../lib/libgcc-qd.a > /dev/null
	make -C sdk/examples BOUNCE.aout > /dev/null
fi
sh scripts/r16b-build-guest.sh > /dev/null
B=$QDOS386_TOOLS/bin; L=sdk/lib; I="-Isdk/include"
mkdir -p build/a13
$B/as -o build/a13/crt0.o sdk/lib/crt0_libc.s
for p in meminfo ramchk smpt top17 cpuset17; do
	P=$(echo "$p" | tr a-z A-Z)
	$B/i386-gcc -O $I -c tests/a13/$p.c -o build/a13/$p.o
	X=""; [ "$p" = smpt ] && X=build/r16/r16regs.o
	$B/i386-ld -N -T sdk/ld/slot1.ld -e _start -o build/a13/$P.aout build/a13/crt0.o build/a13/$p.o $X \
		$L/libc.a $L/libm.a $L/libgcc-qd.a $L/libqdos.a
	python3 tools/r16-protect-mark.py build/a13/$P.aout build/a13/${P}P.aout > /dev/null
done
sha256sum build/a13/*P.aout
