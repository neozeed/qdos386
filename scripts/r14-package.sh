#!/bin/sh
# R14: assemble qdos386-R14-SERIAL-QLTERM.tar.gz from a COMMITTED tree (branch r14-serial-qlterm).
#   usage: TINY386_SRC=<R12C Tiny386 tree> sh scripts/r14-package.sh OUTDIR
# HEAD is archived (source, SDK, the QL compatibility layer, QLTerm C R1 original source and the R14 patch, tests, host
# peers, docs, logs/r14 with screenshots); build products are added: the R14 kernels (QEMU, headless), the frozen R13B
# kernels and shells (unchanged), libraries, QLTerm for QDOS/386, the R14 guest programs, the example volume, the
# unchanged Tiny386 emulator and firmware.  Prints the archive SHA-256.
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd); cd "$ROOT"
OUT=${1:?usage: r14-package.sh OUTDIR}; : "${QDOS386_TOOLS:?}" "${TINY386_SRC:?}"
git diff --quiet HEAD -- . ':!build' ':!logs' || { echo "tree not clean: commit first" >&2; exit 2; }
COMMIT=$(git rev-parse HEAD); NAME=qdos386-R14-SERIAL-QLTERM
W=$OUT/$NAME; rm -rf "$W"; mkdir -p "$W"
git archive HEAD | tar -x -C "$W"
cp_() { [ -e "$1" ] || { echo "missing $1" >&2; exit 3; }; mkdir -p "$W/$2"; cp -a "$1" "$W/$2/"; }
for l in sdk/lib/libqdos.a sdk/lib/libc.a sdk/lib/libm.a sdk/lib/libgcc-qd.a; do cp_ $l sdk/lib; done
for f in build/r14/pc-vga/r14-vga.img build/r14/pc/r14.img \
         build/r13b/pc-vga/r13b-vga.img build/r13b/pc/r13b.img build/r13b/e2100/e2100.img build/r13b/e2101/e2101.img \
         build/r13b/e2106/e2106.img build/apps/shell-f.aout build/apps/sh.aout \
         build/r13a/pc-vga/r13a-vga.img build/r13a/pc/r13a.img build/apps/shell-e.aout \
         build/r14/qlterm/QLTERM.aout build/r14/qlterm/QLTERM-raw.aout build/r14/qlterm/source-changes.diff \
         build/pc-r11f-none-run/bios.bin build/pc-r11f-none-run/vgabios.bin; do
  cp_ $f "$(dirname "$f")"
done
mkdir -p "$W/guest-programs"
cp build/r14/uartprb.aout "$W/guest-programs/UARTPRB.aout"; cp build/r14/sertest.aout "$W/guest-programs/SERTEST.aout"
cp build/r14/qlprobe.aout "$W/guest-programs/QLPROBE.aout"; cp build/r14/qlterm/QLTERM.aout "$W/guest-programs/QLTERM.aout"
for p in TERM XFER ARGV TIMEQ GFXA SURFT COLOUR BOUNCE TITLET SYNCT BASIC EDIT CHILDT LEAK COUNTER PINFO SHUTDOWN; do
  cp_ sdk/examples/$p.aout guest-programs
done
mkdir -p "$W/volumes"
sh scripts/r14-sysvol.sh "$W/volumes/r14-system.img" >/dev/null
mkdir -p "$W/emulator/src/rawdraw" "$W/emulator/src/sdl" "$W/emulator/bin"
for f in ide.c ide.h main.c pc.c pc.h rawdraw/main.c sdl/main.c; do cp "$TINY386_SRC/$f" "$W/emulator/src/$f"; done
cp "$TINY386_SRC/tiny386_headless" "$W/emulator/bin/"; [ -f "$TINY386_SRC/tiny386" ] && cp "$TINY386_SRC/tiny386" "$W/emulator/bin/"
[ -f "$TINY386_SRC/LICENSE" ] && cp "$TINY386_SRC/LICENSE" "$W/emulator/LICENSE"
cat > "$W/emulator/README.md" <<EOR
# Tiny386 for QDOS/386 R14 (unchanged from R12C .. R13B)

