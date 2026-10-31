#!/bin/sh
# R11G: build the Task Force '87 interpreter as a slot-1 a.out with the historical GCC 1.40
# tree and the QDOS/386 SDK headers only.
#   usage: scripts/r11g-build.sh <SDK dir> <historical tree> <out dir>
# Sources: third_party/infocom/orig (unchanged, CRLF) plus the platform patch (infocom.c, file.c, support.c)
# third_party/infocom/r11g-platform.patch (QDOS/386 adaptation of infocom.c; the patch is applied
# to LF copies, which are the build inputs).
# Host headers are hidden: the build runs in a private mount namespace where the host include
# directories are empty tmpfs (the historical driver has no -nostdinc option).
# Flags: -O (the SDK wrapper accepts -O; the makefile's -Os is not accepted by it).
set -eu
if [ "${R11G_INNER:-}" != 1 ]; then
  exec env R11G_INNER=1 unshare -m sh -c '
    for d in /usr/include /usr/local/include /usr/lib/gcc/x86_64-linux-gnu/13/include; do
      mount -t tmpfs none "$d" || { echo "cannot isolate $d" >&2; exit 3; }
    done
    exec sh "$0" "$@"' "$0" "$@"
fi
SDK=$1; T=$2; OUT=$3
ROOT=$(cd "$(dirname "$0")/.." && pwd)
SRC=$ROOT/third_party/infocom/orig; PATCH=$ROOT/third_party/infocom/r11g-platform.patch
# Isolation probe: a glibc-only header must NOT be found.
mkdir -p "$OUT/probe"; printf '#include <sys/cdefs.h>\nint x;\n' > "$OUT/probe/p.c"
if "$T/bin/i386-gcc" -c -O -I"$SDK/include" "$OUT/probe/p.c" -o "$OUT/probe/p.o" 2>/dev/null; then
  echo "ISOLATION FAILED: host header visible" >&2; exit 4; fi
echo "isolation probe: host headers hidden"
rm -rf "$OUT/src" "$OUT/obj"; mkdir -p "$OUT/src" "$OUT/obj"
for f in "$SRC"/*.c "$SRC"/*.h; do tr -d '\r' < "$f" > "$OUT/src/$(basename "$f")"; done
(cd "$OUT/src" && patch -s < "$PATCH")
objs=""
for f in "$OUT"/src/*.c; do
  b=$(basename "$f" .c)
  "$T/bin/i386-gcc" -c -O -I"$SDK/include" -I"$OUT/src" -DTIMESEED "$f" -o "$OUT/obj/$b.o"
  objs="$objs $OUT/obj/$b.o"
done
"$T/bin/as" -o "$OUT/obj/crt0.o" "$SDK/lib/crt0_libc.s"
"$T/bin/i386-ld" -N -T "$SDK/ld/slot1.ld" -e _start -o "$OUT/infocom.aout" \
  "$OUT/obj/crt0.o" $objs "$SDK/lib/libm.a" "$SDK/lib/libc.a" "$SDK/lib/libgcc-qd.a" "$SDK/lib/libqdos.a"
echo "built $OUT/infocom.aout"
