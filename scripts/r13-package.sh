#!/bin/sh
# R13: assemble qdos386-R13-VIRTUAL-DISPLAYS.tar.gz from a COMMITTED tree (branch r13-virtual-displays).
#   usage: TINY386_SRC=<R12C Tiny386 tree> sh scripts/r13-package.sh OUTDIR
# HEAD is archived; build products (R13 kernels - Minerva font and the QDOS5x9 variant - and shell-d; the unchanged
# R12C, R12B, R12A, R12 kernels and shells), guest programs (R13 builds; the frozen R12B and R12A binaries), the
# example system volume, the Tiny386 emulator (unchanged from R12C), firmware and logs/r13 are added.  The Minerva
# ROM is NOT included (only the two GPL-2.0 font source modules, in third_party/minerva).  Prints the archive SHA-256.
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd); cd "$ROOT"
OUT=${1:?usage: r13-package.sh OUTDIR}; : "${QDOS386_TOOLS:?}" "${TINY386_SRC:?}"
git diff --quiet HEAD -- . ':!build' ':!logs' || { echo "tree not clean: commit first" >&2; exit 2; }
COMMIT=$(git rev-parse HEAD); NAME=qdos386-R13-VIRTUAL-DISPLAYS
W=$OUT/$NAME; rm -rf "$W"; mkdir -p "$W"
git archive HEAD | tar -x -C "$W"
cp_() { [ -e "$1" ] || { echo "missing $1" >&2; exit 3; }; mkdir -p "$W/$2"; cp -a "$1" "$W/$2/"; }
for l in sdk/lib/libqdos.a sdk/lib/libc.a sdk/lib/libm.a sdk/lib/libgcc-qd.a; do cp_ $l sdk/lib; done
for f in build/r13/pc-vga/r13-vga.img build/r13/pc/r13.img build/r13/pc-vga-q5/r13-vga-q5.img build/apps/shell-d.aout \
         build/r12c/pc-vga/r12c-vga.img build/r12c/pc/r12c.img build/r12b/pc-vga/r12b-vga.img build/r12b/pc/r12b.img \
         build/r12a/pc-vga/r12a-vga.img build/r12a/pc/r12a.img build/r12/pc-pg/r12-pg.img build/r12/pc-pg-vga/r12-pg-vga.img \
         build/apps/shell-c.aout build/apps/shell-b.aout build/apps/shell-a.aout build/apps/shell-m.aout \
         build/pc-r11f-none-run/bios.bin build/pc-r11f-none-run/vgabios.bin; do
  cp_ $f "$(dirname "$f")"
done
mkdir -p "$W/guest-programs" "$W/guest-programs-r12b" "$W/guest-programs-r12a"
for p in SURFT COLOUR BOUNCE TITLET SYNCT OLDT ENVT NEGDYN BASIC BASTEST MAPTEST BIGBSS HEAPT STKT FAULTT SPAWNT CHILDT PINFO CONAPI CONDEMO EDIT \
         LEAK CONT1 COUNTER TICKER CONINP JOBT WRITER PATHT DIRFUL VFATT SHUTDOWN; do
  cp_ sdk/examples/$p.aout guest-programs
