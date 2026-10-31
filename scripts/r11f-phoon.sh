#!/bin/sh
# Build PHOON.aout: the original Phoon source (third_party/phoon/orig) with the
# R11F portability patch applied to a scratch copy, compiled by the SDK wrapper.
# The original sources are never edited.  Output: build/apps/PHOON.aout
#   usage: QDOS386_TOOLS=<tree> sh scripts/r11f-phoon.sh
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd)
cd "$ROOT"
: "${QDOS386_TOOLS:?set QDOS386_TOOLS to the GCC 1.40 / GAS / i386-ld tree}"
export QDOS386_TOOLS
SRC="$ROOT/build/phoon-r11f/src"
rm -rf "$ROOT/build/phoon-r11f"
mkdir -p "$SRC" "$ROOT/build/apps"
cp third_party/phoon/orig/* "$SRC/"
patch -d "$SRC" -p1 -s < third_party/phoon/r11f-portability.patch
# -DOS_SYSV selects the existing SYSV branch of date_parse.c (timezone UTC, see the patch).
( cd "$SRC" && "$ROOT/sdk/bin/qdos386-gcc" -DOS_SYSV -o "$ROOT/build/apps/PHOON.aout" \
	phoon.c astro.c date_parse.c )
ls -l "$ROOT/build/apps/PHOON.aout"
