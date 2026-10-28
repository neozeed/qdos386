#!/bin/sh
# R11M: assemble qdos386-R11M-OS-USABILITY-STRETCH.tar.gz from a COMMITTED tree.
#   usage: sh scripts/r11m-package.sh OUTDIR        (run in the repo; HEAD is archived; the tree must be clean outside build/ and logs/)
# Needs QDOS386_TOOLS (the legacy tree).  Evidence under logs/r11m (transcripts, captures, scratch images) is copied as it is.
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd); cd "$ROOT"
OUT=${1:?usage: r11m-package.sh OUTDIR}; : "${QDOS386_TOOLS:?}"
git diff --quiet HEAD -- . ':!build' ':!logs' || { echo "tree not clean: commit first" >&2; exit 2; }
COMMIT=$(git rev-parse HEAD); NAME=qdos386-R11M-OS-USABILITY-STRETCH
W=$OUT/$NAME; rm -rf "$W"; mkdir -p "$W"
git archive HEAD | tar -x -C "$W"
cp_() { [ -e "$1" ] || { echo "missing $1" >&2; exit 3; }; mkdir -p "$W/$2"; cp -a "$1" "$W/$2/"; }
# --- SDK headers and libraries (headers are in the source tree), the libraries the programs are linked against
for l in sdk/lib/libqdos.a sdk/lib/libc.a sdk/lib/libm.a sdk/lib/libgcc-qd.a; do cp_ $l sdk/lib; done
# --- kernels, harness, shell, guest programs (build products)
for f in build/r11m/vm/qdos386-r11m.bin build/r11m/vm-pg/qdos386-r11m-pg.bin build/r11m/pc/r11m.img build/r11m/pc-pg/r11m-pg.img \
         build/r11m/pc-vga/r11m-vga.img build/r11m/pc-pg-vga/r11m-pg-vga.img build/r11l/harness/i386vm-r9-blockrw build/apps/shell-m.aout; do
  cp_ $f "$(dirname "$f")"
done
mkdir -p "$W/guest-programs"
for p in CONT1 COUNTER TICKER CONINP JOBT WRITER PATHT DIRFUL VFATT; do cp_ sdk/examples/$p.aout guest-programs; done
cp_ build/apps/PHOON.aout guest-programs
# --- scratch volumes: the deterministic initial volumes used by the demonstration and the matrix
mkdir -p "$W/volumes"
sh scripts/r11m-demovol.sh "$W/volumes/r11m-demo-initial.img" 3072
sh scripts/r11m-mkvol.sh "$W/volumes/r11m-vfat-initial.img" 3072 1 512 VFATT.AOU=sdk/examples/VFATT.aout >/dev/null
sh scripts/r11m-mkvol.sh "$W/volumes/r11m-small-root.img" 3072 1 128 >/dev/null
# --- evidence
[ -d logs/r11m ] && { mkdir -p "$W/logs"; cp -a logs/r11m "$W/logs/"; }
cat > "$W/PACKAGE.md" <<EOP
# $NAME

Base: frozen R11L \`81d29ea900b60955837cbb394ea6176e9ffa9fa1\` (R11K \`82405025\`, R11J \`f7664d7\`, R11I \`bc9a90f\`, R11H-FIX1 \`4affc37\` below it, unchanged).
Branch \`r11m-os-usability\`.  Final source commit: \`$COMMIT\`.  The SHA-256 of this archive is printed by the packaging script and reported
with the delivery (it cannot be inside the archive); \`SHA256SUMS\` lists every other file.

Read \`docs/R11M-RUNTIME-REPORT.md\` first, then \`docs/R11M-KNOWN-LIMITATIONS.md\` for the exact boundary of the claim.

Contents
- source tree (git archive of \`$COMMIT\`): kernel, platform, SDK headers and sources (\`sdk/include\`, \`sdk/lib\`, \`sdk/libc\`, \`sdk/examples/r11m\`),
  shell (\`apps/shell\`), image builders and scripts (\`scripts/r11m-*\`), host tests (\`tests/host/r11m\`), guest sequences (\`tests/r11m\`),
  tools (\`tools/r11m-fatwalk.py\`), docs (\`docs/R11M-*.md\`)
- \`sdk/lib/*.a\`: the libraries the guest programs are linked against
- \`build/r11m/*\`: the six R11M kernels (i386VM plain/paging, Tiny386 plain/paging, QEMU VGA plain/paging); \`build/apps/shell-m.aout\`; the writable block harness
- \`guest-programs/\`: CONT1, COUNTER, TICKER, CONINP, JOBT, WRITER, PATHT, DIRFUL, VFATT, PHOON (host cross-built, run in the guest)
- \`volumes/\`: the deterministic initial scratch volumes (the originals were never written; every run used a copy)
- \`logs/r11m/\`: runtime transcripts, QEMU VGA captures, Tiny386 transcripts, final scratch images, \`fsck.fat\` / \`r11m-fatwalk.py\` /
  mtools judge output, matrix rounds, host test logs, frozen-hash checks

Not included: emulator binaries (Tiny386, QEMU), the private Zork I story (none was provided or invented).
EOP
( cd "$W" && find . -type f ! -name SHA256SUMS | LC_ALL=C sort | sed 's#^\./##' | xargs -d '\n' sha256sum > SHA256SUMS )
( cd "$OUT" && tar -czf $NAME.tar.gz $NAME )
sha256sum "$OUT/$NAME.tar.gz" | tee "$OUT/$NAME.tar.gz.sha256"
