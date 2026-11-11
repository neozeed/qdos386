#!/bin/sh
# Apollo 14 (R18): the R17 guest tools (scripts/a13-build-guest.sh) plus PLANAR (tests/a14/planar.c), the public
# 640x480x16 presentation demo/test card, and its protected (CPL3-marked) copy.   usage: sh scripts/a14-build-guest.sh
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd); cd "$ROOT"; : "${QDOS386_TOOLS:?}"
sh scripts/a13-build-guest.sh > /dev/null
B=$QDOS386_TOOLS/bin; L=sdk/lib; I="-Isdk/include"
mkdir -p build/a14
for p in planar view; do
	P=$(echo "$p" | tr a-z A-Z)
	$B/i386-gcc -O $I -c tests/a14/$p.c -o build/a14/$p.o
	$B/i386-ld -N -T sdk/ld/slot1.ld -e _start -o build/a14/$P.aout build/a13/crt0.o build/a14/$p.o \
		$L/libc.a $L/libm.a $L/libgcc-qd.a $L/libqdos.a
	python3 tools/r16-protect-mark.py build/a14/$P.aout build/a14/${P}P.aout > /dev/null
done
sha256sum build/a14/*P.aout
