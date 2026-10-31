#!/bin/sh
# Pinned host64 GAS sources from LEGACY-GCC-HOST64 R5. No network needed.
set -eu
cd "$(dirname "$0")/.."
mkdir -p build/tools
src=third_party/gas-1.38
sources='as xrealloc xmalloc hash hex-value atof-generic append messages expr app frags input-file input-scrub output-file subsegs symbols version flonum-const flonum-copy flonum-mult bignum-copy obstack error m68k atof-ieee write read'
set --
for f in $sources; do set -- "$@" "$src/$f.c"; done
"${HOSTCC:-cc}" -std=gnu89 -fcommon -g -O0 -DLEGACY_HOST64 -DUSG -DM68K -DM_SUN -include "$src/host64.h" -I"$src" "$@" -o build/tools/as138-m68k >build/tools/gas-m68k-build.log 2>&1
