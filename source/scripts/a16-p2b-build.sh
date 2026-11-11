#!/bin/sh
# Apollo 16 Phase 2B: the cache workloads (tests/a16/r20g.c: stream, small set, large set), the write-coherence test
# (tests/a16/r20c.c), the background progress program (tests/a16/r20p.c), and their protected copies in build/a16/.
#   usage: sh scripts/a16-p2b-build.sh
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd); cd "$ROOT"; : "${QDOS386_TOOLS:?}"
sh scripts/a14-build-guest.sh > /dev/null
B=$QDOS386_TOOLS/bin; L=sdk/lib; I="-Isdk/include"
mkdir -p build/a16
for v in "R20GS:r20g:-DR20G_KIND=1" "R20GW:r20g:-DR20G_KIND=2" "R20GL:r20g:-DR20G_KIND=3" "R20C:r20c:" "R20P:r20p:" "R20X:r20x:" "R20D:r20d:"; do
	IFS=: read -r P src extra <<EOT
$v
EOT
	p=$(echo "$P" | tr A-Z a-z)
	$B/i386-gcc -O $I $extra -c tests/a16/$src.c -o build/a16/$p.o
	$B/i386-ld -N -T sdk/ld/slot1.ld -e _start -o build/a16/$P.aout build/a13/crt0.o build/a16/$p.o \
		$L/libc.a $L/libm.a $L/libgcc-qd.a $L/libqdos.a
	python3 tools/r16-protect-mark.py build/a16/$P.aout build/a16/${P}P.aout > /dev/null
done
sha256sum build/a16/R20XP.aout build/a16/R20DP.aout build/a16/R20GSP.aout build/a16/R20GWP.aout build/a16/R20GLP.aout build/a16/R20CP.aout build/a16/R20PP.aout
