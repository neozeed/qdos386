#!/bin/sh
# R14: build the R14 guest programs with the SDK compiler: UARTPRB, SERTEST, QLPROBE (build/r14/), TERM and XFER
# (sdk/examples), and QLTerm (scripts/r14-build-qlterm.sh).   usage: sh scripts/r14-build-guest.sh
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd); cd "$ROOT"
: "${QDOS386_TOOLS:?set QDOS386_TOOLS}"
mkdir -p build/r14
Q=sdk/qlcompat
sh sdk/bin/qdos386-gcc -O -o build/r14/uartprb.aout tests/r14/uartprb.c
sh sdk/bin/qdos386-gcc -O -o build/r14/sertest.aout tests/r14/sertest.c
sh sdk/bin/qdos386-gcc -O -I $Q/include -I $Q -D main=qlc_program_main -o build/r14/qlprobe.aout tests/r14/qlprobe.c \
  $Q/qlc_io.c $Q/qlc_win.c $Q/qlc_job.c
make -C sdk/examples TERM.aout XFER.aout
sh scripts/r14-build-qlterm.sh build/r14/qlterm
