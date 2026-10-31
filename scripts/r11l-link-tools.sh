#!/bin/sh
# R11L Stage 2/3: link the ported historical tools (GAS 1.38, cccp, cc1, ld) into QDOS/386 a.out apps.
# Host build of guest executables (classification: "host build").  Needs QDOS386_TOOLS (i386-ld) and
# the objects from r11l-try-gas.sh / r11l-try-gcc.sh / r11l-try-ld.sh.  Output: build/r11l/guest/*.AOU
# usage: r11l-link-tools.sh [slot-ld-script]   (default sdk/ld/slot1.ld)
set -eu
cd "$(dirname "$0")/.."
LD=${QDOS386_TOOLS:?set QDOS386_TOOLS}/bin/i386-ld
SCRIPT=${1:-sdk/ld/slot1.ld}
O=build/r11l/guest; mkdir -p $O
CRT=build/r11l/gas/crt0.o
LIBS="sdk/lib/libm.a sdk/lib/libc.a sdk/lib/libgcc-qd.a sdk/lib/libqdos.a"
G=build/r11l/gcc
lk() { out=$1; shift; rm -f $O/$out; $LD -N -T $SCRIPT -e _start -o $O/$out $CRT "$@" $LIBS > $O/$out.ldlog 2>&1 || { cat $O/$out.ldlog; echo "LINK FAILED $out"; exit 1; }
	# the historical ld exits 0 after "multiple definition" diagnostics: any output is a failure
	[ ! -s $O/$out.ldlog ] || { cat $O/$out.ldlog; echo "LINK DIAGNOSTICS $out"; exit 1; }; \
	printf '%-10s %s\n' $out "$(python3 - $O/$out <<'PY'
import struct,sys
d=open(sys.argv[1],'rb').read(32); m,t,da,b=struct.unpack('<4I',d[:16])
print('text=%d data=%d bss=%d file=%d' % (t,da,b,__import__('os').path.getsize(sys.argv[1])))
PY
)"; }
lk AS.AOU $(ls build/r11l/gas/*.o | grep -v crt0)
lk CCCP.AOU $G/cccp.o $G/cexp.o $G/version.o
lk CC1.AOU $(ls $G/*.o | grep -v -e '/cccp.o' -e '/cexp.o' -e '/crt0.o')
lk LD.AOU build/r11l/ld/*.o
