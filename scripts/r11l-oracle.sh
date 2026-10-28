#!/bin/sh
# R11L Phase 0: host oracle.  Runs the historical GCC 1.40 pipeline on the host, in a flat directory that has the
# same names the guest will have (8.3, upper case), and keeps EVERY intermediate.  The stage commands are exactly
# those of $QDOS386_TOOLS/bin/i386-gcc and sdk/bin/qdos386-gcc.
#   usage: sh scripts/r11l-oracle.sh SRC.c OUTDIR [NAME]        (needs QDOS386_TOOLS)
set -eu
SRC=${1:?}; OUT=${2:?}; NAME=${3:-HELLO}
ROOT=$(cd "$(dirname "$0")/.." && pwd); SDK=$ROOT/sdk; T=${QDOS386_TOOLS:?}
rm -rf "$OUT"; mkdir -p "$OUT"; cd "$OUT"
# headers keep the lower-case names the C source uses; the guest folds case when it opens a name (8.3 volume)
for h in "$SDK"/include/*.h; do cp "$h" "$(basename "$h")"; done
cp "$SRC" "$NAME.C"   # the guest names it HELLO.C; the preprocessor sees that exact name
for x in ${ORACLE_EXTRA:-}; do cp "$x" "$(basename "$x")"; done   # local headers etc. (names as the source spells them)
cp "$SDK/lib/crt0_libc.s" CRT0.S; cp "$SDK/lib/libc.a" LIBC.A; cp "$SDK/lib/libm.a" LIBM.A
cp "$SDK/lib/libgcc-qd.a" LIBGCC.A; cp "$SDK/lib/libqdos.a" LIBQDOS.A; cp "$SDK/ld/slot1.ld" SLOT1.LD
{
echo "cccp -nostdinc -undef -Di386 -D__i386__ -I. $NAME.C $NAME.I"
echo "cc1 -quiet -O $NAME.I -o $NAME.S"
echo "as -o $NAME.O $NAME.S"
echo "as -o CRT0.O CRT0.S"
echo "ld -N -T SLOT1.LD -e _start -o $NAME.AOU CRT0.O $NAME.O LIBM.A LIBC.A LIBGCC.A LIBQDOS.A"
} > COMMANDS.TXT
"$T/bin/cccp" -nostdinc -undef -Di386 -D__i386__ -I. "$NAME.C" "$NAME.I"
"$T/bin/cc1" -quiet -O "$NAME.I" -o "$NAME.S"
"$T/bin/as" -o "$NAME.O" "$NAME.S"
"$T/bin/as" -o CRT0.O CRT0.S
"$T/bin/i386-ld" -N -T SLOT1.LD -e _start -o "$NAME.AOU" CRT0.O "$NAME.O" LIBM.A LIBC.A LIBGCC.A LIBQDOS.A
sha256sum "$NAME".C "$NAME".I "$NAME".S "$NAME".O CRT0.O "$NAME".AOU > ORACLE.SHA256
for f in "$NAME.O" "$NAME.AOU"; do python3 - "$f" >> HEADERS.TXT <<'P'
import struct,sys
d=open(sys.argv[1],'rb').read()
m,t,da,b,sy,en,tr,dr=struct.unpack('<8I',d[:32]) if False else struct.unpack('<8I',d[:32])
print("%s: magic %#o text %d data %d bss %d syms %d entry %#x trsize %d drsize %d total %d"%(sys.argv[1],m&0xffff,t,da,b,sy,en,tr,dr,len(d)))
P
done
cat HEADERS.TXT ORACLE.SHA256
