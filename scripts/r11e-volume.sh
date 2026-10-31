#!/bin/sh
# Build the R11E FAT16 data volume (build/fat16/win1-r11e.img) from source.
#   usage: QDOS386_TOOLS=<historical tree> sh scripts/r11e-volume.sh [LGTEST4.aout]
# The optional argument is the LGTEST suite binary (not redistributed here);
# without it the volume has no LGTEST.AOU and the suite cannot run.
set -eu
cd "$(dirname "$0")/.."
: "${QDOS386_TOOLS:?set QDOS386_TOOLS to the GCC 1.40 / GAS / i386-ld tree}"
export QDOS386_TOOLS
make -C sdk/lib >/dev/null
make -C sdk/compiler QDOS386_TOOLS="$QDOS386_TOOLS" >/dev/null
make -C sdk/libc >/dev/null
make -C sdk/libm >/dev/null
make -C sdk/examples QDOSAPI.aout PS.aout C89EDGE.aout CHILD1.aout CHILD2.aout CONWAIT.aout EXITX.aout EXITR.aout HELPER.aout FPARSE.aout FMATH.aout FPRINT.aout FPMULT.aout FPCOMB.aout C89COR.aout JMPTST.aout SCANTS.aout MATHX.aout CTYPE.aout STRTST.aout FTEST.aout LIBHEL.aout CATFIL.aout MEMSTR.aout HEAPEX.aout \
	STREAM.aout FORMAT.aout CONSOL.aout LIBQDO.aout REPEAT.aout EXITFL.aout >/dev/null
mkdir -p build/r11a/fixtures build/fat16 build/vga build/apps
python3 -c "open('build/r11a/fixtures/LINES.TXT','wb').write(b'alpha\nbeta\n\ngamma')"
# R11B fixtures: an empty file and a file with one line longer than the test buffers.
python3 -c "open('build/r11a/fixtures/EMPTY.TXT','wb').write(b'')"
python3 -c "open('build/r11a/fixtures/LONG.TXT','wb').write(b'x'*200 + b'\\n' + b'end\\n')"
# R11E fixture: a 12-character 8.3 name (ABCDEFGH.TXT) and its six-byte content.
python3 -c "open('build/r11a/fixtures/ABCDEFGH.TXT','wb').write(b'twelve')"
# The R10 apps and the README are the same inputs the R10-VGA volume used.
make I386VM="$QDOS386_TOOLS" build/apps/shell.aout build/apps/hello1.aout >/dev/null
mkdir -p build/vga && python3 scripts/make-vga-readme.py build/vga/README.TXT
LG=""
if [ $# -ge 1 ]; then LG="LGTEST.AOU=$1"; fi
QDOS_LAYOUT=r9 python3 tools/make-fat16.py build/fat16/win1-r11e.img \
  SHELL.AOU=build/apps/shell.aout HELLO.AOU=build/apps/hello1.aout \
  FPJOB.AOU=tests/r10/FPJOB.aout README.TXT=build/vga/README.TXT \
  CTYPE.AOU=sdk/examples/CTYPE.aout STRTST.AOU=sdk/examples/STRTST.aout \
  FTEST.AOU=sdk/examples/FTEST.aout LINES.TXT=build/r11a/fixtures/LINES.TXT \
  LIBHEL.AOU=sdk/examples/LIBHEL.aout CATFIL.AOU=sdk/examples/CATFIL.aout \
  MEMSTR.AOU=sdk/examples/MEMSTR.aout HEAPEX.AOU=sdk/examples/HEAPEX.aout \
  STREAM.AOU=sdk/examples/STREAM.aout FORMAT.AOU=sdk/examples/FORMAT.aout \
  CONSOL.AOU=sdk/examples/CONSOL.aout LIBQDO.AOU=sdk/examples/LIBQDO.aout \
  REPEAT.AOU=sdk/examples/REPEAT.aout EXITFL.AOU=sdk/examples/EXITFL.aout \
  HELPER.AOU=sdk/examples/HELPER.aout FPARSE.AOU=sdk/examples/FPARSE.aout \
  FMATH.AOU=sdk/examples/FMATH.aout FPRINT.AOU=sdk/examples/FPRINT.aout \
  FPMULT.AOU=sdk/examples/FPMULT.aout FPCOMB.AOU=sdk/examples/FPCOMB.aout \
  C89COR.AOU=sdk/examples/C89COR.aout JMPTST.AOU=sdk/examples/JMPTST.aout \
  SCANTS.AOU=sdk/examples/SCANTS.aout MATHX.AOU=sdk/examples/MATHX.aout \
  QDOSAPI.AOU=sdk/examples/QDOSAPI.aout PS.AOU=sdk/examples/PS.aout C89EDGE.AOU=sdk/examples/C89EDGE.aout \
  CHILD1.AOU=sdk/examples/CHILD1.aout CHILD2.AOU=sdk/examples/CHILD2.aout \
  CONWAIT.AOU=sdk/examples/CONWAIT.aout \
  EXITX.AOU=sdk/examples/EXITX.aout EXITR.AOU=sdk/examples/EXITR.aout \
  ABCDEFGH.TXT=build/r11a/fixtures/ABCDEFGH.TXT \
  EMPTY.TXT=build/r11a/fixtures/EMPTY.TXT LONG.TXT=build/r11a/fixtures/LONG.TXT $LG
sha256sum build/fat16/win1-r11e.img
