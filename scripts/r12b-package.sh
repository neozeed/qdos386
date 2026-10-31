#!/bin/sh
# R12B: assemble qdos386-R12B-SYSTEM-ENV-SUPERBASIC.tar.gz from a COMMITTED tree (branch r12b-system-environment).
#   usage: TINY386_SRC=<patched Tiny386 tree> SB4=<extracted SuperBASIC-C89-SB4> sh scripts/r12b-package.sh OUTDIR
# HEAD is archived; build products (R12B, R12A and R12 kernels, shells), guest programs (R12B builds and the frozen R12A
# ones), the example system volume, emulator, firmware and logs/ are added.  Prints the archive SHA-256.
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd); cd "$ROOT"
OUT=${1:?usage: r12b-package.sh OUTDIR}; : "${QDOS386_TOOLS:?}" "${TINY386_SRC:?}" "${SB4:?}"
git diff --quiet HEAD -- . ':!build' ':!logs' || { echo "tree not clean: commit first" >&2; exit 2; }
COMMIT=$(git rev-parse HEAD); NAME=qdos386-R12B-SYSTEM-ENV-SUPERBASIC
W=$OUT/$NAME; rm -rf "$W"; mkdir -p "$W"
git archive HEAD | tar -x -C "$W"
cp_() { [ -e "$1" ] || { echo "missing $1" >&2; exit 3; }; mkdir -p "$W/$2"; cp -a "$1" "$W/$2/"; }
for l in sdk/lib/libqdos.a sdk/lib/libc.a sdk/lib/libm.a sdk/lib/libgcc-qd.a; do cp_ $l sdk/lib; done
for f in build/r12b/pc-vga/r12b-vga.img build/r12b/pc/r12b.img build/r12a/pc-vga/r12a-vga.img build/r12a/pc/r12a.img \
         build/r12/pc-pg/r12-pg.img build/r12/pc-pg-vga/r12-pg-vga.img build/apps/shell-b.aout build/apps/shell-a.aout \
         build/apps/shell-m.aout build/pc-r11f-none-run/bios.bin build/pc-r11f-none-run/vgabios.bin; do
  cp_ $f "$(dirname "$f")"
done
mkdir -p "$W/guest-programs" "$W/guest-programs-r12a"
for p in ENVT NEGDYN BASIC BASTEST MAPTEST BIGBSS HEAPT STKT FAULTT SPAWNT CHILDT PINFO CONAPI CONDEMO EDIT LEAK CONT1 COUNTER \
         TICKER CONINP JOBT WRITER PATHT DIRFUL VFATT SHUTDOWN; do
  cp_ sdk/examples/$p.aout guest-programs
done
cp_ build/apps/PHOON.aout guest-programs
sh tests/r12b/frozen-bins.sh >/dev/null
cp -a build/frozen-r12a/*.aout "$W/guest-programs-r12a/"
mkdir -p "$W/volumes"
sh scripts/r12b-sysvol.sh "$W/volumes/r12b-system.img" >/dev/null
mkdir -p "$W/emulator/src/rawdraw" "$W/emulator/src/sdl" "$W/emulator/bin"
for f in ide.c ide.h main.c pc.c pc.h rawdraw/main.c sdl/main.c; do cp "$TINY386_SRC/$f" "$W/emulator/src/$f"; done
cp "$TINY386_SRC/tiny386_headless" "$W/emulator/bin/"; [ -f "$TINY386_SRC/tiny386" ] && cp "$TINY386_SRC/tiny386" "$W/emulator/bin/"
[ -f "$TINY386_SRC/LICENSE" ] && cp "$TINY386_SRC/LICENSE" "$W/emulator/LICENSE"
cp docs/R11M-FIX1-EMULATOR-BUILD.md "$W/emulator/README.md" 2>/dev/null || true
mkdir -p "$W/logs"; cp -a logs/r12b "$W/logs/"
cat > "$W/PACKAGE.md" <<EOP
# $NAME

Base: frozen R12A \`6e30dd7046b4d09d77ec398bea38a76318b36c52\`. Branch \`r12b-system-environment\`. Final source commit:
\`$COMMIT\`. The SHA-256 of this archive is printed by the packaging script and reported with the delivery (it cannot be
inside the archive); \`SHA256SUMS\` lists every other file.

Read \`docs/R12B-RUNTIME-REPORT.md\` first, then \`docs/R12B-KNOWN-LIMITATIONS.md\`, \`docs/R12B-ENV-PROCESS-ABI.md\`,
\`docs/R12B-PATH-STARTUP-ABI.md\`, \`docs/R12B-SUPERBASIC-FILE-ABI.md\`, \`docs/R12B-SUPERBASIC-EXEC-ABI.md\`,
\`docs/R12B-AUDIT.md\`.

**Everything still runs at CPL 0: nothing here is security isolation.**

Contents
- source tree (git archive of \`$COMMIT\`): kernel (\`kernel/env.c\`, \`kernel/pathres.c\` are new), shell
  (\`apps/shell/shell_env.inc\`, \`shell_parse.inc\`), SDK (\`sdk/lib/qdosenv.c\`, \`sdk/libc/envx.c\`, getenv), SuperBASIC
  host words (\`apps/superbasic/qdos/qb_ext.c\`) and the core hooks (\`apps/superbasic/qdos386-core-changes.patch\`), tests
  (\`tests/r12b\`, \`tests/host/r12b\`), docs, the default \`fs/r12b-system/ETC/STARTUP.CMD\`
- \`build/r12b/pc-vga/r12b-vga.img\` (QEMU kernel), \`build/r12b/pc/r12b.img\` (headless/Tiny386 kernel), \`build/apps/shell-b.aout\`;
  the unchanged R12A and R12 kernels and shells for comparison; firmware in \`build/pc-r11f-none-run/\`
- \`guest-programs/\`: the R12B builds (ENVT, NEGDYN, BASIC with LOAD/SAVE/EXEC, BASTEST, ...);
  \`guest-programs-r12a/\`: the frozen R12A binaries used by the regression runs (hashes as in the R12A package)
- \`volumes/r12b-system.img\`: the example system volume (/BIN /ETC /SDK /TMP /HOME /GAMES, default STARTUP.CMD)
- \`emulator/\`: patched Tiny386 sources and headless binary (from R11M-FIX1)
- \`logs/r12b/\`: per-stage transcripts, judge output, screen captures, hashes

Launch: \`sh scripts/r12b-launch-qemu.sh\` (example system volume, R12B kernel).  Host tests: \`sh tests/host/r12b/run.sh\`.  Gates: \`sh tests/r12b/run-s1.sh OUT\` ... \`run-s4.sh\`,
\`sh tests/r12b/run-demo.sh OUT\`.

SuperBASIC (\`apps/superbasic\`, \`BASIC.AOU\`, \`BASTEST.AOU\`) is private research code and not freely redistributable;
\`apps/superbasic/docs/PROVENANCE-LICENSING.md\` and \`SHA256SUMS.sb4\` apply. The example system volume contains
\`/BIN/BASIC.AOU\`: do not publish it as a public image without removing it.
EOP
( cd "$W" && find . -type f ! -name SHA256SUMS | LC_ALL=C sort | sed 's#^\./##' | xargs -d '\n' sha256sum > SHA256SUMS )
( cd "$OUT" && tar -czf $NAME.tar.gz $NAME )
sha256sum "$OUT/$NAME.tar.gz" | tee "$OUT/$NAME.tar.gz.sha256"
