#!/bin/sh
# R15: assemble qdos386-R15-AMP0.tar.gz from a COMMITTED tree (branch r15-amp-worker).
#   usage: TINY386_SRC=<R12C Tiny386 tree> TOOLCHAIN_ZIP=<I386VM-R1 source zip> sh scripts/r15-package.sh OUTDIR
# Full source (git archive of HEAD) + the AMP kernels (opt-in), the ordinary UP kernels (= KCTX-FIX2), the test-only
# AMP variants, symbol maps, worker disassembly and static check, the guest programs, a public-safe volume with AMPT and
# WORKER, the R15 patch against KCTX-FIX2, Tiny386 source and binary, the toolchain source archive, HANDOVER.md,
# PACKAGE.md, SHA256SUMS.  Prints the archive SHA-256.
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd); cd "$ROOT"
OUT=${1:?}; : "${QDOS386_TOOLS:?}" "${TINY386_SRC:?}" "${TOOLCHAIN_ZIP:?}"
git diff --quiet HEAD -- . ':!build' ':!logs' || { echo "tree not clean: commit first" >&2; exit 2; }
COMMIT=$(git rev-parse HEAD); NAME=qdos386-R15-AMP0
W=$OUT/$NAME; rm -rf "$W"; mkdir -p "$W"
git archive HEAD | tar -x -C "$W"
make -B qdos386-pc > /dev/null 2>&1
make -B qdos386-amp-pc > /dev/null 2>&1
make qdos386-amp-variant AMPV=nosipi AMP_EXTRA=-DQDOS_AMP_TEST_NOSIPI > /dev/null 2>&1
make qdos386-amp-variant AMPV=late AMP_EXTRA=-DQDOS_AMP_TEST_LATE=400000000u > /dev/null 2>&1
make qdos386-amp-variant AMPV=burn AMP_EXTRA=-DQDOS_AMP_TEST_BURN > /dev/null 2>&1
sh scripts/r15-build-guest.sh > /dev/null
mkdir -p "$W/kernels/test-only" "$W/guest" "$W/volumes" "$W/patches" "$W/emulator/bin" "$W/emulator/src" "$W/toolchain"
cp build/qdos386-amp/pc-vga/qdos386-amp-vga.img build/qdos386-amp/pc/qdos386-amp.img \
   build/qdos386/pc-vga/qdos386-vga.img build/qdos386/pc/qdos386.img build/pc-r11f-none-run/bios.bin build/pc-r11f-none-run/vgabios.bin "$W/kernels/"
for v in nosipi late burn; do cp build/qdos386-amp-$v/pc-vga/qdos386-amp-vga.img "$W/kernels/test-only/qdos386-amp-$v-vga.img"; done
python3 tools/kctx-symmap.py build/qdos386-amp/pc-vga/kernel.aout > "$W/kernels/qdos386-amp-vga.map"
python3 tools/kctx-symmap.py build/qdos386-amp/pc/kernel.aout > "$W/kernels/qdos386-amp.map"
python3 tests/r15/check-ap-objects.py build/qdos386-amp/pc-vga > "$W/kernels/ap-static-check.txt"
cp build/qdos386-amp/pc-vga/worker-disasm.txt "$W/kernels/qdos386-amp-vga.worker-disasm.txt"
python3 tests/r15/check-ap-objects.py build/qdos386-amp/pc >> "$W/kernels/ap-static-check.txt"
cp build/r15/AMPT.aout build/r15/WORKER.aout build/r14a/SPINM.aout "$W/guest/"
sh scripts/qdos386-public-vol.sh "$W/volumes/r15-public.img" > /dev/null
T=$(mktemp -d)
for p in AMPT WORKER SPINM; do cp "$W/guest/$p.aout" "$T/x"; touch -d '2026-10-30 08:00:00' "$T/x"; MTOOLS_SKIP_CHECK=1 mcopy -o -m -i "$W/volumes/r15-public.img" "$T/x" "::/BIN/$p.AOU"; done
rm -rf "$T"; fsck.fat -n "$W/volumes/r15-public.img" > /dev/null
F="kernel/amp.h kernel/amp.c kernel/amp_s.s kernel/amp_ap.c kernel/amp_ap_s.s kernel/jobs.c kernel/shellboot.c Makefile sdk/include/qdos386amp.h sdk/examples/r15/worker.c"
git diff 5b6fc47 HEAD -- $F > "$W/patches/r15-against-kctx-fix2.patch"
git diff --stat 5b6fc47 HEAD -- $F > "$W/patches/CHANGED-FILES.txt"
( cd "$TINY386_SRC" && find . -type f ! -name '*.o' ! -name 'tiny386' ! -name 'tiny386_headless' ! -name '*.a' | LC_ALL=C sort ) |
  while read -r f; do mkdir -p "$W/emulator/src/$(dirname "$f")"; cp "$TINY386_SRC/$f" "$W/emulator/src/$f"; done
cp "$TINY386_SRC/tiny386_headless" "$W/emulator/bin/"
cp "$TOOLCHAIN_ZIP" "$W/toolchain/I386VM-R1-GCC140-GAS138-AOUT-FULL-SOURCE.zip"; chmod 644 "$W/toolchain/"*.zip
cp docs/R15-HANDOVER.md "$W/HANDOVER.md"
sed "s/@COMMIT@/$COMMIT/g" docs/R15-PACKAGE.md > "$W/PACKAGE.md"
( cd "$W" && find . -type f ! -name SHA256SUMS | LC_ALL=C sort | sed 's#^\./##' | xargs -d '\n' sha256sum > SHA256SUMS )
( cd "$OUT" && tar -czf $NAME.tar.gz $NAME )
sha256sum "$OUT/$NAME.tar.gz" | tee "$OUT/$NAME.tar.gz.sha256"
