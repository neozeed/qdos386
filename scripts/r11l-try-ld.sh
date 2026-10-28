#!/bin/sh
# R11L Phase 0/Stage 2: compile the BFD subset and the ld sources for the guest and report failures.
set -u
cd "$(dirname "$0")/.."; R=$PWD/build/r11l/src/binutils-1.94; W=${1:-build/r11l/ld}; mkdir -p $W; GN=$PWD/port/ld
fail=0
# the default linker scripts ld/i386.c includes (a C string each), as the legacy build-host64.sh writes them
mkdir -p $W/gen; python3 - $W/gen <<'PY'
import sys,json,pathlib
p=pathlib.Path(sys.argv[1])
script='OUTPUT_FORMAT("a.out-i386") OUTPUT_ARCH(i386) SECTIONS { .text 0 : { *(.text) . = ALIGN(16); } .data ALIGN(16) : { *(.data) }  .bss SIZEOF(.data) + ADDR(.data) : { *(.bss) [COMMON] . = ALIGN(4); } }'
for suffix in ['x','xr','xu','xn','xN']: (p/('i386.'+suffix)).write_text(json.dumps("{ "+script+" }"))
PY
cc() { n=$1; s=$2; shift 2
	if sh -c "tools/r11l-cc -O -I$W/gen -I$GN -I$R/include -I$R/bfd -I$R/ld $* -c $s -o $W/$n.o" > $W/$n.log 2>&1; then :; else fail=$((fail+1)); echo "FAIL $n"; grep -v warning $W/$n.log | head -${R11L_LINES:-6}; fi; }
for f in libbfd opncls bfd archive targets cache archures core section format syms reloc init ctor aout32 i386aout cpu-i386 stab-syms; do
	cc $f $R/bfd/$f.c -DMINIMIZE=1 -DDEFAULT_VECTOR=i386aout_vec -DSELECT_ARCHITECTURES=bfd_i386_arch
done
cc obstack $R/libiberty/obstack.c
for f in ldgram ldlex ldlang ldctor ldmain ldindr ldwarn ldwrite ldexp ldemul ldver ldmisc ldsym ldvanilla ldfile i386; do cc $f $R/ld/$f.c; done
echo "ld compile failures: $fail"
