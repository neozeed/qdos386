#!/bin/sh
# R15: the guest programs of the AMP0 milestone: build/r15/AMPT.aout (test driver, tests/r15/ampt.c) and
# build/r15/WORKER.aout (demo, sdk/examples/r15/worker.c).   usage: sh scripts/r15-build-guest.sh
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd); cd "$ROOT"; : "${QDOS386_TOOLS:?}"
mkdir -p build/r15 build/r14a
make -C sdk/lib > /dev/null; make -C sdk/libc ../lib/libc.a > /dev/null; make -C sdk/libm ../lib/libm.a > /dev/null
make -C sdk/compiler ../lib/libgcc-qd.a > /dev/null
sh sdk/bin/qdos386-gcc -O -o build/r15/AMPT.aout tests/r15/ampt.c > /dev/null
sh sdk/bin/qdos386-gcc -O -o build/r15/WORKER.aout sdk/examples/r15/worker.c > /dev/null
[ -f build/r14a/SPINM.aout ] || sh sdk/bin/qdos386-gcc -O -o build/r14a/SPINM.aout tests/r14a/spinm.c > /dev/null
sha256sum build/r15/AMPT.aout build/r15/WORKER.aout
