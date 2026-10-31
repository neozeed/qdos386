#!/bin/sh
# R11M-FIX1: independent checks of a FAT16 scratch image.  Prints one "CHK name PASS|FAIL detail" line per check and "IMAGE PASS|FAIL".
#   fat-copies  the two FAT copies are byte-identical (python, reads the image directly)
#   fsck        fsck.fat -n reports no problem (exit 0 and no "differ")
#   fatwalk     tools/r11m-fatwalk.py --strict --vfat reports 0 errors
#   usage: sh scripts/fix1-check-image.sh IMG [OUTFILE]
set -u
ROOT=$(cd "$(dirname "$0")/.." && pwd); cd "$ROOT"
I=${1:?usage}; O=${2:-/dev/null}
bad=0
r=$(python3 - "$I" <<'PY'
import struct, sys
d = open(sys.argv[1], 'rb').read()
res = struct.unpack('<H', d[14:16])[0]; nf = d[16]; fsz = struct.unpack('<H', d[22:24])[0]; bps = struct.unpack('<H', d[11:13])[0]
f = [d[(res + k * fsz) * bps:(res + (k + 1) * fsz) * bps] for k in range(nf)]
diff = sum(1 for k in range(0, len(f[0]), 2) if f[0][k:k + 2] != f[1][k:k + 2]) if nf > 1 else 0
print("%d %d" % (nf, diff))
PY
)
nf=${r% *}; diff=${r#* }
if [ "$nf" -lt 2 ] || [ "$diff" = 0 ]; then echo "CHK fat-copies PASS ($nf copies, $diff differing entries)"; else echo "CHK fat-copies FAIL ($diff differing entries)"; bad=1; fi
fo=$(fsck.fat -n "$I" 2>&1); frc=$?
if [ $frc -eq 0 ] && ! echo "$fo" | grep -qi differ; then echo "CHK fsck PASS"; else echo "CHK fsck FAIL (rc=$frc: $(echo "$fo" | sed -n 2p))"; bad=1; fi
wo=$(python3 tools/r11m-fatwalk.py "$I" --strict --vfat 2>&1); wrc=$?
{ echo "$fo"; echo "$wo"; } > "$O" 2>/dev/null
if [ $wrc -eq 0 ]; then echo "CHK fatwalk PASS ($(echo "$wo" | tail -1))"; else echo "CHK fatwalk FAIL (rc=$wrc: $(echo "$wo" | head -1))"; bad=1; fi
[ $bad -eq 0 ] && echo "IMAGE PASS" || echo "IMAGE FAIL"
exit $bad
