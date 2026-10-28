#!/bin/sh
# R11K: build the bootloader boundary-test disks.  TEST ONLY images: the kernel carries
# -DQDOS_TEST_BIGK (verified pad data at the END of the kernel, platform/pc/bigk_check.c), sized so
# that kernel + transfer area comes to a chosen number of sectors, and is wrapped by the NEW loader
# (boot2.bin + make-image2.py) and, where the legacy tool accepts it, by the ORIGINAL loader.
# The base kernel is the unchanged default (non-paging) R9 shell kernel.
#   sizes (total sectors, 4 of them the transfer area):
#     119 below the old limit | 120 exactly at it (0xF000 bytes) | 121 above it | 128 the 64 KiB segment
#     129 first size the old loader cannot read | 200 | 400 | 896 the new maximum
#   usage: I386VM=<historical tree> sh scripts/r11k-boot-images.sh [outdir=build/r11k/bootimg]
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd); cd "$ROOT"
: "${I386VM:?set I386VM to the historical tree}"
export QDOS386_TOOLS=${QDOS386_TOOLS:-$I386VM}
OUT=${1:-build/r11k/bootimg}; mkdir -p "$OUT"; : > "$OUT/manifest.txt"   # variant total_sectors pad_bytes text_bytes kernel_raw_bytes
SIZES=${SIZES:-"119 120 121 128 129 200 400 896"}
make build/pc-r9/boot.bin build/pc-r9/boot2.bin I386VM="$I386VM" >/dev/null 2>&1 || make build/pc-r9/boot2.bin I386VM="$I386VM" >/dev/null
# plain/vga: the default non-paging kernel.  pgplain/pgvga: the same with -DQDOS_PAGING (R11K), whose
# kernel is already above the old limit; they prove the larger paging-enabled kernel with the new loader.
for V in ${VARIANTS:-plain vga pgplain pgvga}; do
  PCD=build/r11k/bigk-$V
  FL="-DQDOS_LAYOUT_R9 -DR7_SHELL_MODE -DQDOS_TEST_BIGK"
  PGX=
  case $V in vga|pgvga) FL="-DQDOS_CON_VGA $FL";; esac
  case $V in pg*) FL="$FL -DQDOS_PAGING"; PGX="$PCD/paging.o $PCD/paging_s.o";; esac
  mkdir -p "$PCD"
  # Pass 1: minimal pad, to learn the base kernel size.
  python3 tools/r11k-gen-bigk.py 4 "$PCD/bigk.s" >/dev/null
  "$I386VM/bin/as" -o "$PCD/bigk_data.o" "$PCD/bigk.s"
  make pc I386VM="$I386VM" PCD="$PCD" PCFLAGS="$FL" PCIMG=probe.img PCBOOT=build/pc-r9/boot2.bin PCMKIMG=platform/pc/make-image2.py \
       PCEXTRA="$PGX $PCD/bigk_check.o $PCD/bigk_data.o" >/dev/null 2>&1
  BASE=$(stat -c %s "$PCD/kernel.raw")
  echo "base kernel ($V, 4-byte pad): $BASE bytes"
  for T in $SIZES; do
    K=$((T - 4))                       # transfer area = 4 sectors
    PAD=$(( (K * 512 - (BASE - 4) - 16) / 4 * 4 ))
    [ $PAD -ge 4 ] || { echo "skip $T ($V): base kernel already $BASE bytes"; continue; }
    python3 tools/r11k-gen-bigk.py $PAD "$PCD/bigk.s" >/dev/null
    "$I386VM/bin/as" -o "$PCD/bigk_data.o" "$PCD/bigk.s"
    rm -f "$PCD/kernel.aout"
    make pc I386VM="$I386VM" PCD="$PCD" PCFLAGS="$FL" PCIMG=new-$T.img PCBOOT=build/pc-r9/boot2.bin \
         PCMKIMG=platform/pc/make-image2.py PCEXTRA="$PGX $PCD/bigk_check.o $PCD/bigk_data.o" >/dev/null 2>&1
    GOT=$(( ( $(stat -c %s "$PCD/kernel.raw") + 511) / 512 ))
    [ "$GOT" -eq "$K" ] || { echo "size mismatch for $T ($V): kernel sectors $GOT, wanted $K" >&2; exit 3; }
    cp "$PCD/new-$T.img" "$OUT/$V-new-$T.img"
    cp "$PCD/kernel.raw" "$OUT/$V-kernel-$T.raw"
    TEXT=$(python3 -c "import struct,sys;print(struct.unpack_from('<8I',open(sys.argv[1],'rb').read(32))[1])" "$PCD/kernel.aout")
    echo "$V $T $PAD $TEXT $(stat -c %s "$PCD/kernel.raw")" >> "$OUT/manifest.txt"
    if [ "$T" -le 120 ] && [ -z "$PGX" ]; then
      make pc I386VM="$I386VM" PCD="$PCD" PCFLAGS="$FL" PCIMG=old-$T.img PCBOOT=build/pc-r9/boot.bin >/dev/null 2>&1
      cp "$PCD/old-$T.img" "$OUT/$V-old-$T.img"
    fi
    echo "built $V $T: kernel $(stat -c %s "$PCD/kernel.raw") bytes, $K sectors"
  done
done
sha256sum "$OUT"/*.img | sed 's#  .*/#  #' > "$OUT/SHA256SUMS.txt"
