#!/bin/sh
# R16A: wrap an existing FAT16 superfloppy image as a primary FAT16 partition (type 0x06) at LBA 2048 of a new MBR
# disk image.  Used to turn a normal system volume into a Windows-style MBR-partitioned data disk for QEMU hdb.
#   usage: sh scripts/r16a-wrap-mbr.sh IN_SUPERFLOPPY OUT_MBR [START_LBA]
set -eu
IN=${1:?in}; OUT=${2:?out}; START=${3:-2048}
BYTES=$(wc -c < "$IN"); PSECT=$((BYTES / 512))
[ $((BYTES % 512)) -eq 0 ] || { echo "input not a whole number of 512-byte sectors" >&2; exit 2; }
TOTAL=$((START + PSECT))
rm -f "$OUT"
dd if=/dev/zero of="$OUT" bs=512 count="$TOTAL" status=none
dd if="$IN" of="$OUT" bs=512 seek="$START" conv=notrunc status=none
python3 -I - "$OUT" "$START" "$PSECT" <<'PY'
import sys, struct
img, start, size = sys.argv[1], int(sys.argv[2]), int(sys.argv[3])
with open(img, "r+b") as f:
    mbr = bytearray(f.read(512))
    for i in range(446, 512): mbr[i] = 0
    e = 446
    mbr[e+0] = 0x00
    mbr[e+1:e+4] = b'\xfe\xff\xff'
    mbr[e+4] = 0x06
    mbr[e+5:e+8] = b'\xfe\xff\xff'
    mbr[e+8:e+12]  = struct.pack('<I', start)
    mbr[e+12:e+16] = struct.pack('<I', size)
    mbr[510] = 0x55; mbr[511] = 0xAA
    f.seek(0); f.write(mbr)
print("wrapped %s -> %s: FAT16 primary (0x06) at LBA %d, %d sectors" % (img, img, start, size))
PY
