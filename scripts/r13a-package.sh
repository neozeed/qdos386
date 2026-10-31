#!/bin/sh
# R13A: assemble qdos386-R13A-GRAPHICS-USABILITY.tar.gz from a COMMITTED tree (branch r13a-graphics-usability).
#   usage: TINY386_SRC=<R12C Tiny386 tree> sh scripts/r13a-package.sh OUTDIR
# HEAD is archived (source, font converters and sources, fonts/, third_party/minerva with MINERVA.FNT, tests, docs,
# logs/r13a); build products are added: the R13A kernels (no Minerva bytes: the fallback-only build) and shell-e, the
# frozen R13 kernels and shell-d (unchanged, for comparison), the R12C kernels and shell-c (regression base),
# libraries, the guest programs, two example volumes (with and without the optional Minerva font), the unchanged
# Tiny386 emulator and firmware.  Prints the archive SHA-256.
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd); cd "$ROOT"
OUT=${1:?usage: r13a-package.sh OUTDIR}; : "${QDOS386_TOOLS:?}" "${TINY386_SRC:?}"
git diff --quiet HEAD -- . ':!build' ':!logs' || { echo "tree not clean: commit first" >&2; exit 2; }
COMMIT=$(git rev-parse HEAD); NAME=qdos386-R13A-GRAPHICS-USABILITY
W=$OUT/$NAME; rm -rf "$W"; mkdir -p "$W"
git archive HEAD | tar -x -C "$W"
cp_() { [ -e "$1" ] || { echo "missing $1" >&2; exit 3; }; mkdir -p "$W/$2"; cp -a "$1" "$W/$2/"; }
for l in sdk/lib/libqdos.a sdk/lib/libc.a sdk/lib/libm.a sdk/lib/libgcc-qd.a; do cp_ $l sdk/lib; done
for f in build/r13a/pc-vga/r13a-vga.img build/r13a/pc/r13a.img build/apps/shell-e.aout \
         build/r13/pc-vga/r13-vga.img build/r13/pc/r13.img build/r13/pc-vga-q5/r13-vga-q5.img build/apps/shell-d.aout \
         build/r12c/pc-vga/r12c-vga.img build/r12c/pc/r12c.img build/apps/shell-c.aout \
         build/pc-r11f-none-run/bios.bin build/pc-r11f-none-run/vgabios.bin; do
  cp_ $f "$(dirname "$f")"
done
mkdir -p "$W/guest-programs"
for p in GFXA SURFT COLOUR BOUNCE TITLET SYNCT BASIC EDIT CHILDT LEAK COUNTER PINFO SHUTDOWN; do
  cp_ sdk/examples/$p.aout guest-programs
done
mkdir -p "$W/volumes"
sh scripts/r13a-sysvol.sh "$W/volumes/r13a-system.img" >/dev/null
R13A_NO_MINERVA=1 sh scripts/r13a-sysvol.sh "$W/volumes/r13a-system-nominerva.img" >/dev/null
mkdir -p "$W/emulator/src/rawdraw" "$W/emulator/src/sdl" "$W/emulator/bin"
for f in ide.c ide.h main.c pc.c pc.h rawdraw/main.c sdl/main.c; do cp "$TINY386_SRC/$f" "$W/emulator/src/$f"; done
cp "$TINY386_SRC/tiny386_headless" "$W/emulator/bin/"; [ -f "$TINY386_SRC/tiny386" ] && cp "$TINY386_SRC/tiny386" "$W/emulator/bin/"
[ -f "$TINY386_SRC/LICENSE" ] && cp "$TINY386_SRC/LICENSE" "$W/emulator/LICENSE"
cat > "$W/emulator/README.md" <<EOR
# Tiny386 for QDOS/386 R13A (unchanged from R12C and R13)

