#!/bin/sh
# R11M-FIX1: assemble qdos386-R11M-FIX1-TINY386-SHUTDOWN.tar.gz from a COMMITTED tree (branch r11m-fix1).
#   usage: TINY386_SRC=<patched Tiny386 tree> sh scripts/fix1-package.sh OUTDIR
# The tree must be clean outside build/ and logs/.  Needs QDOS386_TOOLS.  HEAD is archived; build products, the patched emulator (source files,
# binaries built from base + patch), firmware, volumes and logs/fix1 are added.  Prints the archive SHA-256.
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd); cd "$ROOT"
OUT=${1:?usage: fix1-package.sh OUTDIR}; : "${QDOS386_TOOLS:?}" "${TINY386_SRC:?set TINY386_SRC to the patched Tiny386 tree}"
git diff --quiet HEAD -- . ':!build' ':!logs' || { echo "tree not clean: commit first" >&2; exit 2; }
COMMIT=$(git rev-parse HEAD); NAME=qdos386-R11M-FIX1-TINY386-SHUTDOWN
W=$OUT/$NAME; rm -rf "$W"; mkdir -p "$W"
git archive HEAD | tar -x -C "$W"
cp_() { [ -e "$1" ] || { echo "missing $1" >&2; exit 3; }; mkdir -p "$W/$2"; cp -a "$1" "$W/$2/"; }
for l in sdk/lib/libqdos.a sdk/lib/libc.a sdk/lib/libm.a sdk/lib/libgcc-qd.a; do cp_ $l sdk/lib; done
for f in build/r11m/vm/qdos386-r11m.bin build/r11m/vm-pg/qdos386-r11m-pg.bin build/r11m/pc/r11m.img build/r11m/pc-pg/r11m-pg.img \
         build/r11m/pc-vga/r11m-vga.img build/r11m/pc-pg-vga/r11m-pg-vga.img build/r11l/harness/i386vm-r9-blockrw build/apps/shell-m.aout \
         build/pc-r11f-none-run/bios.bin build/pc-r11f-none-run/vgabios.bin build/fix1/failsync.so; do
  cp_ $f "$(dirname "$f")"
done
mkdir -p "$W/guest-programs"
for p in CONT1 COUNTER TICKER CONINP JOBT WRITER PATHT DIRFUL VFATT SHUTDOWN; do cp_ sdk/examples/$p.aout guest-programs; done
cp_ build/apps/PHOON.aout guest-programs
mkdir -p "$W/volumes"
sh scripts/fix1-vol.sh "$W/volumes/fix1-initial.img" 3072
sh scripts/r11m-demovol.sh "$W/volumes/r11m-demo-initial.img" 3072
# --- the patched emulator: the 7 changed source files, the base hashes, the binaries built from base + patch, the licence
mkdir -p "$W/emulator/src/rawdraw" "$W/emulator/src/sdl" "$W/emulator/bin"
for f in ide.c ide.h main.c pc.c pc.h rawdraw/main.c sdl/main.c; do cp "$TINY386_SRC/$f" "$W/emulator/src/$f"; done
cp "$TINY386_SRC/tiny386_headless" "$TINY386_SRC/tiny386" "$W/emulator/bin/"
[ -f "$TINY386_SRC/LICENSE" ] && cp "$TINY386_SRC/LICENSE" "$W/emulator/LICENSE"
cp "$ROOT/docs/R11M-FIX1-EMULATOR-BUILD.md" "$W/emulator/README.md" 2>/dev/null || true
[ -d logs/fix1 ] && { mkdir -p "$W/logs"; cp -a logs/fix1 "$W/logs/"; }
cat > "$W/PACKAGE.md" <<EOP
# $NAME

Base: frozen R11M release \`a16edcdea8ee25e24d89901a3f19e56bc2a72e07\` (R11M-OS-USABILITY-STRETCH; R11L, R11K, R11J, R11I, R11H-FIX1 below it, unchanged).
Branch \`r11m-fix1\`.  Final source commit: \`$COMMIT\`.  The SHA-256 of this archive is printed by the packaging script and reported with the
delivery (it cannot be inside the archive); \`SHA256SUMS\` lists every other file.

Read \`docs/R11M-FIX1-RUNTIME-REPORT.md\` first, then \`docs/R11M-FIX1-DURABILITY-REPORT.md\`, \`docs/R11M-FIX1-SHUTDOWN-ABI.md\`, \`docs/R11M-FIX1-TEST-POLICY.md\`.

Contents
- source tree (git archive of \`$COMMIT\`): kernel (unchanged since R11M), SDK, shell, scripts (\`scripts/fix1-*\`, \`scripts/legacy-i386vm.sh\`), tests (\`tests/fix1\`),
  docs (\`docs/R11M-FIX1-*.md\`), \`patches/tiny386-fix1-shutdown.patch\`
- \`emulator/\`: the 7 patched Tiny386 source files, \`bin/tiny386_headless\` and \`bin/tiny386\` (windowed rawdraw frontend) built from base + patch in this
  environment (Linux x86-64), the Tiny386 licence, build notes
- \`build/r11m/*\`: the six R11M kernels (unchanged); \`build/apps/shell-m.aout\`; firmware \`build/pc-r11f-none-run/{bios,vgabios}.bin\`; \`build/fix1/failsync.so\` (fault-injection shim)
- \`guest-programs/\`: the R11M guest programs plus SHUTDOWN.AOU
- \`volumes/fix1-initial.img\`: the deterministic initial volume (R11M demo volume + SHUTDOWN.AOU); \`volumes/r11m-demo-initial.img\`: the unchanged R11M one
- \`logs/fix1/\`: transcripts, emulator end records (\`*.end\`), final images, fsck/fatwalk output, write traces, matrix and regression judges

Quick check: \`sh scripts/fix1-smoke.sh OUT\` (QEMU primary, Tiny386 with guest-requested shutdown; i386VM not run).
EOP
( cd "$W" && find . -type f ! -name SHA256SUMS | LC_ALL=C sort | sed 's#^\./##' | xargs -d '\n' sha256sum > SHA256SUMS )
( cd "$OUT" && tar -czf $NAME.tar.gz $NAME )
sha256sum "$OUT/$NAME.tar.gz" | tee "$OUT/$NAME.tar.gz.sha256"
