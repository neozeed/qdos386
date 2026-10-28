#!/bin/sh
# R11L: assemble qdos386-R11L-SELFHOST-STRETCH.tar.gz from a COMMITTED tree.
#   usage: sh scripts/r11l-package.sh OUTDIR        (run in the repo; HEAD is archived; the tree must be clean)
# Needs QDOS386_TOOLS (the legacy tree: its binaries and original sources are part of the package).
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd); cd "$ROOT"
OUT=${1:?usage: r11l-package.sh OUTDIR}; : "${QDOS386_TOOLS:?}"
git diff --quiet HEAD -- . ':!build' ':!logs' || { echo "tree not clean: commit first" >&2; exit 2; }
COMMIT=$(git rev-parse HEAD); NAME=qdos386-R11L-SELFHOST-STRETCH
W=$OUT/$NAME; rm -rf "$W"; mkdir -p "$W"
git archive HEAD | tar -x -C "$W"
cp_() { [ -e "$1" ] || { echo "missing $1" >&2; exit 3; }; mkdir -p "$W/$2"; cp -a "$1" "$W/$2/"; }
# --- toolchain: sources (original LF copy and the guest-staged copy with the R11L patch), hashes of the originals, binaries
mkdir -p "$W/toolchain/src-original-lf" "$W/toolchain/src-guest-staged" "$W/toolchain/host-bin" "$W/toolchain/guest-bin" "$W/toolchain/objects"
cp -a build/r11l/src-host/. "$W/toolchain/src-original-lf/"; cp -a build/r11l/src/. "$W/toolchain/src-guest-staged/"
cp -a build/r11l/src/ORIGINAL.SHA256 "$W/toolchain/ORIGINAL.SHA256"
cp -a "$QDOS386_TOOLS"/bin/. "$W/toolchain/host-bin/"; cp -a build/r11l/host-cc1/cc1 "$W/toolchain/host-bin/cc1-lp64-repaired"
cp -a build/r11l/guest/*.AOU "$W/toolchain/guest-bin/"; cp -a build/r11l/apps/*.AOU "$W/toolchain/guest-bin/"
for d in gas gcc ld gen; do mkdir -p "$W/toolchain/objects/$d"; cp -a build/r11l/$d/. "$W/toolchain/objects/$d/"; done
# --- libraries and headers the programs are built against
for l in sdk/lib/libqdos.a sdk/lib/libc.a sdk/lib/libm.a sdk/lib/libgcc-qd.a; do cp_ $l sdk/lib; done
# --- kernels, harness
for f in build/r11l/vm-fs/qdos386-r11l-fs.bin build/r11l/vm-fs-pg/qdos386-r11l-fs-pg.bin build/r11l/pc-fs/r11l-fs.img \
         build/r11l/pc-fs-vga/r11l-fs-vga.img build/r11l/pc-fs-pg/r11l-fs-pg.img build/r11l/pc-fs-pg-vga/r11l-fs-pg-vga.img \
         build/r11l/harness/i386vm-r9-blockrw build/apps/shell-l.aout; do cp_ $f "$(dirname "$f")"; done
# --- evidence: every log, transcript, capture, scratch disk image (initial and final), integrity report of the R11L runs
mkdir -p "$W/logs"; cp -a logs/r11l "$W/logs/"
# --- the demonstration artifacts, extracted from the final scratch volume
A=$W/artifacts/hello; mkdir -p "$A"
F=$(ls logs/r11l/final/stage5/vm.img 2>/dev/null || true)
if [ -n "$F" ]; then for n in HELLO.C HELLO.I HELLO.S HELLO.O HELLO.AOU CRT0.O; do mcopy -n -i "$F" "::$n" "$A/$n"; done; fi
cat > "$W/PACKAGE.md" <<EOP
# $NAME

Base: frozen R11K \`82405025f873dcf13dea8bf352399add195ebaca\` (R11J \`f7664d7\`, R11I \`bc9a90f\`, R11H-FIX1 \`4affc37\` below it, unchanged).
Branch \`r11l-selfhost\`.  Final source commit: \`$COMMIT\`.

Native, historical GCC 1.40 (cccp, cc1), GNU as 1.38 and the binutils-1.94 i386 a.out ld, running as QDOS/386 programs on a
writable FAT16 volume, driven by \`CC.AOU\`.  Read \`docs/R11L-SELFHOST-RUNTIME-REPORT.md\` first, then
\`docs/R11L-KNOWN-LIMITATIONS.md\` for the exact boundary of the claim (the tools themselves were built on the host).

Contents
- source tree (git archive of \`$COMMIT\`): kernel, platform, SDK (libc additions, \`sdk/examples/r11l\`), scripts, tests, docs (\`docs/R11L-*.md\`)
- \`toolchain/\`: original legacy sources (LF copy) and the guest-staged copy, SHA-256 of every original file, host bootstrap
  binaries, the LP64-repaired host cc1, the guest tools \`AS/CCCP/CC1/LD.AOU\` and utilities, the compiled objects
- \`sdk/lib/*.a\`, kernels (\`build/r11l/...\`), the writable harness binary, \`build/apps/shell-l.aout\`
- \`logs/r11l/\`: oracle (host) artifacts, Stage 1-5 transcripts, QEMU screen captures, Tiny386 transcripts, scratch disk images
  (initial and final), \`fsck.fat\` and \`tools/r11l-fatcheck.py\` reports, regression matrices and their judge output
- \`artifacts/hello/\`: HELLO.C .I .S .O .AOU and CRT0.O as left on the final scratch volume by the guest

Not included: emulator binaries (Tiny386, QEMU), the private Zork I story.
\`SHA256SUMS\` lists every file of the package except itself.
EOP
( cd "$W" && find . -type f ! -name SHA256SUMS | LC_ALL=C sort | sed 's#^\./##' | xargs -d '\n' sha256sum > SHA256SUMS )
( cd "$OUT" && tar -czf $NAME.tar.gz $NAME )
sha256sum "$OUT/$NAME.tar.gz" | tee "$OUT/$NAME.tar.gz.sha256"
