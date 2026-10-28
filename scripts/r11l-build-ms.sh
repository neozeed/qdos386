#!/bin/sh
# R11L Phase 0/Stage 2: build the MEASUREMENT variant of the guest tools: the same objects, linked with
# libc/libqdos members compiled -DQD_MEMSTAT (sdk/libc/memstat.c) and crt0_ms.s.  Each tool prints one
# "MEMSTAT ..." line at exit (heap peak, largest block, allocations, peak channels, stack peak, ticks).
# The measurement binaries are never used for the functional/identity runs.   Output: build/r11l/guest-ms/
set -eu
cd "$(dirname "$0")/.."
T=${QDOS386_TOOLS:?}; GCC=$T/bin/i386-gcc; AS=$T/bin/as; LD=$T/bin/i386-ld
M=build/r11l/ms; O=build/r11l/guest-ms; mkdir -p $M $O
INC="-Isdk/include -Iport/include -Isdk/libc"
for f in malloc stdlib memstat; do $GCC -c -O -DQD_MEMSTAT $INC sdk/libc/$f.c -o $M/$f.o; done
for f in qdos qdosfs; do $GCC -c -O -DQD_MEMSTAT -Isdk/include sdk/lib/$f.c -o $M/$f.o; done
$AS -o $M/crt0_ms.o sdk/lib/crt0_ms.s
# libs: the shipped archives with the instrumented members replaced/added (host ar, as for the shipped libs)
rm -rf $M/lc $M/lq; mkdir -p $M/lc $M/lq
(cd $M/lc && ar x ../../../../sdk/libc/../lib/libc.a && cp ../malloc.o ../stdlib.o . && ar rc ../libc-ms.a *.o)
(cd $M/lq && ar x ../../../../sdk/lib/libqdos.a && cp ../qdos.o ../qdosfs.o . && ar rc ../libqdos-ms.a *.o)
LIBS="sdk/lib/libm.a $M/libc-ms.a sdk/lib/libgcc-qd.a $M/libqdos-ms.a"
SCRIPT=${1:-sdk/ld/slot1.ld}
G=build/r11l/gcc
lk() { out=$1; shift; rm -f $O/$out; $LD -N -T $SCRIPT -e _start -o $O/$out $M/crt0_ms.o $M/memstat.o "$@" $LIBS > $O/$out.ldlog 2>&1 || { cat $O/$out.ldlog; exit 1; }; [ ! -s $O/$out.ldlog ] || { cat $O/$out.ldlog; exit 1; }; }
lk AS.AOU $(ls build/r11l/gas/*.o | grep -v crt0)
lk CCCP.AOU $G/cccp.o $G/cexp.o $G/version.o
lk CC1.AOU $(ls $G/*.o | grep -v -e '/cccp.o' -e '/cexp.o' -e '/crt0.o')
lk LD.AOU build/r11l/ld/*.o
ls -l $O/*.AOU
