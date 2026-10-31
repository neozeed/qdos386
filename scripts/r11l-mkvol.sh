#!/bin/sh
# R11L: make a clean, deterministic FAT16 scratch volume with the host tools mkfs.fat and mcopy
# (dosfstools / mtools).  The kernel's mount rules are met by construction: 512-byte sectors, two
# FATs, power-of-two sectors per cluster, 4085..65524 clusters.
#   usage: r11l-mkvol.sh OUT.img SIZE_KIB SPC ROOT_ENTRIES LABEL [NAME=path ...]
# Files are copied with mcopy; their modification time is taken from the source file (set it with
# touch -d for reproducible volumes).  The volume id is fixed so the image bytes are reproducible.
set -eu
OUT=${1:?usage}; KIB=${2:?}; SPC=${3:?}; ROOT=${4:?}; LABEL=${5:?}; shift 5
rm -f "$OUT"
dd if=/dev/zero of="$OUT" bs=1024 count="$KIB" 2>/dev/null
mkfs.fat -F 16 -s "$SPC" -r "$ROOT" -f 2 -S 512 -R 1 -n "$LABEL" -i 52314C31 "$OUT" >/dev/null
for a in "$@"; do
  name=${a%%=*}; path=${a#*=}
  MTOOLS_SKIP_CHECK=1 mcopy -i "$OUT" -m "$path" "::$name"
done
