#!/bin/sh
# R11L: prove that the writable-FAT work did not change any default or R11K image.
# Rebuilds, from the current tree and WITHOUT -DQDOS_FS_WRITE, the images below into build/r11l/chk-*
# and compares them with the hashes frozen in R11H/R11I/R11K (logs/r11k/default/identity.txt and
# docs/R11K-RUNTIME-REPORT.md).  Exit status 1 if any differs.
#   usage: I386VM=<historical tree> sh scripts/r11l-default-check.sh [OUTFILE]
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd); cd "$ROOT"
: "${I386VM:?}"
OUTF=${1:-/dev/stdout}
R9="-DR7_SHELL_MODE -DQDOS_LAYOUT_R9"
mk() { make "$@" I386VM="$I386VM" >/dev/null 2>&1; }
mk build/pc-r9/boot.bin; mk build/pc-r9/boot2.bin
rm -rf build/r11l/chk-*
mk i386vm I386D=build/r11l/chk-block I386FLAGS="$R9 -DQDOS_I386VM_BLOCK" I386IMG=qdos386-r11i-block.bin I386WIN= IMGENV="QDOS_LAYOUT=r9"
mk i386vm I386D=build/r11l/chk-pg I386FLAGS="$R9 -DQDOS_I386VM_BLOCK -DQDOS_PAGING" I386IMG=qdos386-r11k-paging.bin I386WIN= IMGENV="QDOS_LAYOUT=r9" I386EXTRA="kernel/paging.c kernel/paging.s"
mk pc PCD=build/r11l/chk-pcdef PCFLAGS="-DQDOS_LAYOUT_R9 -DR7_SHELL_MODE" PCIMG=r11k-default.img PCBOOT=build/pc-r9/boot2.bin PCMKIMG=platform/pc/make-image2.py
mk pc PCD=build/r11l/chk-pcpg PCFLAGS="-DQDOS_LAYOUT_R9 -DR7_SHELL_MODE -DQDOS_PAGING" PCIMG=r11k-paging.img PCBOOT=build/pc-r9/boot2.bin PCMKIMG=platform/pc/make-image2.py PCEXTRA="build/r11l/chk-pcpg/paging.o build/r11l/chk-pcpg/paging_s.o"
mk pc PCD=build/r11l/chk-pcr11h PCFLAGS="-DQDOS_LAYOUT_R9 -DR7_SHELL_MODE" PCIMG=r11h-shell.img PCBOOT=build/pc-r9/boot.bin
mk i386vm I386D=build/r11l/chk-r11h I386FLAGS="$R9" I386IMG=qdos386-r11h-rel.bin I386WIN=build/fat16/win1-r11h.img IMGENV="QDOS_LAYOUT=r9"
bad=0
chk() { # name file expected
  got=$(sha256sum "$2" | cut -c1-64)
  if [ "$got" = "$3" ]; then echo "IDENTICAL $1 $got" >> "$OUTF"; else echo "DIFFERENT $1 got $got want $3" >> "$OUTF"; bad=1; fi
}
: > "$OUTF" 2>/dev/null || true
chk r11i-block      build/r11l/chk-block/qdos386-r11i-block.bin 3ee37c19ac52acbc186bab89dbf06f20af67f4543d1d479d91c17f1bbbc17400
chk r11k-vm-paging  build/r11l/chk-pg/qdos386-r11k-paging.bin   2e61c4aee1d05e5dcb18fb40e9aec8ea068afa216e44099d310b583e9c83730a
chk r11k-pc-default build/r11l/chk-pcdef/r11k-default.img       2067b3e810cf295f6c7f6d5a8ca5849133b342ff385f65f2a63733a08a5a202c
chk r11k-pc-paging  build/r11l/chk-pcpg/r11k-paging.img         3a09141da237c85c1f44b9a843a72d6bb88baaecf1dfdfdbacdf74154129617f
chk r11h-pc         build/r11l/chk-pcr11h/r11h-shell.img        8f22b0b2613d05543f8a174f341c772ffccc15a56e7aa53264ff0b8d6f512307
chk r11h-rel-vm     build/r11l/chk-r11h/qdos386-r11h-rel.bin    c5462138ea630fa73d5ca82c69d08f79e7a919eda2e7f11b4db5c2fd358af03a
exit $bad
