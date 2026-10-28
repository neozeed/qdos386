#!/bin/sh
# R11L: HOST build of the bootstrap cc1 = the legacy tree's cc1 (same sources, same flags as build-native.sh) with ONE
# source repair: expr.c's misplaced parenthesis in emit_push_insn (see port/patches/gcc-1.40.patch).  The legacy host cc1
# crashes compiling reload.c (a struct argument passed by value), so cc1 for the guest cannot be bootstrapped with it.
# Used only to compile the guest cc1 (and as a second oracle); the oracle for hello.c stays the legacy bin/cc1.
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd); cd "$ROOT"
S=$ROOT/build/r11l/src-host/gcc-1.40; G=$ROOT/build/r11l/gen; O=$ROOT/build/r11l/host-cc1; CC=${CC:-cc}
rm -rf "$O"; mkdir -p "$O"
CF="-std=gnu89 -w -O0 -g -DLEGACY_HOST64 -DTARGET_CPU_I386 -I$G -I$S -I$S/config"
for obj in c-decl c-typeck c-conv toplev version tree print-tree stor-layout fold-const rtl rtlanal expr stmt expmed explow optabs varasm symout dbxout sdbout emit-rtl integrate jump cse loop flow stupid combine regclass local-alloc global-alloc reload reload1 caller-save final recog pragma obstack; do
	src=$S/$obj.c
	# the repaired expr.c is compiled from a copy in $O so that "config.h" still resolves to the HOST (LP64) one in $S
	if [ $obj = expr ]; then cp "$ROOT/build/r11l/src/gcc-1.40/expr.c" "$O/expr-fixed.c"; src=$O/expr-fixed.c; fi
	$CC $CF -c "$src" -o "$O/$obj.o"
done
$CC $CF -c "$G/c-tab.c" -o "$O/c-tab.o"
for obj in insn-emt insn-xrt insn-out insn-pep insn-rcg; do $CC $CF -c "$G/$obj.c" -o "$O/$obj.o"; done
$CC "$O"/*.o -lm -o "$O/cc1"
ls -l "$O/cc1" | awk '{print $5, $9}'