Upstream Tiny386 with \`patches/tiny386-fix1-shutdown.patch\` (guest 0x8900 shutdown) and
\`patches/tiny386-r12c-flush.patch\` (ATA FLUSH CACHE).  \`src/\` holds the changed files, \`bin/\` the binaries built
with \`make USE_SLIRP=n tiny386_headless tiny386\` (docs/R11M-FIX1-EMULATOR-BUILD.md).  Not extended for R14: its
serial model is described in docs/R14-AUDIT.md 3.2.
EOR
cat > "$W/PACKAGE.md" <<EOP
# $NAME

Base: frozen R13B \`d9721897b4262a96a27e422e74c89cb5b428f06e\` (archive \`c13171d2...\`). Branch \`r14-serial-qlterm\`.
Final source commit: \`$COMMIT\`. The SHA-256 of this archive is reported with the delivery (it cannot be inside the
archive); \`SHA256SUMS\` lists every other file.

Read \`docs/R14-RUNTIME-REPORT.md\` first, then \`docs/R14-KNOWN-LIMITATIONS.md\`, \`docs/R14-SERIAL-ABI.md\`,
\`docs/R14-QL-COMPAT-ABI.md\`, \`docs/R14-QLTERM-PORT-MAP.md\`, \`docs/R14-AUDIT.md\`, \`docs/R14-LICENSE-PROVENANCE.md\`.

**Everything still runs at CPL 0: nothing here is security isolation.**

Contents
- source tree (git archive of \`$COMMIT\`): the UART driver \`kernel/uart.c\`, \`kernel/uart_s.s\` (\`-DQDOS_R14\` only);
  libqdos serial calls \`sdk/lib/qdosser.c\`; the QL compatibility layer \`sdk/qlcompat\`; QLTerm C R1 as supplied
  \`third_party/qlterm-c-r1/orig\` (GPL-3.0-or-later, unmodified) and the one-function patch
  \`third_party/qlterm-c-r1/r14-qdos386.patch\`; TERM and XFER \`sdk/examples/r14\`; host scripts
  \`scripts/r14-serial-args.sh\`, \`scripts/r14-qemu-serial.sh\`; build scripts \`scripts/r14-build-*.sh\`,
  \`tools/r14-qlterm-config.py\`; tests and host peers \`tests/r14\`; logs and screenshots \`logs/r14\`
- \`build/r14/pc-vga/r14-vga.img\` (QEMU), \`build/r14/pc/r14.img\` (headless/Tiny386); the frozen R13B and R13A
  kernels and shells, unchanged; firmware
- \`build/r14/qlterm/QLTERM.aout\` (QLTerm for QDOS/386, configured SER1I / 115K2 / WIN1_HOME/), \`QLTERM-raw.aout\`
  (the original configuration block), \`source-changes.diff\` (the whole source change)
- \`guest-programs/\`; \`volumes/r14-system.img\` (R13B system volume + QLTERM, TERM, XFER, SERTEST in /BIN)
- \`emulator/\`: Tiny386 (unchanged)

Launch with COM1 on the host: \`sh scripts/r14-qemu-serial.sh volumes/r14-system.img tcp\` (prints the port; then
e.g. \`socat - TCP:127.0.0.1:PORT\`) or \`... pty\`; in QDOS/386: \`START QLTERM /NEW\` then \`CON 4\` (or Ctrl+Esc),
F3 for QLTerm's menu; or \`TERM SER1 115200\`. Tests: \`sh tests/r14/run-host.sh\`, \`run-gate1.sh\` .. \`run-gate5.sh\`
(gates 4 and 5 need lrzsz on the host), \`run-tiny.sh\`, \`run-regressions.sh\`, \`check-frozen.sh\`.

Licences: QLTerm C R1 and the QDOS/386 port of it (patch, compatibility layer as linked into QLTerm, build and test
scripts) are GPL-3.0-or-later; the original notices are in \`third_party/qlterm-c-r1/orig\`. MINERVA.FNT (on the
volume, optional) is Minerva 1.98's font data (GPL-2.0). SuperBASIC (\`apps/superbasic\`, \`BASIC.AOU\`) is private
research code and not freely redistributable; \`apps/superbasic/docs/PROVENANCE-LICENSING.md\` applies. The example
volume contains \`/BIN/BASIC.AOU\`: do not publish it as a public image without removing it.
EOP
( cd "$W" && find . -type f ! -name SHA256SUMS | LC_ALL=C sort | sed 's#^\./##' | xargs -d '\n' sha256sum > SHA256SUMS )
( cd "$OUT" && tar -czf $NAME.tar.gz $NAME )
sha256sum "$OUT/$NAME.tar.gz" | tee "$OUT/$NAME.tar.gz.sha256"
