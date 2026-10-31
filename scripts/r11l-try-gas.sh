#!/bin/sh
# R11L Phase 0/Stage 2: compile every GAS 1.38 i386 source for the guest and report failures.
set -u
cd "$(dirname "$0")/.."; S=$PWD/build/r11l/src/gas-1.38; W=${1:-build/r11l/gas}; mkdir -p $W
fail=0
for f in as xrealloc xmalloc hash hex-value atof-generic append messages expr app frags input-file input-scrub output-file subsegs symbols version flonum-const flonum-copy flonum-mult bignum-copy obstack error i386 atof-ieee write read; do
	if sh -c "tools/r11l-cc -O -I$S -DI386 -DUSG -c $S/$f.c -o $W/$f.o" > $W/$f.log 2>&1; then :; else fail=$((fail+1)); echo "FAIL $f"; head -6 $W/$f.log; fi
done
echo "gas compile failures: $fail"
