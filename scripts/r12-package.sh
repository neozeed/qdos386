#!/bin/sh
# R12: assemble qdos386-R12-INTERACTIVE-OS-STRETCH.tar.gz from a COMMITTED tree (branch r12-interactive).
#   usage: TINY386_SRC=<patched Tiny386 tree> SB4=<extracted SuperBASIC-C89-SB4> sh scripts/r12-package.sh OUTDIR
# HEAD is archived; the build products, guest programs, volumes, emulator binaries, firmware and logs/ (with the disk images) are added.
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd); cd "$ROOT"
OUT=${1:?usage: r12-package.sh OUTDIR}; : "${QDOS386_TOOLS:?}" "${TINY386_SRC:?}" "${SB4:?}"
git diff --quiet HEAD -- . ':!build' ':!logs' || { echo "tree not clean: commit first" >&2; exit 2; }
COMMIT=$(git rev-parse HEAD); NAME=qdos386-R12-INTERACTIVE-OS-STRETCH
W=$OUT/$NAME; rm -rf "$W"; mkdir -p "$W"
git archive HEAD | tar -x -C "$W"
cp_() { [ -e "$1" ] || { echo "missing $1" >&2; exit 3; }; mkdir -p "$W/$2"; cp -a "$1" "$W/$2/"; }
for l in sdk/lib/libqdos.a sdk/lib/libc.a sdk/lib/libm.a sdk/lib/libgcc-qd.a; do cp_ $l sdk/lib; done
for f in build/r12/pc-pg/r12-pg.img build/r12/pc-pg-vga/r12-pg-vga.img build/apps/shell-m.aout build/pc-r11f-none-run/bios.bin build/pc-r11f-none-run/vgabios.bin; do
  cp_ $f "$(dirname "$f")"
done
mkdir -p "$W/guest-programs"
for p in BASIC BASTEST CONAPI CONDEMO EDIT LEAK CONLOAD CONT1 COUNTER TICKER CONINP JOBT WRITER PATHT DIRFUL VFATT SHUTDOWN; do
  [ -e sdk/examples/$p.aout ] && cp -a sdk/examples/$p.aout "$W/guest-programs/" || { [ -e sdk/examples/r12/$p.aout ] && cp -a sdk/examples/r12/$p.aout "$W/guest-programs/" || echo "note: $p.aout not built, skipped" >&2; }
done
cp_ build/apps/PHOON.aout guest-programs
mkdir -p "$W/volumes"
sh scripts/r12-vol.sh "$W/volumes/r12-initial.img" 3072 >/dev/null
sh tests/r12/mk-s4-vol.sh "$W/volumes/r12-s4-initial.img" >/dev/null
mkdir -p "$W/emulator/src/rawdraw" "$W/emulator/src/sdl" "$W/emulator/bin"
for f in ide.c ide.h main.c pc.c pc.h rawdraw/main.c sdl/main.c; do cp "$TINY386_SRC/$f" "$W/emulator/src/$f"; done
cp "$TINY386_SRC/tiny386_headless" "$W/emulator/bin/"; [ -f "$TINY386_SRC/tiny386" ] && cp "$TINY386_SRC/tiny386" "$W/emulator/bin/"
[ -f "$TINY386_SRC/LICENSE" ] && cp "$TINY386_SRC/LICENSE" "$W/emulator/LICENSE"
cp docs/R11M-FIX1-EMULATOR-BUILD.md "$W/emulator/README.md" 2>/dev/null || true
mkdir -p "$W/logs"; cp -a logs/r12 "$W/logs/"
cat > "$W/PACKAGE.md" <<EOP
# $NAME

Base: frozen R11M-FIX1 \`6fbb9ff7c8a695d91a368a985a2b1da8b53a9bbc\`. Branch \`r12-interactive\`. Final source commit: \`$COMMIT\`.
The SHA-256 of this archive is printed by the packaging script and reported with the delivery (it cannot be inside the archive);
\`SHA256SUMS\` lists every other file.

Read \`docs/R12-RUNTIME-REPORT.md\` first, then \`docs/R12-KNOWN-LIMITATIONS.md\`, \`docs/R12-AUDIT.md\`, \`docs/R12-SHELL-ABI.md\`,
\`docs/R12-CONSOLE-EDITOR-ABI.md\`, \`docs/R12-SESSIONS-JOBS.md\`, \`docs/R12-SUPERBASIC-PORT.md\`, \`docs/R12-GRAPHICS-POC.md\` (Stage 5 deferred).

Contents
- source tree (git archive of \`$COMMIT\`): kernel, SDK, shell, editor, SuperBASIC port (\`apps/superbasic\`), scripts (\`scripts/r12-*\`), tests (\`tests/r12\`), docs
- \`build/r12/pc-pg-vga/r12-pg-vga.img\`: the QEMU kernel; \`build/r12/pc-pg/r12-pg.img\`: the headless/Tiny386 kernel; firmware in \`build/pc-r11f-none-run/\`
- \`guest-programs/\`: BASIC, EDIT, SDK utilities, Phoon (.aout; the volume names them .AOU)
- \`volumes/r12-initial.img\` (shell + editor + tools), \`volumes/r12-s4-initial.img\` (adds the SuperBASIC corpus and DEMO.BAS)
- \`emulator/\`: the patched Tiny386 sources and headless binary (Linux x86-64) from R11M-FIX1
- \`logs/r12/\`: per-stage and integrated transcripts, judge output, final disk images, fsck/fatwalk results

Launch: \`sh scripts/r12-launch-qemu.sh\` (interactive QEMU; Alt+F1..F4 switch consoles; \`DIR /W\`, \`EDIT\`, \`START BASIC.AOU /CON:2\`).
Demo check: \`sh tests/r12/run-demo.sh OUT\` (needs QEMU, mtools, fsck.fat, SB4).

SuperBASIC is private research code and not freely redistributable; \`apps/superbasic/docs/PROVENANCE-LICENSING.md\` and \`SHA256SUMS.sb4\` are shipped unchanged in meaning. Nothing here is a redistribution grant.
EOP
( cd "$W" && find . -type f ! -name SHA256SUMS | LC_ALL=C sort | sed 's#^\./##' | xargs -d '\n' sha256sum > SHA256SUMS )
( cd "$OUT" && tar -czf $NAME.tar.gz $NAME )
sha256sum "$OUT/$NAME.tar.gz" | tee "$OUT/$NAME.tar.gz.sha256"
