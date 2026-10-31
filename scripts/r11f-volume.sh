#!/bin/sh
# Build the R11F FAT16 data volume (build/fat16/win1-r11f.img): the R11E volume
# contents plus the R11F time tests and PHOON.AOU.
#   usage: QDOS386_TOOLS=<historical tree> sh scripts/r11f-volume.sh [LGTEST4.aout]
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd)
cd "$ROOT"
: "${QDOS386_TOOLS:?set QDOS386_TOOLS to the GCC 1.40 / GAS / i386-ld tree}"
export QDOS386_TOOLS
W="$ROOT/sdk/bin/qdos386-gcc"
make -C sdk/lib >/dev/null
make -C sdk/compiler QDOS386_TOOLS="$QDOS386_TOOLS" >/dev/null
make -C sdk/libc >/dev/null
make -C sdk/libm >/dev/null
make -C sdk/examples QDOSAPI.aout PS.aout C89EDGE.aout CHILD1.aout CHILD2.aout CONWAIT.aout EXITX.aout EXITR.aout HELPER.aout FPARSE.aout FMATH.aout FPRINT.aout FPMULT.aout FPCOMB.aout C89COR.aout JMPTST.aout SCANTS.aout MATHX.aout CTYPE.aout STRTST.aout FTEST.aout LIBHEL.aout CATFIL.aout MEMSTR.aout HEAPEX.aout \
	STREAM.aout FORMAT.aout CONSOL.aout LIBQDO.aout REPEAT.aout EXITFL.aout >/dev/null
mkdir -p build/r11a/fixtures build/fat16 build/vga build/apps build/r11f
python3 -c "open('build/r11a/fixtures/LINES.TXT','wb').write(b'alpha\nbeta\n\ngamma')"
python3 -c "open('build/r11a/fixtures/EMPTY.TXT','wb').write(b'')"
python3 -c "open('build/r11a/fixtures/LONG.TXT','wb').write(b'x'*200 + b'\\n' + b'end\\n')"
python3 -c "open('build/r11a/fixtures/ABCDEFGH.TXT','wb').write(b'twelve')"
make I386VM="$QDOS386_TOOLS" build/apps/shell.aout build/apps/hello1.aout >/dev/null
mkdir -p build/vga && python3 scripts/make-vga-readme.py build/vga/README.TXT
# R11F: the time tests.  Three builds: no clock (TMABIN), pinned clock (TMABIP),
# advancing clock on PC-rate timers (TMABIA) and on i386VM's nominal rate (TMABII).
# Expected values are in sdk/examples/r11f/timeabi/timeabi_vectors.h (generated).
TD="$ROOT/sdk/examples/r11f/timeabi"
"$W" -o "$ROOT/build/r11f/PHCHK.aout" "$ROOT/sdk/examples/r11f/phchk/phchk.c"
"$W" -o "$ROOT/build/r11f/TMABIN.aout" "$TD/timeabi.c"
"$W" -DTIMEABI_MODE=1 -DTIMEABI_EPOCH=1793102400UL -o "$ROOT/build/r11f/TMABIP.aout" "$TD/timeabi.c"
"$W" -DTIMEABI_MODE=2 -DTIMEABI_EPOCH=946684800UL -o "$ROOT/build/r11f/TMABIA.aout" "$TD/timeabi.c"
"$W" -DTIMEABI_MODE=2 -DTIMEABI_EPOCH=946684800UL -DTIMEABI_RATE_NUM=100UL -DTIMEABI_RATE_DEN=1UL \
	-o "$ROOT/build/r11f/TMABII.aout" "$TD/timeabi.c"
sh scripts/r11f-phoon.sh >/dev/null
# PHDATE: the Phoon date parser (the patched date_parse.c) probed directly.  Expected values:
# sdk/examples/r11f/phdate/phdate-expected.txt (Python calendar.timegm).
"$W" -DOS_SYSV -I"$ROOT/build/phoon-r11f/src" -o "$ROOT/build/r11f/PHDATE.aout" \
	"$TD/../phdate/phdate.c" "$ROOT/build/phoon-r11f/src/date_parse.c"
# TMABIW: a deliberately wrong-rate control.  It assumes 110 ticks per second, 10% more than the
# i386VM timer (100/1).  It must FAIL on the advancing i386VM image; TMABII must PASS there.
"$W" -DTIMEABI_MODE=2 -DTIMEABI_EPOCH=946684800UL -DTIMEABI_RATE_NUM=110UL -DTIMEABI_RATE_DEN=1UL \
	-o "$ROOT/build/r11f/TMABIW.aout" "$TD/timeabi.c"
LG=""
if [ $# -ge 1 ]; then LG="LGTEST.AOU=$1"; fi
# Production volume: no control program.
ARGS="\
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
  EMPTY.TXT=build/r11a/fixtures/EMPTY.TXT LONG.TXT=build/r11a/fixtures/LONG.TXT \
  TMABIN.AOU=build/r11f/TMABIN.aout TMABIP.AOU=build/r11f/TMABIP.aout \
  TMABIA.AOU=build/r11f/TMABIA.aout TMABII.AOU=build/r11f/TMABII.aout \
  PHOON.AOU=build/apps/PHOON.aout PHCHK.AOU=build/r11f/PHCHK.aout \
  PHDATE.AOU=build/r11f/PHDATE.aout $LG"
QDOS_LAYOUT=r9 python3 tools/make-fat16.py build/fat16/win1-r11f.img $ARGS
# Control volume (test only, never a release image): the production volume plus TMABIW.
QDOS_LAYOUT=r9 python3 tools/make-fat16.py build/fat16/win1-r11f-control.img $ARGS TMABIW.AOU=build/r11f/TMABIW.aout
sha256sum build/fat16/win1-r11f.img build/fat16/win1-r11f-control.img
