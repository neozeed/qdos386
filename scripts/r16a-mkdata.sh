#!/bin/sh
# R16A fixture generator: build an MBR-partitioned FAT16 "data disk" for QEMU hdb, with one primary FAT16 partition
# (type 0x06) starting at LBA 2048, plus a couple of files including a VFAT long name.  Also emits the matching
# unpartitioned superfloppy with the same files, for the legacy-compatibility test.
#   usage: sh scripts/r16a-mkdata.sh OUTDIR [PART_KIB]
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd); cd "$ROOT"
export MTOOLS_SKIP_CHECK=1 LC_ALL=C.UTF-8
O=${1:?usage: OUTDIR}; PK=${2:-16384}         # partition size in KiB (default 16 MiB)
mkdir -p "$O"
START=2048                                     # partition start LBA (sectors)
PSECT=$((PK * 2))                              # partition size in sectors (512-byte)

# --- the partition's FAT16 filesystem (built as its own superfloppy image, then placed in the MBR disk) ---
INNER="$O/part.fat16"
rm -f "$INNER"
mkfs.fat -F 16 -s 4 -n R16ADATA -C "$INNER" "$PK" >/dev/null
# contents: a short-name file, a classic 8.3, and a VFAT long name; a subdir with a file
printf 'hello from the R16A MBR FAT16 data disk\n' > "$O/HELLO.TXT"
printf 'the quick brown fox\n' > "$O/readme"
mcopy -o -i "$INNER" "$O/HELLO.TXT" ::/HELLO.TXT
mmd   -i "$INNER" ::/DOCS
printf 'long-name file on a partitioned data disk\n' > "$O/LongDataFileName.txt"
mcopy -o -i "$INNER" "$O/LongDataFileName.txt" "::/DOCS/LongDataFileName.txt"
mcopy -o -i "$INNER" "$O/readme" ::/READ.ME
fsck.fat -n "$INNER" >/dev/null
echo "partition fs $INNER: fsck clean, $PSECT sectors"

# --- the superfloppy variant (same fs, no MBR) for the legacy test ---
cp "$INNER" "$O/data-superfloppy.img"

# --- the MBR disk: [MBR | gap | FAT16 partition @ START] ---
MBR="$O/data-mbr.img"
TOTAL=$((START + PSECT))                        # whole-device sectors
rm -f "$MBR"
dd if=/dev/zero of="$MBR" bs=512 count="$TOTAL" status=none
dd if="$INNER" of="$MBR" bs=512 seek="$START" conv=notrunc status=none
python3 -I - "$MBR" "$START" "$PSECT" <<'PY'
import sys, struct
img, start, size = sys.argv[1], int(sys.argv[2]), int(sys.argv[3])
with open(img, "r+b") as f:
    mbr = bytearray(f.read(512))
    for i in range(446, 512): mbr[i] = 0          # clear boot code tail + table
    e = 446                                        # partition entry 0
    mbr[e+0] = 0x80                                # bootable flag (harmless; exercises status byte)
    mbr[e+1:e+4] = b'\xfe\xff\xff'                 # CHS first (unused; LBA is authoritative)
    mbr[e+4] = 0x06                                # type: FAT16B
    mbr[e+5:e+8] = b'\xfe\xff\xff'                 # CHS last
    mbr[e+8:e+12]  = struct.pack('<I', start)      # start LBA
    mbr[e+12:e+16] = struct.pack('<I', size)       # size in sectors
    mbr[510] = 0x55; mbr[511] = 0xAA
    f.seek(0); f.write(mbr)
print("MBR disk %s: type 0x06 FAT16 primary at LBA %d, %d sectors, device %d sectors" % (img, start, size, start+size))
PY
echo "R16A data disks ready: $MBR (partitioned), $O/data-superfloppy.img (legacy)"
