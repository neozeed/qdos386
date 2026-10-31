#!/bin/sh
# R14A: build the ANSIT fixture (tests/r14a/ansit.c) with the same objects and libraries as Hack's Makefile.qdos386:
# crt0_libc, c68vt.o, libc68.a before libc.a.   usage: sh scripts/r14a-build-ansit.sh [OUT.aout]
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd); cd "$ROOT"
: "${QDOS386_TOOLS:?set QDOS386_TOOLS}"
OUT=${1:-build/r14a/ANSIT.aout}; T=build/r14a/ansit-obj; rm -rf "$T"; mkdir -p "$T" "$(dirname "$OUT")"
make -C sdk/lib >/dev/null; make -C sdk/libc ../lib/libc68.a >/dev/null
B=$QDOS386_TOOLS/bin; I="-Isdk/qlcompat/include -Isdk/include -Iport/include"
$B/as -o $T/crt0.o sdk/lib/crt0_libc.s
$B/i386-gcc -O $I -c tests/r14a/ansit.c -o $T/ansit.o
$B/i386-gcc -O $I -c sdk/qlcompat/c68/c68vt.c -o $T/c68vt.o
L=sdk/lib
$B/i386-ld -N -T sdk/ld/slot1.ld -e _start -o "$OUT" $T/crt0.o $T/ansit.o $T/c68vt.o $L/libc68.a $L/libm.a $L/libc.a \
  $L/libgcc-qd.a $L/libqdos.a
ls -la "$OUT"
