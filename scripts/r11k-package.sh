#!/bin/sh
# R11K: assemble qdos386-R11K-PAGING-INTEGRATION.tar.gz from a COMMITTED tree.
#   usage: sh scripts/r11k-package.sh <outdir>      (run in the repo; HEAD is archived; the tree must be clean)
# Contents: git archive of HEAD (source, docs, scripts, tests, logs/r11k ...) + the build products that
# the documents refer to (not re-creatable without the historical toolchain): see PACKAGE.md.
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd); cd "$ROOT"
OUT=${1:?usage: r11k-package.sh OUTDIR}
git diff --quiet HEAD -- . ':!build' || { echo "tree not clean: commit first" >&2; exit 2; }
COMMIT=$(git rev-parse HEAD); NAME=qdos386-R11K-PAGING-INTEGRATION
W=$OUT/$NAME; rm -rf "$W"; mkdir -p "$W"
git archive HEAD | tar -x -C "$W"
copy() { [ -e "$1" ] || { echo "missing $1" >&2; exit 3; }; mkdir -p "$W/$(dirname "$1")"; cp -a "$1" "$W/$1"; }
# R11H / R11I / R11J build products carried forward unchanged (volumes, default images, firmware, harness inputs)
for d in build/fat16 build/i386vm-r11h-neg build/i386vm-r11h-pin build/i386vm-r11h-rel build/pc-fault \
         build/pc-r11f-none-run build/pc-r11h build/pc-r11h-none build/pc-r11h-pin build/pc-r11h-vga \
         build/r11h build/r11h-maps build/r11h-neg build/r11i build/r11j build/pc-r9 build/harness-r11j; do
  [ -d "$d" ] && { mkdir -p "$W/$d"; cp -a "$d/." "$W/$d/"; }
done
# R11K products
for f in build/r11k/vm-paging/qdos386-r11k-paging.bin build/r11k/pc-paging/r11k-paging.img \
         build/r11k/pc-paging-vga/r11k-paging-vga.img build/r11k/pc-default/r11k-default.img \
         build/r11k/pc-default-vga/r11k-default-vga.img build/r11k/test-vm-flaunch/qdos386-r11k-test-flaunch.bin \
         build/r11k/test-pc-flaunch/r11k-test-flaunch.img build/r11k/test-vm-cap/qdos386-r11k-test-cap.bin \
         build/r11k/test-vm-intcr3/qdos386-r11k-test-intcr3.bin build/r11k/test-pc-intcr3/r11k-test-intcr3.img \
         build/r11k/test-vm-share/qdos386-r11k-test-share.bin build/r11k/test-vm-tasks/qdos386-r11k-test-tasks.bin \
         build/r11k/test-pc-tasks/r11k-test-tasks.img build/r11k/test-pc-tasks-vga/r11k-test-tasks-vga.img \
         build/r11k/apps/TWICE.AOU build/r11k/apps/PGRUN.AOU build/r11k/apps/TWICET.AOU \
         build/r11k/apps/SAME.AOU build/r11k/apps/SAMELNCH.AOU build/r11k/vol/BIG.BIN build/fat16/win1-r11k.img; do
  copy "$f"
done
mkdir -p "$W/build/r11k/bootimg"; cp -a build/r11k/bootimg/*.img build/r11k/bootimg/*.raw build/r11k/bootimg/manifest.txt "$W/build/r11k/bootimg/"
rm -f "$W/build/r11k/bootimg/"*-old-* 2>/dev/null || true; cp -a build/r11k/bootimg/*-old-*.img "$W/build/r11k/bootimg/" 2>/dev/null || true
for l in build/lib build/xfer.bin; do [ -e "$l" ] && copy "$l"; done
for l in sdk/lib/libqdos.a sdk/lib/libc.a sdk/lib/libm.a sdk/lib/libgcc-qd.a; do [ -e "$l" ] && copy "$l"; done
rm -rf "$W/build/r11k/qemu-"* 2>/dev/null || true
cat > "$W/PACKAGE.md" <<EOP
# $NAME

Base: frozen R11J \`f7664d7\` (archive SHA256 \`24c5c635...\`; R11I \`bc9a90f\` and R11H-FIX1 \`4affc37\` below it, all unchanged).
Branch \`r11k-integration\`.  Final source commit: \`$COMMIT\`.

**Bounded checkpoint, not an isolation release.**  Ring 0 only; only the 1 MiB image window of a paged
process is private; the heap, stacks and kernel are shared.  Paging exists only in images built with
\`-DQDOS_PAGING\`; every default image is byte-identical to R11I (\`logs/r11k/default/identity.txt\`).  The new
PC loader (\`boot2\`) is selectable independently of paging; the original loader is still the default.

Contents
- Source tree (git archive of \`$COMMIT\`): kernel, platform (incl. \`platform/pc/boot2.s\`, \`make-image2.py\`), SDK
  (\`qdospg.c\`, \`examples/r11k\`), tools, scripts, docs (\`docs/R11K-*.md\`), tests, patches, logs.
- \`build/r11k/\`: paging kernels (i386VM block; PC; PC VGA), default kernels with the new loader, TEST ONLY kernels
  (failed copy, pool cap, interrupt around CR3, kernel tasks, shared-frames negative control), bootloader boundary
  disks (\`bootimg/\`), \`apps/\` (TWICE, PGRUN, TWICET, SAME, SAMELNCH), \`vol/BIG.BIN\`.
- \`build/fat16/win1-r11k.img\`: R11K test volume = the frozen R11H release files + R11J and R11K programs + BIG.BIN.
- Everything carried forward from R11J's package \`build/\` (default images, R11H volumes, firmware, harness binaries).
- \`logs/r11k/\`: baseline, audit, default identity, boot matrix (3 rounds), old-limit experiment, full matrix
  (3 rounds each), \`results.json\` from \`scripts/r11k-assert.py\`.

Not included (deliberately): emulator binaries (Tiny386, QEMU) and the historical GCC 1.40 tree (identified by
hash in \`docs/R11K-RUNTIME-REPORT.md\`); the private Zork I story.

Rebuild (fresh extraction)
    export I386VM=<historical tree> QDOS386_TOOLS=<same tree>
    sh scripts/r11j-build-harness.sh build/harness
    sh scripts/r11k-images.sh            # paging, default-with-new-loader and TEST kernels
    sh scripts/r11k-boot-images.sh       # bootloader boundary disks
    sh scripts/r11k-build.sh             # SDK libraries, TWICE/PGRUN/TWICET, win1-r11k.img

Verification: \`docs/R11K-RUNTIME-REPORT.md\`; \`python3 scripts/r11k-assert.py logs/r11k/matrix\`; \`SHA256SUMS\` lists
every file in this package except itself.
EOP
( cd "$W" && find . -type f ! -name SHA256SUMS | LC_ALL=C sort | sed 's#^\./##' | xargs sha256sum > SHA256SUMS )
( cd "$OUT" && tar -czf $NAME.tar.gz $NAME )
sha256sum "$OUT/$NAME.tar.gz"
