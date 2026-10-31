#!/bin/sh
# KCTX-FIX1: assemble qdos386-KCTX-FIX1.tar.gz from a COMMITTED tree (branch kernel-context-fix).
#   usage: TINY386_SRC=<R12C Tiny386 tree> sh scripts/kctx-package.sh OUTDIR
# HEAD is archived (full source incl. kernel/kctx1.s, tests/kctx, docs, logs/kctx with the QEMU traces); added: the
# corrected kernels (QEMU kfix1-vga.img, headless kfix1.img) with symbol maps, the R14 kernels they replace (for A/B),
# the minimal patch, the probe programs, the example volume, the unchanged Tiny386.  Prints the archive SHA-256.
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd); cd "$ROOT"
OUT=${1:?}; : "${QDOS386_TOOLS:?}" "${TINY386_SRC:?}"
git diff --quiet HEAD -- . ':!build' ':!logs' || { echo "tree not clean: commit first" >&2; exit 2; }
COMMIT=$(git rev-parse HEAD); NAME=qdos386-KCTX-FIX1
W=$OUT/$NAME; rm -rf "$W"; mkdir -p "$W"
git archive HEAD | tar -x -C "$W"
make -B kfix1-pc > /dev/null 2>&1; make r14-pc > /dev/null 2>&1
sh tests/kctx/build.sh build/kctx > /dev/null
mkdir -p "$W/kernels" "$W/patches" "$W/probe" "$W/volumes"
for v in pc-vga/kfix1-vga.img pc/kfix1.img; do cp build/kfix1/$v "$W/kernels/"; done
python3 tools/kctx-symmap.py build/kfix1/pc-vga/kernel.aout > "$W/kernels/kfix1-vga.map"
python3 tools/kctx-symmap.py build/kfix1/pc/kernel.aout > "$W/kernels/kfix1.map"
cp build/r14/pc-vga/r14-vga.img build/r14/pc/r14.img "$W/kernels/"
python3 tools/kctx-symmap.py build/r14/pc-vga/kernel.aout > "$W/kernels/r14-vga.map"
cp build/pc-r11f-none-run/bios.bin build/pc-r11f-none-run/vgabios.bin "$W/kernels/"
git diff 7be7353 HEAD -- kernel/jobs.h kernel/jobs.c kernel/proc.c kernel/paging.c Makefile > "$W/patches/kctx-fix1-c-and-make.patch"
diff -u kernel/kctx.s kernel/kctx1.s > "$W/patches/kctx.s-to-kctx1.s.diff" || true
cp build/kctx/ALIGNT.aout build/kctx/spin_misaligned.aout build/kctx/spin_aligned.aout "$W/probe/"
R14_BIN_EXTRA="ALIGNT.AOU=build/kctx/ALIGNT.aout SPMIS.AOU=build/kctx/spin_misaligned.aout SPALI.AOU=build/kctx/spin_aligned.aout" \
  sh scripts/r14a-sysvol.sh "$W/volumes/kfix1-system.img" > /dev/null
mkdir -p "$W/emulator/bin"; cp "$TINY386_SRC/tiny386_headless" "$W/emulator/bin/"
[ -f "$TINY386_SRC/LICENSE" ] && cp "$TINY386_SRC/LICENSE" "$W/emulator/LICENSE"
cp docs/KCTX-FIX1-HANDBACK.md "$W/HANDBACK.md"
cat > "$W/PACKAGE.md" <<EOP
# $NAME — corrective kernel release

Base: R14A \`7be7353\` (frozen Hack milestone; R14 source \`43a1f448\`). Branch \`kernel-context-fix\`, commit \`$COMMIT\`.
Read \`HANDBACK.md\` (for the GW-BASIC builder), then \`docs/KCTX-FIX1-AUDIT.md\` and \`docs/KCTX-FIX1-REPORT.md\`.

- \`kernels/kfix1-vga.img\` (QEMU), \`kernels/kfix1.img\` (headless / Tiny386), symbol maps; \`r14-vga.img\`, \`r14.img\`
  (unchanged R14, for A/B); BIOS files
- \`patches/\`: the C/Makefile change and \`kctx.s\` → \`kctx1.s\` as diffs
- \`probe/\`: ALIGNT (residues 0-3, spin/trap/yield) and the supplied spin controls; \`volumes/kfix1-system.img\`
  (R14A volume + the probes in /BIN; it contains SuperBASIC /BIN/BASIC.AOU: private, do not publish)
- full source: \`make kfix1-pc\`; frozen targets unchanged (\`sh tests/r14/check-frozen.sh\`)
EOP
( cd "$W" && find . -type f ! -name SHA256SUMS | LC_ALL=C sort | sed 's#^\./##' | xargs -d '\n' sha256sum > SHA256SUMS )
( cd "$OUT" && tar -czf $NAME.tar.gz $NAME )
sha256sum "$OUT/$NAME.tar.gz" | tee "$OUT/$NAME.tar.gz.sha256"
