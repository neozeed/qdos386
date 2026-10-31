#!/bin/sh
# R13B: assemble qdos386-R13B-SHELL-USABILITY.tar.gz from a COMMITTED tree (branch r13b-shell-usability).
#   usage: TINY386_SRC=<R12C Tiny386 tree> sh scripts/r13b-package.sh OUTDIR
# HEAD is archived (source, SDK, Makefiles, tests, docs, logs/r13b); build products are added: the R13B kernels (QEMU,
# headless, and the three build-time epoch test kernels), the R13B boot shell shell-f and /BIN/SH.AOU, the frozen R13A
# kernels and shell-e (unchanged; the clockless kernel of Gate 4), libraries, guest programs, the example volume, the
# unchanged Tiny386 emulator and firmware.  Prints the archive SHA-256.
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd); cd "$ROOT"
OUT=${1:?usage: r13b-package.sh OUTDIR}; : "${QDOS386_TOOLS:?}" "${TINY386_SRC:?}"
git diff --quiet HEAD -- . ':!build' ':!logs' || { echo "tree not clean: commit first" >&2; exit 2; }
COMMIT=$(git rev-parse HEAD); NAME=qdos386-R13B-SHELL-USABILITY
W=$OUT/$NAME; rm -rf "$W"; mkdir -p "$W"
git archive HEAD | tar -x -C "$W"
cp_() { [ -e "$1" ] || { echo "missing $1" >&2; exit 3; }; mkdir -p "$W/$2"; cp -a "$1" "$W/$2/"; }
for l in sdk/lib/libqdos.a sdk/lib/libc.a sdk/lib/libm.a sdk/lib/libgcc-qd.a; do cp_ $l sdk/lib; done
for f in build/r13b/pc-vga/r13b-vga.img build/r13b/pc/r13b.img build/r13b/e2100/e2100.img build/r13b/e2101/e2101.img \
         build/r13b/e2106/e2106.img build/apps/shell-f.aout build/apps/sh.aout \
         build/r13a/pc-vga/r13a-vga.img build/r13a/pc/r13a.img build/apps/shell-e.aout \
         build/pc-r11f-none-run/bios.bin build/pc-r11f-none-run/vgabios.bin; do
  cp_ $f "$(dirname "$f")"
done
mkdir -p "$W/guest-programs"
for p in ARGV WHICHA WHICHB WHICHC TIMEQ GFXA SURFT COLOUR BOUNCE TITLET SYNCT BASIC EDIT CHILDT LEAK COUNTER PINFO SHUTDOWN; do
  cp_ sdk/examples/$p.aout guest-programs
done
mkdir -p "$W/volumes"
R13B_BIN_EXTRA="ARGV.AOU=sdk/examples/ARGV.aout TIMEQ.AOU=sdk/examples/TIMEQ.aout" sh scripts/r13b-sysvol.sh "$W/volumes/r13b-system.img" >/dev/null
mkdir -p "$W/emulator/src/rawdraw" "$W/emulator/src/sdl" "$W/emulator/bin"
for f in ide.c ide.h main.c pc.c pc.h rawdraw/main.c sdl/main.c; do cp "$TINY386_SRC/$f" "$W/emulator/src/$f"; done
cp "$TINY386_SRC/tiny386_headless" "$W/emulator/bin/"; [ -f "$TINY386_SRC/tiny386" ] && cp "$TINY386_SRC/tiny386" "$W/emulator/bin/"
[ -f "$TINY386_SRC/LICENSE" ] && cp "$TINY386_SRC/LICENSE" "$W/emulator/LICENSE"
cat > "$W/emulator/README.md" <<EOR
# Tiny386 for QDOS/386 R13B (unchanged from R12C, R13 and R13A)

