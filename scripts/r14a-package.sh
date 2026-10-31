#!/bin/sh
# R14A: assemble qdos386-R14A-HACK103.tar.gz from a COMMITTED tree (branch r14a-hack-ql-port).
#   usage: TINY386_SRC=<R12C Tiny386 tree> sh scripts/r14a-package.sh OUTDIR
# HEAD is archived (source, SDK with the C68 additions, the pristine QL Hack 1.03 tree, Makefile.qdos386, tests,
# docs, logs/r14a with screenshots and the QL oracle captures); build products are added: HACK.AOU with its link map,
# symbol report, build log and (empty) source diff, the C68 libc variant libc68.a, the shared libraries, the R14
# kernels (unchanged), the ANSIT and SPINM test programs, the R14 guest programs, the example volume and the
# unchanged Tiny386 emulator.  Prints the archive SHA-256.
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd); cd "$ROOT"
OUT=${1:?usage: r14a-package.sh OUTDIR}; : "${QDOS386_TOOLS:?}" "${TINY386_SRC:?}"
git diff --quiet HEAD -- . ':!build' ':!logs' || { echo "tree not clean: commit first" >&2; exit 2; }
COMMIT=$(git rev-parse HEAD); NAME=qdos386-R14A-HACK103
W=$OUT/$NAME; rm -rf "$W"; mkdir -p "$W"
git archive HEAD | tar -x -C "$W"
sh scripts/r14a-build-hack.sh build/r14a/hack > /dev/null
sh scripts/r14a-build-ansit.sh build/r14a/ANSIT.aout > /dev/null
sh sdk/bin/qdos386-gcc -O -o build/r14a/SPINM.aout tests/r14a/spinm.c > /dev/null
cp_() { [ -e "$1" ] || { echo "missing $1" >&2; exit 3; }; mkdir -p "$W/$2"; cp -a "$1" "$W/$2/"; }
for l in sdk/lib/libqdos.a sdk/lib/libc.a sdk/lib/libc68.a sdk/lib/libm.a sdk/lib/libgcc-qd.a; do cp_ $l sdk/lib; done
mkdir -p "$W/build/r14a/hack"
for f in src/HACK.AOU src/hack.map symbols.txt build.log source-changes.diff HACK.AOU.sha256; do
  cp build/r14a/hack/$f "$W/build/r14a/hack/"; done
for f in build/r14/pc-vga/r14-vga.img build/r14/pc/r14.img build/apps/shell-f.aout build/apps/sh.aout \
         build/r14/qlterm/QLTERM.aout build/r14a/ANSIT.aout build/r14a/SPINM.aout \
         build/pc-r11f-none-run/bios.bin build/pc-r11f-none-run/vgabios.bin; do
  cp_ $f "$(dirname "$f")"
done
mkdir -p "$W/guest-programs"
cp build/r14a/hack/src/HACK.AOU "$W/guest-programs/HACK.AOU"
cp build/r14/qlterm/QLTERM.aout "$W/guest-programs/QLTERM.aout"
for p in TERM XFER BOUNCE BIGBSS BASIC LEAK PINFO SHUTDOWN; do cp_ sdk/examples/$p.aout guest-programs; done
mkdir -p "$W/volumes"
sh scripts/r14a-sysvol.sh "$W/volumes/r14a-system.img" >/dev/null
mkdir -p "$W/emulator/src/rawdraw" "$W/emulator/src/sdl" "$W/emulator/bin"
for f in ide.c ide.h main.c pc.c pc.h rawdraw/main.c sdl/main.c; do cp "$TINY386_SRC/$f" "$W/emulator/src/$f"; done
cp "$TINY386_SRC/tiny386_headless" "$W/emulator/bin/"; [ -f "$TINY386_SRC/tiny386" ] && cp "$TINY386_SRC/tiny386" "$W/emulator/bin/"
[ -f "$TINY386_SRC/LICENSE" ] && cp "$TINY386_SRC/LICENSE" "$W/emulator/LICENSE"
printf '# Tiny386 for QDOS/386 R14A (unchanged from R12C .. R14)\n\nSee docs/R11M-FIX1-EMULATOR-BUILD.md; not changed for R14A.\n' > "$W/emulator/README.md"
cat > "$W/PACKAGE.md" <<EOP
# $NAME

