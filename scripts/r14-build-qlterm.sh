#!/bin/sh
# R14: build QLTerm C R1 for QDOS/386 from the UNMODIFIED third_party/qlterm-c-r1/orig sources: copy them, apply
# third_party/qlterm-c-r1/r14-qdos386.patch (the one source change: TAS -> XCHG), compile them together with the QL
# compatibility layer (sdk/qlcompat) using the SDK compiler, then configure the program (tools/r14-qlterm-config.py,
# the MenuConfig role).   usage: sh scripts/r14-build-qlterm.sh [OUTDIR]   -> OUTDIR/QLTERM.aout (+ QLTERM-raw.aout)
# Configuration of the test build: PORT=0 (SER1I), BAUD=10 (115200), DLDIR=WIN1_HOME/ - the original defaults are
# Telnet (no TCP/IP on QDOS/386), 2400 and RAM2_ (no RAM disk); the original's own tests reconfigured them too.
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd); cd "$ROOT"
: "${QDOS386_TOOLS:?set QDOS386_TOOLS}"
O=${1:-build/r14/qlterm}
rm -rf "$O/src"; mkdir -p "$O"
cp -r third_party/qlterm-c-r1/orig/src "$O/src"
(cd "$O" && patch -s -p1 < "$ROOT/third_party/qlterm-c-r1/r14-qdos386.patch")
diff -r third_party/qlterm-c-r1/orig/src "$O/src" > "$O/source-changes.diff" || true
Q=sdk/qlcompat
sh sdk/bin/qdos386-gcc -O -I $Q/include -I $Q -D main=qlc_program_main -o "$O/QLTERM-raw.aout" \
  "$O/src/main.c" "$O/src/serio.c" "$O/src/screen.c" "$O/src/filexfer.c" "$O/src/platform.c" "$O/src/config.c" \
  "$O/src/font.c" $Q/qlc_io.c $Q/qlc_win.c $Q/qlc_job.c
python3 tools/r14-qlterm-config.py "$O/QLTERM-raw.aout" "$O/QLTERM.aout" PORT=0 BAUD=10 DLDIR=WIN1_HOME/