Upstream Tiny386 with \`patches/tiny386-fix1-shutdown.patch\` (guest 0x8900 shutdown) and
\`patches/tiny386-r12c-flush.patch\` (ATA FLUSH CACHE).  \`src/\` holds the changed files, \`bin/\` the binaries built
with \`make USE_SLIRP=n tiny386_headless tiny386\` (docs/R11M-FIX1-EMULATOR-BUILD.md).
EOR
cat > "$W/PACKAGE.md" <<EOP
# $NAME

Base: frozen R13A \`951352c73d20f4be2f593c974fbb47e6061154aa\` (archive \`6c966941...\`). Branch \`r13b-shell-usability\`.
Final source commit: \`$COMMIT\`. The SHA-256 of this archive is reported with the delivery (it cannot be inside the
archive); \`SHA256SUMS\` lists every other file.

Read \`docs/R13B-RUNTIME-REPORT.md\` first, then \`docs/R13B-KNOWN-LIMITATIONS.md\`, \`docs/R13B-EXEC-RESOLUTION-ABI.md\`,
\`docs/R13B-TASK-LAUNCHER.md\`, \`docs/R13B-CLOCK-ABI.md\`, \`docs/R13B-AUDIT.md\`.

**Everything still runs at CPL 0: nothing here is security isolation.**

Contents
- source tree (git archive of \`$COMMIT\`): shell \`apps/shell\` (\`shell_r13b.inc\`: CLS, DATE, TIME, programs by name;
  boot shell -DR13B_SHELL at 0x200000, \`SH.AOU\` -DR13B_DYN at 0x240000); kernel \`kernel/rtc.c\` (CMOS RTC),
  \`kernel/clock_boot.c\`, \`kernel/taskmgr.c\` (F2), \`kernel/imgload.c\` (\`img_launch_as\`), \`kernel/vcon.c\` (focus
  return), all under \`-DQDOS_R13B\`; SDK examples \`sdk/examples/r13b\` (ARGV, TIMEQ); tests \`tests/r13b\` (gates,
  demo, Tiny386, regressions, host RTC vectors, Phoon host oracle); \`logs/r13b\`; docs
- \`build/r13b/pc-vga/r13b-vga.img\` (QEMU), \`build/r13b/pc/r13b.img\` (headless/Tiny386), \`build/r13b/e2100|e2101|e2106\`
  (build-time epoch test kernels), \`build/apps/shell-f.aout\` (boot shell), \`build/apps/sh.aout\` (SH.AOU); the frozen
  R13A kernels and shell-e, unchanged; firmware
- \`guest-programs/\`; \`volumes/r13b-system.img\` (R13B boot shell, /BIN/SH.AOU, ARGV, TIMEQ, the R13A fonts)
- \`emulator/\`: Tiny386 (unchanged from R12C)

Launch: \`sh scripts/r12b-launch-qemu.sh volumes/r13b-system.img build/r13b/pc-vga/r13b-vga.img\` (QEMU's RTC gives the
date), then e.g. \`DATE\`, \`PHOON\`, \`START SH /NEW\`, \`CLS\`, Ctrl+Esc then F2. Gates: \`sh tests/r13b/run-host.sh\`,
\`sh tests/r13b/run-gate.sh 1..4 OUT\`, \`run-demo.sh\`, \`run-tiny.sh\`, \`run-regressions.sh\`.

MINERVA.FNT (on the volume, optional) is Minerva 1.98's font data (GPL-2.0, third_party/minerva). SuperBASIC
(\`apps/superbasic\`, \`BASIC.AOU\`) is private research code and not freely redistributable;
\`apps/superbasic/docs/PROVENANCE-LICENSING.md\` applies. The example volume contains \`/BIN/BASIC.AOU\`: do not publish
it as a public image without removing it.
EOP
( cd "$W" && find . -type f ! -name SHA256SUMS | LC_ALL=C sort | sed 's#^\./##' | xargs -d '\n' sha256sum > SHA256SUMS )
( cd "$OUT" && tar -czf $NAME.tar.gz $NAME )
sha256sum "$OUT/$NAME.tar.gz" | tee "$OUT/$NAME.tar.gz.sha256"
