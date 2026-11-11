#!/bin/sh
# R16B: build the userland tools TOP and CPUSET (tests/r16b/*.c) and protected (CPL3-marked) copies, plus the R16T
# test program (scripts/r16-build-guest.sh).   usage: sh scripts/r16b-build-guest.sh
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd); cd "$ROOT"; : "${QDOS386_TOOLS:?}"
sh scripts/r16-build-guest.sh > /dev/null
B=$QDOS386_TOOLS/bin; L=sdk/lib; I="-Isdk/include"
mkdir -p build/r16b
$B/as -o build/r16b/crt0.o sdk/lib/crt0_libc.s
for p in top cpuset; do
	P=$(echo "$p" | tr a-z A-Z)
	$B/i386-gcc -O $I -c tests/r16b/$p.c -o build/r16b/$p.o
	$B/i386-ld -N -T sdk/ld/slot1.ld -e _start -o build/r16b/$P.aout build/r16b/crt0.o build/r16b/$p.o \
		$L/libc.a $L/libm.a $L/libgcc-qd.a $L/libqdos.a
	python3 tools/r16-protect-mark.py build/r16b/$P.aout build/r16b/${P}P.aout > /dev/null
done
python3 tools/r16-protect-mark.py sdk/examples/BOUNCE.aout build/r16b/BOUNCEP.aout > /dev/null   # marked copy, same code
sha256sum build/r16/R16TP.aout build/r16b/TOPP.aout build/r16b/CPUSETP.aout build/r16b/BOUNCEP.aout
