#!/bin/sh
# R12C: assemble qdos386-R12C-TASK-MANAGER-SYNC.tar.gz from a COMMITTED tree (branch r12c-task-sync).
#   usage: TINY386_SRC=<R12C Tiny386 tree> sh scripts/r12c-package.sh OUTDIR
# HEAD is archived; build products (R12C kernels and shell; the unchanged R12B, R12A, R12 kernels and shells),
# guest programs (R12C builds; the frozen R12B and R12A binaries), the example system volume, the Tiny386 R12C
# emulator sources and binaries, firmware and logs/r12c are added.  Prints the archive SHA-256.
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd); cd "$ROOT"
OUT=${1:?usage: r12c-package.sh OUTDIR}; : "${QDOS386_TOOLS:?}" "${TINY386_SRC:?}"
git diff --quiet HEAD -- . ':!build' ':!logs' || { echo "tree not clean: commit first" >&2; exit 2; }
COMMIT=$(git rev-parse HEAD); NAME=qdos386-R12C-TASK-MANAGER-SYNC
W=$OUT/$NAME; rm -rf "$W"; mkdir -p "$W"
git archive HEAD | tar -x -C "$W"
cp_() { [ -e "$1" ] || { echo "missing $1" >&2; exit 3; }; mkdir -p "$W/$2"; cp -a "$1" "$W/$2/"; }
for l in sdk/lib/libqdos.a sdk/lib/libc.a sdk/lib/libm.a sdk/lib/libgcc-qd.a; do cp_ $l sdk/lib; done
for f in build/r12c/pc-vga/r12c-vga.img build/r12c/pc/r12c.img build/r12b/pc-vga/r12b-vga.img build/r12b/pc/r12b.img \
         build/r12a/pc-vga/r12a-vga.img build/r12a/pc/r12a.img build/r12/pc-pg/r12-pg.img build/r12/pc-pg-vga/r12-pg-vga.img \
         build/apps/shell-c.aout build/apps/shell-b.aout build/apps/shell-a.aout build/apps/shell-m.aout \
         build/pc-r11f-none-run/bios.bin build/pc-r11f-none-run/vgabios.bin; do
  cp_ $f "$(dirname "$f")"
done
mkdir -p "$W/guest-programs" "$W/guest-programs-r12b" "$W/guest-programs-r12a"
for p in TITLET SYNCT OLDT ENVT NEGDYN BASIC BASTEST MAPTEST BIGBSS HEAPT STKT FAULTT SPAWNT CHILDT PINFO CONAPI CONDEMO EDIT \
         LEAK CONT1 COUNTER TICKER CONINP JOBT WRITER PATHT DIRFUL VFATT SHUTDOWN; do
  cp_ sdk/examples/$p.aout guest-programs
