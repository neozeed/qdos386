#!/bin/sh
# R11L Phase 0/Stage 3: compile cccp and cc1 sources for the guest and report failures.
set -u
cd "$(dirname "$0")/.."; S=$PWD/build/r11l/src/gcc-1.40; GN=$PWD/build/r11l/gen; W=${1:-build/r11l/gcc}; mkdir -p $W
fail=0
cc() { # cc name src extra...
	n=$1; s=$2; shift 2
	if sh -c "tools/r11l-cc -O -I$GN -I$S -I$S/config -DUSG -DLEGACY_HOST64 -DTARGET_CPU_I386 $* -c $s -o $W/$n.o" > $W/$n.log 2>&1; then :; else fail=$((fail+1)); echo "FAIL $n"; grep -v "warning" $W/$n.log | head -${R11L_LINES:-6}; fi
}
for f in cccp; do cc $f $S/cccp.c; done
cc cexp $GN/cexp.c; cc version $S/version.c
for f in c-decl c-typeck c-conv toplev tree print-tree stor-layout fold-const rtl rtlanal expr stmt expmed explow optabs varasm symout dbxout sdbout emit-rtl integrate jump cse loop flow stupid combine regclass local-alloc global-alloc reload reload1 caller-save final recog pragma obstack; do cc $f $S/$f.c; done
cc c-tab $GN/c-tab.c
for f in insn-emt insn-xrt insn-out insn-pep insn-rcg; do cc $f $GN/$f.c; done
echo "gcc compile failures: $fail"
