#!/bin/sh
# R14A: build Hack 1.03 (the supplied QDOS/C68 port) for QDOS/386: copy the pristine third_party/hack103-ql/orig tree,
# add third_party/hack103-ql/r14a/Makefile.qdos386, apply third_party/hack103-ql/r14a/hack-qdos386.patch (if not
# empty), build with the host GCC 1.40 tree.   usage: sh scripts/r14a-build-hack.sh [OUTDIR]  -> OUTDIR/src/HACK.AOU
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd); cd "$ROOT"
: "${QDOS386_TOOLS:?set QDOS386_TOOLS}"
O=${1:-build/r14a/hack}
rm -rf "$O"; mkdir -p "$O"
cp -r third_party/hack103-ql/orig "$O/src"
cp third_party/hack103-ql/r14a/Makefile.qdos386 "$O/src/"
P=third_party/hack103-ql/r14a/hack-qdos386.patch
if [ -s "$P" ]; then (cd "$O/src" && patch -s -p1 < "$ROOT/$P"); fi
make -C sdk/lib >/dev/null; make -C sdk/libc ../lib/libc68.a >/dev/null
make -C "$O/src" -f Makefile.qdos386 QDOS386_TOOLS="$QDOS386_TOOLS" QDOS386="$ROOT" > "$O/build.log" 2>&1 || { tail -20 "$O/build.log"; exit 1; }
sed -i "s#$ROOT/##g" "$O/src/hack.map"     # library paths relative to the tree: the map is the same in every checkout
(cd third_party/hack103-ql && diff -ru orig "$ROOT/$O/src" -x '*.o' -x HACK.AOU -x hack.map -x onames.h -x Makefile.qdos386) > "$O/source-changes.diff" || true
L=sdk/lib
python3 scripts/r14a-symreport.py "$O/src" "$O/src/c68vt.o" $L/libc68.a $L/libm.a $L/libc.a $L/libgcc-qd.a $L/libqdos.a \
	> "$O/symbols.txt" || { cat "$O/symbols.txt"; echo "r14a-build-hack: unresolved externals"; exit 1; }
grep -c 'warning' "$O/build.log" > "$O/warnings.count" || true
sha256sum "$O/src/HACK.AOU" | sed "s| .*src/| |" > "$O/HACK.AOU.sha256"
ls -la "$O/src/HACK.AOU"