done
cp_ build/apps/PHOON.aout guest-programs
sh tests/r12c/frozen-bins.sh >/dev/null; cp -a build/frozen-r12b/*.aout "$W/guest-programs-r12b/"
sh tests/r12b/frozen-bins.sh >/dev/null; cp -a build/frozen-r12a/*.aout "$W/guest-programs-r12a/"
mkdir -p "$W/volumes"
sh scripts/r12c-sysvol.sh "$W/volumes/r12c-system.img" >/dev/null
mkdir -p "$W/emulator/src/rawdraw" "$W/emulator/src/sdl" "$W/emulator/bin"
for f in ide.c ide.h main.c pc.c pc.h rawdraw/main.c sdl/main.c; do cp "$TINY386_SRC/$f" "$W/emulator/src/$f"; done
cp "$TINY386_SRC/tiny386_headless" "$W/emulator/bin/"; [ -f "$TINY386_SRC/tiny386" ] && cp "$TINY386_SRC/tiny386" "$W/emulator/bin/"
[ -f "$TINY386_SRC/LICENSE" ] && cp "$TINY386_SRC/LICENSE" "$W/emulator/LICENSE"
cat > "$W/emulator/README.md" <<EOR
# Tiny386 for QDOS/386 R12C

Upstream Tiny386 with \`patches/tiny386-fix1-shutdown.patch\` (R11M-FIX1: guest 0x8900 shutdown, checked image flush and
close) and \`patches/tiny386-r12c-flush.patch\` (R12C: ATA FLUSH CACHE 0xE7/0xEA = runtime fflush + fsync of the image
without closing it; failures reported to the guest as ERR, error register 0x40; fault injection TINY386_FLUSH_FAIL=N,
N- and TINY386_FLUSH_FAIL_STAGE=fflush for tests).  \`src/\` holds the changed files, \`bin/\` the binaries built with
\`make USE_SLIRP=n tiny386_headless tiny386\` (docs/R11M-FIX1-EMULATOR-BUILD.md).  See docs/R12C-SYNC-DURABILITY.md.
EOR
mkdir -p "$W/logs"; cp -a logs/r12c "$W/logs/"
cat > "$W/PACKAGE.md" <<EOP
# $NAME

Base: frozen R12B \`27a5d49d9ba6f8f9436d392feb668efbdaa7b552\` (archive \`af43204b...\`). Branch \`r12c-task-sync\`.
Final source commit: \`$COMMIT\`. The SHA-256 of this archive is printed by the packaging script and reported with
the delivery (it cannot be inside the archive); \`SHA256SUMS\` lists every other file.

Read \`docs/R12C-RUNTIME-REPORT.md\` first, then \`docs/R12C-KNOWN-LIMITATIONS.md\`, \`docs/R12C-TASK-MANAGER-ABI.md\`,
\`docs/R12C-PROGRAM-TITLE-ABI.md\`, \`docs/R12C-SYNC-DURABILITY.md\`, \`docs/R12C-AUDIT.md\`.

**Everything still runs at CPL 0: nothing here is security isolation.**

Contents
- source tree (git archive of \`$COMMIT\`): kernel (\`kernel/taskmgr.c\`, \`kernel/sync.c\` are new; titles in
  \`kernel/jobctl.c\` and \`kernel/aout.c\`), shell (\`apps/shell/shell_r12c.inc\`), SDK (\`sdk/lib/qdostitle.c\`,
  \`sdk/lib/progname.c\`, crt0), tests (\`tests/r12c\`), docs, \`fs/r12c-system/ETC/STARTUP.CMD\`, emulator patches
- \`build/r12c/pc-vga/r12c-vga.img\` (QEMU kernel), \`build/r12c/pc/r12c.img\` (headless/Tiny386 kernel),
  \`build/apps/shell-c.aout\`; the unchanged R12B, R12A and R12 kernels and shells; firmware in \`build/pc-r11f-none-run/\`
- \`guest-programs/\`: the R12C builds (TITLET, SYNCT, OLDT built with the R12B crt0, BASIC, EDIT, ...);
  \`guest-programs-r12b/\` and \`guest-programs-r12a/\`: the frozen binaries used by the regression runs
- \`volumes/r12c-system.img\`: the example system volume (R12B layout, R12C shell and startup script)
- \`emulator/\`: Tiny386 with the R11M-FIX1 and R12C patches (sources of the changed files, binaries)
- \`logs/r12c/\`: per-stage transcripts, judge output, screen pictures, Tiny386 evidence, image checks, hashes

Launch: \`sh scripts/r12b-launch-qemu.sh volumes/r12c-system.img build/r12c/pc-vga/r12c-vga.img\`
(Ctrl+Esc opens the task selector; \`SYNC\`, \`TASKS /L\` in the shell).
Gates: \`sh tests/r12c/run-s1.sh OUT\` ... \`run-s4.sh\`, \`run-demo.sh\`, \`run-tiny.sh\`, \`run-regressions.sh\`.

SuperBASIC (\`apps/superbasic\`, \`BASIC.AOU\`, \`BASTEST.AOU\`) is private research code and not freely redistributable;
\`apps/superbasic/docs/PROVENANCE-LICENSING.md\` and \`SHA256SUMS.sb4\` apply. The example system volume contains
\`/BIN/BASIC.AOU\`: do not publish it as a public image without removing it.
EOP
( cd "$W" && find . -type f ! -name SHA256SUMS | LC_ALL=C sort | sed 's#^\./##' | xargs -d '\n' sha256sum > SHA256SUMS )
( cd "$OUT" && tar -czf $NAME.tar.gz $NAME )
sha256sum "$OUT/$NAME.tar.gz" | tee "$OUT/$NAME.tar.gz.sha256"
