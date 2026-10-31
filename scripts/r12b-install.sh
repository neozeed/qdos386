#!/bin/sh
# R12B: install the system directory layout onto an existing QDOS/386 FAT16 volume image (host mtools).
#   usage: sh scripts/r12b-install.sh VOL.img
# Creates /BIN /ETC /SDK/BIN /SDK/LIB /SDK/INCLUDE /TMP /HOME /GAMES, copies the system programs into /BIN, Phoon into
# /GAMES, the SDK headers and libraries into /SDK, and fs/r12b-system/ETC/STARTUP.CMD.  Programs already in the root
# stay there as well (the root is the last place the program search looks, so old volumes keep working), except the
# ones listed in MOVE (LEAK stays: the R12 probe opens WIN1:/LEAK.AOU), which are removed from the root so that finding them proves the PATH search.
# Deterministic: fixed file times.  Existing directories are reused; existing files are replaced.
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd); cd "$ROOT"
export MTOOLS_SKIP_CHECK=1 LC_ALL=C.UTF-8
V=${1:?usage: r12b-install.sh VOL.img}
E=${QDOS_BIN_DIR:-sdk/examples}
S=fs/r12b-system
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
STAMP='2026-10-29 08:00:00'
put() {   # put SRC DEST(::/path)
  cp "$1" "$T/f"; touch -d "$STAMP" "$T/f"; mcopy -o -m -i "$V" "$T/f" "$2"
}
for d in BIN ETC SDK SDK/BIN SDK/LIB SDK/INCLUDE TMP HOME GAMES; do
  mmd -i "$V" "::/$d" 2>/dev/null || true
done
for n in EDIT BASIC LEAK PINFO ENVT SHUTDOWN; do
  [ -f "$E/$n.aout" ] && put "$E/$n.aout" "::/BIN/$n.AOU"
done
for n in ${R12B_BIN_EXTRA:-}; do put "${n#*=}" "::/BIN/${n%%=*}"; done
# Phoon: from the root of the volume (the R11F build) to /GAMES
if mtype -i "$V" ::/PHOON.AOU > "$T/ph" 2>/dev/null; then put "$T/ph" ::/GAMES/PHOON.AOU; mdel -i "$V" ::/PHOON.AOU; fi
for n in HELLO ARGS; do
  if mtype -i "$V" "::/$n.AOU" > "$T/x" 2>/dev/null; then put "$T/x" "::/SDK/BIN/$n.AOU"; fi
done
for f in sdk/include/*.h; do put "$f" "::/SDK/INCLUDE/$(basename "$f" | tr a-z A-Z)"; done
for f in libc libqdos libgcc-qd; do [ -f "sdk/lib/$f.a" ] && put "sdk/lib/$f.a" "::/SDK/LIB/$(echo $f | tr a-z A-Z).A"; done
put "$S/SDK/LIB/README.TXT" ::/SDK/LIB/README.TXT
put "$S/HOME/README.TXT" ::/HOME/README.TXT
put "$S/TMP/README.TXT" ::/TMP/README.TXT
put "${R12B_STARTUP:-$S/ETC/STARTUP.CMD}" ::/ETC/STARTUP.CMD
# moved programs: only in /BIN (so a run from elsewhere proves the PATH search)
for n in ${R12B_MOVE:-EDIT BASIC PINFO ENVT SHUTDOWN}; do mdel -i "$V" "::/$n.AOU" 2>/dev/null || true; done
echo "installed the R12B system layout on $V"
