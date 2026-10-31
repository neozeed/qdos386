#!/bin/sh
# R16: build the CPL3 test program build/r16/R16T.aout (tests/r16/r16t.c + tests/r16/r16regs.s) and a protected
# copy build/r16/R16TP.aout (same bytes, magic bit 24 set).   usage: sh scripts/r16-build-guest.sh
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd); cd "$ROOT"; : "${QDOS386_TOOLS:?}"
B=$QDOS386_TOOLS/bin; L=sdk/lib; I="-Isdk/include"
mkdir -p build/r16
make -C sdk/lib > /dev/null; make -C sdk/libc ../lib/libc.a > /dev/null
make -C sdk/libm ../lib/libm.a > /dev/null; make -C sdk/compiler ../lib/libgcc-qd.a > /dev/null
$B/as -o build/r16/crt0.o sdk/lib/crt0_libc.s
$B/as -o build/r16/r16regs.o tests/r16/r16regs.s
$B/i386-gcc -O $I -c tests/r16/r16t.c -o build/r16/r16t.o
$B/i386-ld -N -T sdk/ld/slot1.ld -e _start -o build/r16/R16T.aout build/r16/crt0.o build/r16/r16t.o build/r16/r16regs.o \
	$L/libc.a $L/libm.a $L/libgcc-qd.a $L/libqdos.a
python3 tools/r16-protect-mark.py build/r16/R16T.aout build/r16/R16TP.aout > /dev/null
sha256sum build/r16/R16T.aout build/r16/R16TP.aout