done
cp_ build/apps/PHOON.aout guest-programs
sh tests/r12c/frozen-bins.sh >/dev/null; cp -a build/frozen-r12b/*.aout "$W/guest-programs-r12b/"
sh tests/r12b/frozen-bins.sh >/dev/null; cp -a build/frozen-r12a/*.aout "$W/guest-programs-r12a/"
mkdir -p "$W/volumes"
sh scripts/r13-sysvol.sh "$W/volumes/r13-system.img" >/dev/null
mkdir -p "$W/emulator/src/rawdraw" "$W/emulator/src/sdl" "$W/emulator/bin"
for f in ide.c ide.h main.c pc.c pc.h rawdraw/main.c sdl/main.c; do cp "$TINY386_SRC/$f" "$W/emulator/src/$f"; done
cp "$TINY386_SRC/tiny386_headless" "$W/emulator/bin/"; [ -f "$TINY386_SRC/tiny386" ] && cp "$TINY386_SRC/tiny386" "$W/emulator/bin/"
[ -f "$TINY386_SRC/LICENSE" ] && cp "$TINY386_SRC/LICENSE" "$W/emulator/LICENSE"
cat > "$W/emulator/README.md" <<EOR
# Tiny386 for QDOS/386 R13 (unchanged from R12C)

Upstream Tiny386 with \`patches/tiny386-fix1-shutdown.patch\` (R11M-FIX1: guest 0x8900 shutdown, checked image flush and
close) and \`patches/tiny386-r12c-flush.patch\` (R12C: ATA FLUSH CACHE 0xE7/0xEA = runtime fflush + fsync of the image
without closing it; failures reported to the guest as ERR, error register 0x40; fault injection TINY386_FLUSH_FAIL=N,
N- and TINY386_FLUSH_FAIL_STAGE=fflush for tests).  \`src/\` holds the changed files, \`bin/\` the binaries built with
\`make USE_SLIRP=n tiny386_headless tiny386\` (docs/R11M-FIX1-EMULATOR-BUILD.md).  See docs/R12C-SYNC-DURABILITY.md.
EOR
mkdir -p "$W/logs"; cp -a logs/r13 "$W/logs/"
cat > "$W/PACKAGE.md" <<EOP
# $NAME

Base: frozen R12C \`7abb678b24764541845ef936adc45e9110f1a52b\` (archive \`96e7f6fc...\`). Branch \`r13-virtual-displays\`.
Final source commit: \`$COMMIT\`. The SHA-256 of this archive is printed by the packaging script and reported with
the delivery (it cannot be inside the archive); \`SHA256SUMS\` lists every other file.

Read \`docs/R13-RUNTIME-REPORT.md\` first, then \`docs/R13-KNOWN-LIMITATIONS.md\`, \`docs/R13-DISPLAY-ABI.md\`,
\`docs/R13-FONT-PROVENANCE.md\`, \`docs/R13-MEMORY-BUDGET.md\`, \`docs/R13-AUDIT.md\`.

**Everything still runs at CPL 0: nothing here is security isolation.**

Contents
- source tree (git archive of \`$COMMIT\`): kernel (\`kernel/surface.c\`, \`kernel/gsurf.c\`, \`kernel/font.c\`,
  \`kernel/font_minerva.c\` (generated), \`platform/pc/vgapres.c\` are new; sessions in \`kernel/vcon.c\`), shell
  (\`apps/shell\`, -DR13_SHELL), SDK (\`sdk/lib/qdossurf.c\`, \`sdk/examples/r13\`), font extractor
  (\`tools/r13-font\`), \`third_party/minerva\` (the two Minerva 1.98 font modules, GPL-2.0, and COPYING), tests
  (\`tests/r13\`), docs, \`fs/r13-system/ETC/STARTUP.CMD\`
- \`build/r13/pc-vga/r13-vga.img\` (QEMU kernel, Minerva font), \`build/r13/pc/r13.img\` (headless/Tiny386 kernel),
  \`build/r13/pc-vga-q5/r13-vga-q5.img\` (QEMU kernel with the QDOS5x9 fallback font, no GPL material),
  \`build/apps/shell-d.aout\`; the unchanged R12C, R12B, R12A and R12 kernels and shells; firmware
- \`guest-programs/\`: the R13 builds (SURFT, COLOUR, BOUNCE, TITLET, BASIC, EDIT, ...); \`guest-programs-r12b/\` and
  \`guest-programs-r12a/\`: the frozen binaries used by the regression runs
- \`volumes/r13-system.img\`: the example system volume (R12B layout, R13 shell, SURFT/COLOUR/BOUNCE in /BIN)
- \`emulator/\`: Tiny386 with the R11M-FIX1 and R12C patches (unchanged from R12C)
- \`logs/r13/\`: per-stage transcripts, judge output, QEMU screendumps (PNG), Tiny386 evidence, image checks

Launch: \`sh scripts/r12b-launch-qemu.sh volumes/r13-system.img build/r13/pc-vga/r13-vga.img\`, then e.g.
\`START COLOUR /NEW\`, \`START BOUNCE /NEW\` and Ctrl+Esc. Gates: \`sh tests/r13/run-host.sh\`, \`run-s1.sh OUT\` ...
\`run-s5.sh\`, \`run-tiny.sh\`, \`run-regressions.sh\`.

The default R13 kernels contain the Minerva 1.98 font data (GPL-2.0): see docs/R13-FONT-PROVENANCE.md before
distributing them; \`r13-vga-q5.img\` carries none. SuperBASIC (\`apps/superbasic\`, \`BASIC.AOU\`, \`BASTEST.AOU\`) is
private research code and not freely redistributable; \`apps/superbasic/docs/PROVENANCE-LICENSING.md\` and
\`SHA256SUMS.sb4\` apply. The example system volume contains \`/BIN/BASIC.AOU\`: do not publish it as a public image
without removing it.
EOP
( cd "$W" && find . -type f ! -name SHA256SUMS | LC_ALL=C sort | sed 's#^\./##' | xargs -d '\n' sha256sum > SHA256SUMS )
( cd "$OUT" && tar -czf $NAME.tar.gz $NAME )
sha256sum "$OUT/$NAME.tar.gz" | tee "$OUT/$NAME.tar.gz.sha256"
