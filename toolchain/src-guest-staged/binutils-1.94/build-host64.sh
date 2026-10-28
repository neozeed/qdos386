#!/bin/bash
set -eu
ROOT=$(cd "$(dirname "$0")" && pwd)
OUT=${BUILD194:-$ROOT/../build-194}
mkdir -p "$OUT"
OUT=$(cd "$OUT" && pwd)
cd "$OUT"
cp "$ROOT/bfd/hosts/h-host64.h" sysdep.h
CC=${CC:-cc}
FLAGS=(${EXTRA194:-} -std=gnu89 -g -O0 -fcommon -Werror=implicit-function-declaration -include "$OUT/sysdep.h" -I"$OUT" -I"$ROOT/include" -I"$ROOT/bfd" -I"$ROOT/ld")
for src in libbfd opncls bfd archive targets cache archures core section format syms reloc init ctor aout32 i386aout cpu-i386 stab-syms; do
 "$CC" "${FLAGS[@]}" -DMINIMIZE=1 -DDEFAULT_VECTOR=i386aout_vec -DSELECT_ARCHITECTURES=bfd_i386_arch -c "$ROOT/bfd/$src.c" -o "$src.o" >"$src.log" 2>&1 || { cat "$src.log"; exit 1; }
done
for src in obstack; do
 "$CC" "${FLAGS[@]}" -c "$ROOT/libiberty/$src.c" -o "$src.o" >"$src.log" 2>&1 || { cat "$src.log"; exit 1; }
done
ar rcs libbfd.a libbfd.o opncls.o bfd.o archive.o targets.o cache.o archures.o core.o section.o format.o syms.o reloc.o init.o ctor.o aout32.o i386aout.o cpu-i386.o stab-syms.o obstack.o

BISON=${BISON:-$ROOT/../build/bison/bison}
FLEX=${FLEX:-$ROOT/../deps194/usr/bin/flex}
if [ ! -f "$ROOT/ld/ldgram.c" ]; then
 BISON_SIMPLE="$ROOT/../build/bison/bison.simple" "$BISON" -d -o ldgram.c "$ROOT/ld/ldgram.y"
else
 cp "$ROOT/ld/ldgram.c" "$ROOT/ld/ldgram.h" .
fi
if [ ! -f "$ROOT/ld/ldlex.c" ]; then
 "$FLEX" -o ldlex.c "$ROOT/ld/ldlex.l"
else
 cp "$ROOT/ld/ldlex.c" .
fi
python3 - "$OUT" <<'PY2'
import sys,json,pathlib
p=pathlib.Path(sys.argv[1])
script='OUTPUT_FORMAT("a.out-i386") OUTPUT_ARCH(i386) SECTIONS { .text 0 : { *(.text) . = ALIGN(16); } .data ALIGN(16) : { *(.data) }  .bss SIZEOF(.data) + ADDR(.data) : { *(.bss) [COMMON] . = ALIGN(4); } }'
for suffix in ['x','xr','xu','xn','xN']: (p/('i386.'+suffix)).write_text(json.dumps("{ "+script+" }"))
PY2
for src in ldgram ldlex ldlang ldctor ldmain ldindr ldwarn ldwrite ldexp ldemul ldver ldmisc ldsym ldvanilla ldfile i386; do
 input="$ROOT/ld/$src.c"
 case $src in ldgram|ldlex) input="$OUT/$src.c";; esac
 "$CC" "${FLAGS[@]}" -c "$input" -o "$src.o" >"$src.log" 2>&1 || { cat "$src.log"; exit 1; }
done
"$CC" ${EXTRA194:-} -o i386-ld ldgram.o ldlex.o ldlang.o ldctor.o ldmain.o ldindr.o ldwarn.o ldwrite.o ldexp.o ldemul.o ldver.o ldmisc.o ldsym.o ldvanilla.o ldfile.o i386.o libbfd.a >link.log 2>&1 || { cat link.log; exit 1; }
