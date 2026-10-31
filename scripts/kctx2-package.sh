#!/bin/sh
# KCTX-FIX2: assemble qdos386-KCTX-FIX2-MAINLINE.tar.gz from a COMMITTED tree (branch r14a-kctx-mainline-fix).
#   usage: TINY386_SRC=<R12C Tiny386 tree> TOOLCHAIN_ZIP=<I386VM-R1 source zip> NEG_DIR=<dir with the R14 delivery's
#          r14-vga.img and r14.img> sh scripts/kctx2-package.sh OUTDIR
# Full source (git archive of HEAD), the current kernels built by the normal target, their symbol maps, the minimal
# patch against R14A with the list of changed files, the KCTX probes, the public-safe system volume, the read-only
# negative-control kernels (original R14), the Tiny386 R12C source and binary, the toolchain source archive,
# HANDOVER.md, PACKAGE.md, SHA256SUMS.  Prints the archive SHA-256.
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd); cd "$ROOT"
OUT=${1:?}; : "${QDOS386_TOOLS:?}" "${TINY386_SRC:?}" "${TOOLCHAIN_ZIP:?}" "${NEG_DIR:?}"
git diff --quiet HEAD -- . ':!build' ':!logs' || { echo "tree not clean: commit first" >&2; exit 2; }
COMMIT=$(git rev-parse HEAD); NAME=qdos386-KCTX-FIX2-MAINLINE
W=$OUT/$NAME; rm -rf "$W"; mkdir -p "$W"
git archive HEAD | tar -x -C "$W"
make -B qdos386-pc > /dev/null 2>&1
sh tests/kctx/build.sh build/kctx > /dev/null
mkdir -p "$W/kernels/negative-control" "$W/patches" "$W/probe" "$W/volumes" "$W/emulator/bin" "$W/emulator/src" "$W/toolchain"
cp build/qdos386/pc-vga/qdos386-vga.img build/qdos386/pc/qdos386.img "$W/kernels/"
python3 tools/kctx-symmap.py build/qdos386/pc-vga/kernel.aout > "$W/kernels/qdos386-vga.map"
python3 tools/kctx-symmap.py build/qdos386/pc/kernel.aout > "$W/kernels/qdos386.map"
python3 tools/kctx2-resume-check.py build/qdos386/pc-vga/kernel.aout build/qdos386/pc-vga/kernel.raw exact > "$W/kernels/resume-convention.txt"
python3 tools/kctx2-resume-check.py build/qdos386/pc/kernel.aout build/qdos386/pc/kernel.raw exact >> "$W/kernels/resume-convention.txt"
cp build/pc-r11f-none-run/bios.bin build/pc-r11f-none-run/vgabios.bin "$W/kernels/"
cp "$NEG_DIR/r14-vga.img" "$NEG_DIR/r14.img" "$W/kernels/negative-control/"
chmod 444 "$W/kernels/negative-control/"*.img
F="kernel/kctx.s kernel/frozen/kctx-r12a-r14.s kernel/jobs.h kernel/jobs.c kernel/proc.c kernel/paging.c Makefile scripts/r11h-maps-link.sh"
git diff -M 7be7353 HEAD -- $F > "$W/patches/kctx-fix2-against-r14a.patch"
git diff -M --stat 7be7353 HEAD -- $F > "$W/patches/CHANGED-FILES.txt"
cp build/kctx/ALIGNT.aout build/kctx/spin_misaligned.aout build/kctx/spin_aligned.aout "$W/probe/"
sh scripts/qdos386-public-vol.sh "$W/volumes/qdos386-public.img" > /dev/null
( cd "$TINY386_SRC" && find . -type f ! -name '*.o' ! -name 'tiny386' ! -name 'tiny386_headless' ! -name '*.a' | LC_ALL=C sort ) |
  while read -r f; do mkdir -p "$W/emulator/src/$(dirname "$f")"; cp "$TINY386_SRC/$f" "$W/emulator/src/$f"; done
cp "$TINY386_SRC/tiny386_headless" "$W/emulator/bin/"
cp "$TOOLCHAIN_ZIP" "$W/toolchain/I386VM-R1-GCC140-GAS138-AOUT-FULL-SOURCE.zip"; chmod 644 "$W/toolchain/"*.zip
cp docs/KCTX-FIX2-HANDOVER.md "$W/HANDOVER.md"
sed "s/@COMMIT@/$COMMIT/g" docs/KCTX-FIX2-PACKAGE.md > "$W/PACKAGE.md"
( cd "$W" && find . -type f ! -name SHA256SUMS | LC_ALL=C sort | sed 's#^\./##' | xargs -d '\n' sha256sum > SHA256SUMS )
( cd "$OUT" && tar -czf $NAME.tar.gz $NAME )
sha256sum "$OUT/$NAME.tar.gz" | tee "$OUT/$NAME.tar.gz.sha256"
