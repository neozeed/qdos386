#!/bin/sh
# Build the R11A FAT16 data volume (build/fat16/win1-r11a.img) from source.
#   usage: QDOS386_TOOLS=<historical tree> sh scripts/r11a-volume.sh [LGTEST4.aout]
# The optional argument is the LGTEST suite binary (not redistributed here);
# without it the volume has no LGTEST.AOU and the suite cannot run.
set -eu
cd "$(dirname "$0")/.."
: "${QDOS386_TOOLS:?set QDOS386_TOOLS to the GCC 1.40 / GAS / i386-ld tree}"
export QDOS386_TOOLS
make -C sdk/lib >/dev/null
make -C sdk/libc >/dev/null
# The example rules depend only on their sources, not on libc.a, so force a
# rebuild here; otherwise stale binaries from an older libc can be shipped.
rm -f sdk/examples/CTYPE.aout sdk/examples/STRTST.aout sdk/examples/FTEST.aout \
	sdk/examples/LIBHEL.aout sdk/examples/CATFIL.aout sdk/examples/MEMSTR.aout \
	sdk/examples/HEAPEX.aout
make -C sdk/examples CTYPE.aout STRTST.aout FTEST.aout LIBHEL.aout CATFIL.aout MEMSTR.aout HEAPEX.aout >/dev/null
mkdir -p build/r11a/fixtures build/fat16 build/vga build/apps
python3 -c "open('build/r11a/fixtures/LINES.TXT','wb').write(b'alpha\nbeta\n\ngamma')"
# The R10 apps and the README are the same inputs the R10-VGA volume used.
make I386VM="$QDOS386_TOOLS" build/apps/shell.aout build/apps/hello1.aout >/dev/null
mkdir -p build/vga && python3 scripts/make-vga-readme.py build/vga/README.TXT
LG=""
if [ $# -ge 1 ]; then LG="LGTEST.AOU=$1"; fi
QDOS_LAYOUT=r9 python3 tools/make-fat16.py build/fat16/win1-r11a.img \
  SHELL.AOU=build/apps/shell.aout HELLO.AOU=build/apps/hello1.aout \
  FPJOB.AOU=tests/r10/FPJOB.aout README.TXT=build/vga/README.TXT \
  CTYPE.AOU=sdk/examples/CTYPE.aout STRTST.AOU=sdk/examples/STRTST.aout \
  FTEST.AOU=sdk/examples/FTEST.aout LINES.TXT=build/r11a/fixtures/LINES.TXT \
  LIBHEL.AOU=sdk/examples/LIBHEL.aout CATFIL.AOU=sdk/examples/CATFIL.aout \
  MEMSTR.AOU=sdk/examples/MEMSTR.aout HEAPEX.AOU=sdk/examples/HEAPEX.aout $LG
sha256sum build/fat16/win1-r11a.img
