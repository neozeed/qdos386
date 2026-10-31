#!/bin/sh
# R11I: the block harness opens the disk read-only.  Runs the block harness with its input held
# open on a FIFO, reads the disk's descriptor flags from /proc/PID/fdinfo while it runs, checks the
# access mode is O_RDONLY and the descriptor refers to the disk file, then ends the input.  The
# disk file is hashed before and after; it must not change.
#   usage: sh scripts/r11i-fdcheck.sh <i386vm-r9-block> <kernel-only image.bin> <disk.img> <outdir>
set -eu
H=${1:?usage}; IMG=${2:?usage}; DISK=${3:?usage}; OUT=${4:?usage}
mkdir -p "$OUT"; RES=$OUT/fdcheck.txt
F=$OUT/stdin.fifo; rm -f "$F"; mkfifo "$F"
H_SUM=$(sha256sum "$DISK" | cut -c1-64)
"$H" "$IMG" "$DISK" < "$F" > "$OUT/fdcheck-run.txt" 2>&1 &
PID=$!
exec 3> "$F"          # keep the FIFO open: the harness blocks on input, so it stays alive
FD=""; i=0
while [ $i -lt 100 ]; do
  for fd in /proc/$PID/fd/*; do
    t=$(readlink "$fd" 2>/dev/null || true)
    if [ "$t" = "$(readlink -f "$DISK")" ]; then FD=$(basename "$fd"); break; fi
  done
  [ -n "$FD" ] && break
  i=$((i+1)); sleep 0.1
done
if [ -z "$FD" ]; then
  echo "FDCHECK: FAIL disk descriptor not found" | tee "$RES"; exec 3>&-; kill $PID 2>/dev/null || true; exit 1
fi
FLAGS=$(awk '/^flags:/ {print $2}' /proc/$PID/fdinfo/$FD)
ACC=$(( 0$FLAGS & 3 ))
exec 3>&-                 # end of input: the harness exits
wait $PID 2>/dev/null || true
rm -f "$F"
H_SUM2=$(sha256sum "$DISK" | cut -c1-64)
{
  echo "disk descriptor: /proc/PID/fd/$FD -> $(readlink -f "$DISK")"
  echo "fdinfo flags (octal): $FLAGS  access mode: $ACC (0 = O_RDONLY)"
  echo "disk sha256 before: $H_SUM"
  echo "disk sha256 after:  $H_SUM2"
} > "$RES"
if [ "$ACC" = 0 ] && [ "$H_SUM" = "$H_SUM2" ]; then echo "FDCHECK: PASS" >> "$RES"; cat "$RES"; exit 0; fi
echo "FDCHECK: FAIL" >> "$RES"; cat "$RES"; exit 1
