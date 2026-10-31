#!/bin/sh
# R11L: HOST bootstrap of the generated sources GCC 1.40 needs (bison parsers and the insn-*.c tables from i386.md).
# These are textual outputs of the historical bison 1.16 and the gen*.c programs, run once on the host exactly as
# build-native.sh in the legacy tree does (host cc, -DLEGACY_HOST64 -DTARGET_CPU_I386).  The guest compilers are then
# built from these files with the historical GCC 1.40; nothing here compiles or assembles hello.c.
#   usage: sh scripts/r11l-bootstrap-gen.sh        (after tools/r11l-stage-src.sh)
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd); cd "$ROOT"
S=$ROOT/build/r11l/src-host; G=$ROOT/build/r11l/gen; CC=${CC:-cc}
rm -rf "$G"; mkdir -p "$G/bison" "$G/tools"
CF="-std=gnu89 -w -O0 -DLEGACY_HOST64 -DTARGET_CPU_I386"
srcs=; for f in LR0 allocate closure conflicts derives files getargs gram lalr lex main nullable output print reader reduce symtab warshall version getopt getopt1; do srcs="$srcs $S/bison-1.16/$f.c"; done
$CC $CF -DUSG -DSTDC_HEADERS '-DXPFILE="bison.simple"' '-DXPFILE1="bison.hairy"' $srcs -o "$G/bison/bison" > "$G/bison/build.log" 2>&1
cp "$S/bison-1.16/bison.simple" "$S/gcc-1.40/c-parse.y" "$S/gcc-1.40/cexp.y" "$G/bison/"
cp "$S/gcc-1.40/config/i386.md" "$G/md"
( cd "$G/bison" && BISON_SIMPLE="$G/bison/bison.simple" ./bison -v -d -o c-tab.c c-parse.y && BISON_SIMPLE="$G/bison/bison.simple" ./bison -v -o cexp.c cexp.y ) > "$G/bison/run.log" 2>&1
cp "$G/bison/c-tab.c" "$G/bison/c-tab.h" "$G/bison/cexp.c" "$G"/
cd "$G"
for g in config flags codes emit extract output peep recog; do
	$CC $CF -I"$G" -I"$S/gcc-1.40" -I"$S/gcc-1.40/config" "$S/gcc-1.40/gen$g.c" "$S/gcc-1.40/rtl.c" "$S/gcc-1.40/obstack.c" -o "tools/gen$g" > "tools/gen$g.log" 2>&1
	case $g in config|flags|codes) out="insn-$g.h";; emit) out=insn-emt.c;; extract) out=insn-xrt.c;; output) out=insn-out.c;; peep) out=insn-pep.c;; recog) out=insn-rcg.c;; esac
	"tools/gen$g" md > "$out"
done
sha256sum c-tab.c c-tab.h cexp.c insn-*.c insn-*.h > GENERATED.SHA256
ls -l c-tab.c cexp.c insn-*.c insn-*.h | awk '{print $5,$9}'
