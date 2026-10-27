#!/bin/sh
# R11I guest short-read test.  Boots the block kernel on a COPY of the volume with stdin held
# open through a FIFO, truncates the copy just past the root directory while the shell runs,
# then issues DIR (metadata still readable), RUN MULTI.AOU and TYPE README.TXT (data now short).
# The kernel must report errors and keep the shell running; the production volume is never touched.
#   usage: I386VM_BLOCK_HARNESS=<bin> sh scripts/r11i-shortread.sh <outdir> <block image> <volume>
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd); cd "$ROOT"
: "${I386VM_BLOCK_HARNESS:?}"
OUT=${1:?}; IMG=${2:?}; VOL=${3:?}; mkdir -p "$OUT"
COPY=$OUT/shortread-copy.img; FIFO=$OUT/shortread.fifo; LOG=$OUT/shortread.txt
H0=$(sha256sum "$VOL" | cut -c1-64)
cp "$VOL" "$COPY"; rm -f "$FIFO"; mkfifo "$FIFO"
CUT=$(python3 - "$COPY" <<'PY'
import struct,sys
d=open(sys.argv[1],'rb').read(64)
bps,spc,res,nf,rent=struct.unpack_from('<HBHBH',d,11); spf=struct.unpack_from('<H',d,22)[0]
print((res+nf*spf)*bps + ((rent*32+bps-1)//bps)*bps)
PY
)
"$I386VM_BLOCK_HARNESS" "$IMG" "$COPY" < "$FIFO" > "$LOG" 2>&1 &
PID=$!
exec 3>"$FIFO"
i=0; until grep -q "SHELL: running" "$LOG" 2>/dev/null; do i=$((i+1)); [ $i -gt 600 ] && break; sleep 0.1; done
truncate -s "$CUT" "$COPY"
echo "truncated copy to $CUT bytes (end of root directory)" > "$OUT/shortread-note.txt"
printf 'DIR\nRUN MULTI.AOU\nTYPE README.TXT\nRUN HELLO.AOU\n' >&3
sleep 20
exec 3>&-
wait $PID || true
H1=$(sha256sum "$VOL" | cut -c1-64)
echo "production volume before $H0" >> "$OUT/shortread-note.txt"
echo "production volume after  $H1" >> "$OUT/shortread-note.txt"
cat "$OUT/shortread-note.txt"