Upstream Tiny386 with \`patches/tiny386-fix1-shutdown.patch\` (guest 0x8900 shutdown) and
\`patches/tiny386-r12c-flush.patch\` (ATA FLUSH CACHE).  \`src/\` holds the changed files, \`bin/\` the binaries built
with \`make USE_SLIRP=n tiny386_headless tiny386\` (docs/R11M-FIX1-EMULATOR-BUILD.md).
EOR
cat > "$W/PACKAGE.md" <<EOP
# $NAME

Base: frozen R13 \`c3683d58e02817570f66919aead2451dd13e51a6\` (archive \`bac427c6...\`). Branch
\`r13a-graphics-usability\`. Final source commit: \`$COMMIT\`. The SHA-256 of this archive is reported with the delivery
(it cannot be inside the archive); \`SHA256SUMS\` lists every other file.

Read \`docs/R13A-RUNTIME-REPORT.md\` first, then \`docs/R13A-KNOWN-LIMITATIONS.md\`, \`docs/R13A-GRAPHICS-ABI.md\`,
\`docs/R13A-FONT-FORMAT.md\`, \`docs/R13A-FONT-PROVENANCE.md\`, \`docs/R13A-AUDIT.md\`.

**Everything still runs at CPL 0: nothing here is security isolation.**

Contents
- source tree (git archive of \`$COMMIT\`): font registry \`kernel/fontreg.c\`, generated built-in fonts
  \`kernel/font_recovery.c\` / \`kernel/font_console.c\` (public-domain / unrestricted X11 fonts, sources and notice in
  \`fonts/src\`), key events \`kernel/kev.c\`, R13A presenter \`platform/pc/vgapres13a.c\`, surface/session changes
  under \`-DQDOS_R13A\`; shell \`apps/shell/shell_r13a.inc\` (-DR13A_SHELL); SDK \`sdk/lib/qdosgfx.c\`,
  \`sdk/include/qdos386.h\`, \`sdk/examples/r13a/gfxa\`; font tools \`tools/r13a-font\` (QFNT writer/parser, PCF and
  Minerva converters, broken-file generator, Minerva scanner); \`fonts/\` (RECOVERY.FNT, CON8X16.FNT);
  \`third_party/minerva/\` (the GPL-2.0 font modules, COPYING and the optional **MINERVA.FNT**); tests \`tests/r13a\`;
  \`logs/r13a\` (transcripts, judge output, screendumps as PNG, Tiny386 evidence, image checks)
- \`build/r13a/pc-vga/r13a-vga.img\` (QEMU kernel), \`build/r13a/pc/r13a.img\` (headless/Tiny386 kernel) - both the
  fallback-only build: no Minerva bytes; \`build/apps/shell-e.aout\` (R13A shell); the frozen R13 kernels and
  shell-d and the R12C kernels and shell-c, unchanged; firmware
- \`guest-programs/\`: GFXA and the programs the tests use (R13A tree builds)
- \`volumes/r13a-system.img\` (R13A shell, /SYSTEM/FONTS with RECOVERY, CON8X16 and the optional MINERVA.FNT + notice)
  and \`volumes/r13a-system-nominerva.img\` (the same without Minerva)
- \`emulator/\`: Tiny386 (unchanged from R12C)

Launch: \`sh scripts/r12b-launch-qemu.sh volumes/r13a-system.img build/r13a/pc-vga/r13a-vga.img\`, then e.g.
\`CONSOLE GFX /ALL\`, \`FONT /SYSTEM/FONTS/MINERVA.FNT /CON\`, \`START GFXA pattern 0 /NEW\`, \`START COLOUR /NEW\`,
Ctrl+Esc. Gates: \`sh tests/r13a/run-host.sh\`, \`sh tests/r13a/run-gate.sh 1..4 OUT\`, \`run-demo.sh\`, \`run-tiny.sh\`,
\`run-regressions.sh\`.

MINERVA.FNT is Minerva 1.98's font data (GPL-2.0, see third_party/minerva/COPYING and docs/R13A-FONT-PROVENANCE.md);
it is optional and the R13A kernels do not contain it. SuperBASIC (\`apps/superbasic\`, \`BASIC.AOU\`) is private
research code and not freely redistributable; \`apps/superbasic/docs/PROVENANCE-LICENSING.md\` applies. Both example
volumes contain \`/BIN/BASIC.AOU\`: do not publish them as public images without removing it.
EOP
( cd "$W" && find . -type f ! -name SHA256SUMS | LC_ALL=C sort | sed 's#^\./##' | xargs -d '\n' sha256sum > SHA256SUMS )
( cd "$OUT" && tar -czf $NAME.tar.gz $NAME )
sha256sum "$OUT/$NAME.tar.gz" | tee "$OUT/$NAME.tar.gz.sha256"
