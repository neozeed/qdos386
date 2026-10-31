#!/bin/sh
# R11H negative test volume (test only, never a release image).  It is the R11H release volume
# plus malformed a.out files derived from MULTIA.AOU (linked for profile 2, entry 0x380000).  Each
# damage is one field or one byte, so the expected refusal is known; NEG.AOU checks the codes.
# The release volume (build/fat16/win1-r11h.img) is not modified.
#   usage: QDOS386_TOOLS=<tree> sh scripts/r11h-negative-volume.sh
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd)
cd "$ROOT"
[ -f build/r11h/MULTIA.AOU ] || { echo "run scripts/r11h-build.sh first" >&2; exit 6; }
D=build/r11h-neg; mkdir -p "$D"
python3 -I - build/r11h/MULTIA.AOU "$D" <<'PY'
import sys, os, struct
src, out = sys.argv[1], sys.argv[2]
b = open(src, 'rb').read()
def put(name, data):
    open(os.path.join(out, name), 'wb').write(data)
def hdr(i, v):
    h = bytearray(b[:32]); h[i:i+4] = struct.pack('<I', v); return bytes(h) + b[32:]
put('BADMAG.AOU', hdr(0, 0x108))                 # OMAGIC replaced by a wrong magic
put('RELOC.AOU', hdr(24, 4))                     # trsize != 0
put('BADENT.AOU', hdr(20, 0x250000))             # entry is no profile base
put('ENTOFF.AOU', hdr(20, 0x380010))             # entry inside profile 2, not its base
put('BADSYM.AOU', hdr(16, struct.unpack('<I', b[16:20])[0] + 1))   # symbols not a record multiple
put('BIGTXT.AOU', hdr(4, 0x200000))              # text larger than the profile
put('TRUNC.AOU', b[:-1])                         # last byte missing
put('TRAIL.AOU', b + b'\x00')                    # one trailing byte
put('SHORT.AOU', b[:20])                         # shorter than the header
put('EMPTYX.AOU', b'')
PY
ARGS=$(sed -n '/^ARGS="\\$/,/PHDATE.AOU=build\/r11f\/PHDATE.aout/p' scripts/r11f-volume.sh \
  | sed -e '1s/^ARGS="\\$//' -e 's/ \$LG"$//' -e 's/\\$//')
NEGARGS=""
for f in BADMAG RELOC BADENT ENTOFF BADSYM BIGTXT TRUNC TRAIL SHORT EMPTYX; do
	NEGARGS="$NEGARGS $f.AOU=$D/$f.AOU"
done
R11H_VOLUME_OUT=build/fat16/win1-r11h-neg.img sh scripts/r11h-volume.sh $NEGARGS