Base: frozen R14 \`43a1f4481dc080bdd4604f5a53694c78e7739aa0\` (packaging \`2279de87\`, archive \`1dd86907...\`).
Branch \`r14a-hack-ql-port\`. Final source commit: \`$COMMIT\`. The archive's own SHA-256 is reported with the
delivery; \`SHA256SUMS\` lists every other file.

Read \`docs/R14A-HACK-RUNTIME-REPORT.md\` first, then \`docs/R14A-KNOWN-LIMITATIONS.md\`, \`docs/R14A-HACK-INTAKE.md\`,
\`docs/R14A-QL-C-ABI.md\`, \`docs/R14A-LIBVT-ANSI.md\`.

**Everything still runs at CPL 0: nothing here is security isolation.**

Contents
- source tree (git archive of \`$COMMIT\`): the supplied QL Hack 1.03 tree unchanged \`third_party/hack103-ql/orig\`
  (\`orig-SHA256SUMS\`; NetHack files of its \`test/\` withheld: \`orig-WITHHELD.txt\`), the opt-in
  \`third_party/hack103-ql/r14a/Makefile.qdos386\` (there is **no source patch**: 0 changed lines), the C68 additions
  \`sdk/qlcompat/include/{ansicondef.h,qptr.h}\`, \`sdk/qlcompat/c68/c68vt.c\`, the C68 libc variant (\`sdk/libc\`
  \`-DQDOS_C68_CONWRITE\` / \`-DQDOS_C68_POSIX\` → \`libc68.a\`), scripts \`scripts/r14a-*\`, tests \`tests/r14a\`,
  the QL screen OCR \`tools/r14a-qlocr.py\`, logs \`logs/r14a\` (gates, QL oracle captures, screenshots)
- \`build/r14a/hack/\`: \`HACK.AOU\`, \`hack.map\`, \`symbols.txt\`, \`build.log\`, \`source-changes.diff\` (empty)
- \`build/r14/*\`: the R14 kernels, unchanged (R14A needs no kernel change); \`build/r14a/ANSIT.aout\`, \`SPINM.aout\`
- \`guest-programs/\`; \`volumes/r14a-system.img\` (the R14 volume + \`/GAMES/HACK\`)
- \`emulator/\`: Tiny386 (unchanged)

Play: \`qemu-system-i386 -machine pc -cpu pentium -m 16 -drive file=build/r14/pc-vga/r14-vga.img,format=raw,if=ide,index=0 -drive
file=volumes/r14a-system.img,format=raw,if=ide,index=1\` (or the scripts in \`scripts/\`), then in the shell
\`CD /GAMES/HACK\` and \`HACK\` (or \`START HACK /NEW\` and Ctrl+Esc). Rebuild: \`sh scripts/r14a-build-hack.sh\`.
Tests: \`tests/r14a/run-gate1.sh\` .. \`run-gate4.sh\`, \`run-tiny.sh\`, \`oracle-ql.sh\` (needs QLBox R20),
\`check-r14-identical.sh\`.

Licences: Hack 1.0.3 © 1985 Stichting Centrum voor Wiskunde en Informatica and © 1982 Jay Fenlason (BSD-style), the
QL port files © 2022 neozeed (BSD-3-Clause): notices in \`third_party/hack103-ql/orig\` (\`copyright\`,
\`copyright-jf\`, \`LICENSE\`) and they travel with HACK.AOU. SuperBASIC (\`BASIC.AOU\`) is private research code and
not freely redistributable: the example volume contains \`/BIN/BASIC.AOU\`; do not publish it as a public image
without removing it. QLTerm is GPL-3.0-or-later (R14).
EOP
( cd "$W" && find . -type f ! -name SHA256SUMS | LC_ALL=C sort | sed 's#^\./##' | xargs -d '\n' sha256sum > SHA256SUMS )
( cd "$OUT" && tar -czf $NAME.tar.gz $NAME )
sha256sum "$OUT/$NAME.tar.gz" | tee "$OUT/$NAME.tar.gz.sha256"
