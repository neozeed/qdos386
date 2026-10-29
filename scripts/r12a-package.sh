#!/bin/sh
# R12A: assemble qdos386-R12A-DYNAMIC-PROCESSES.tar.gz from a COMMITTED tree (branch r12a-dynamic-processes).
#   usage: TINY386_SRC=<patched Tiny386 tree> SB4=<extracted SuperBASIC-C89-SB4> sh scripts/r12a-package.sh OUTDIR
# HEAD is archived; build products (R12A and R12 kernels), guest programs, volumes, emulator binaries, firmware and
# logs/ (with the disk images) are added.  Prints the archive SHA-256.
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd); cd "$ROOT"
OUT=${1:?usage: r12a-package.sh OUTDIR}; : "${QDOS386_TOOLS:?}" "${TINY386_SRC:?}" "${SB4:?}"
git diff --quiet HEAD -- . ':!build' ':!logs' || { echo "tree not clean: commit first" >&2; exit 2; }
COMMIT=$(git rev-parse HEAD); NAME=qdos386-R12A-DYNAMIC-PROCESSES
W=$OUT/$NAME; rm -rf "$W"; mkdir -p "$W"
git archive HEAD | tar -x -C "$W"
cp_() { [ -e "$1" ] || { echo "missing $1" >&2; exit 3; }; mkdir -p "$W/$2"; cp -a "$1" "$W/$2/"; }
for l in sdk/lib/libqdos.a sdk/lib/libc.a sdk/lib/libm.a sdk/lib/libgcc-qd.a; do cp_ $l sdk/lib; done
for f in build/r12a/pc-vga/r12a-vga.img build/r12a/pc/r12a.img build/r12/pc-pg/r12-pg.img build/r12/pc-pg-vga/r12-pg-vga.img \
         build/apps/shell-a.aout build/apps/shell-m.aout build/pc-r11f-none-run/bios.bin build/pc-r11f-none-run/vgabios.bin; do
  cp_ $f "$(dirname "$f")"
done
mkdir -p "$W/guest-programs"
for p in MAPTEST BIGBSS HEAPT STKT FAULTT SPAWNT CHILDT PINFO BASIC BASTEST CONAPI CONDEMO EDIT LEAK CONT1 COUNTER TICKER CONINP JOBT \
         WRITER PATHT DIRFUL VFATT SHUTDOWN; do
  cp_ sdk/examples/$p.aout guest-programs
done
cp_ build/apps/PHOON.aout guest-programs
mkdir -p "$W/volumes"
sh scripts/r12a-vol.sh "$W/volumes/r12a-initial.img" >/dev/null
sh tests/r12a/mk-demo-vol.sh "$W/volumes/r12a-demo-initial.img" >/dev/null
mkdir -p "$W/emulator/src/rawdraw" "$W/emulator/src/sdl" "$W/emulator/bin"
for f in ide.c ide.h main.c pc.c pc.h rawdraw/main.c sdl/main.c; do cp "$TINY386_SRC/$f" "$W/emulator/src/$f"; done
cp "$TINY386_SRC/tiny386_headless" "$W/emulator/bin/"; [ -f "$TINY386_SRC/tiny386" ] && cp "$TINY386_SRC/tiny386" "$W/emulator/bin/"
[ -f "$TINY386_SRC/LICENSE" ] && cp "$TINY386_SRC/LICENSE" "$W/emulator/LICENSE"
cp docs/R11M-FIX1-EMULATOR-BUILD.md "$W/emulator/README.md" 2>/dev/null || true
mkdir -p "$W/logs"; cp -a logs/r12a "$W/logs/"
cat > "$W/PACKAGE.md" <<EOP
# $NAME

Base: frozen R12 \`0e92a742291a47067823d42783ee3eb366d02b83\`. Branch \`r12a-dynamic-processes\`. Final source commit: \`$COMMIT\`.
The SHA-256 of this archive is printed by the packaging script and reported with the delivery (it cannot be inside the
archive); \`SHA256SUMS\` lists every other file.

Read \`docs/R12A-RUNTIME-REPORT.md\` first, then \`docs/R12A-KNOWN-LIMITATIONS.md\`, \`docs/R12A-PROCESS-ABI.md\`,
\`docs/R12A-MEMORY-MAP.md\`, \`docs/R12A-INTERRUPT-STACKS.md\`, \`docs/R12A-MEMORY-AUDIT.md\`, \`docs/R12A-MIGRATION-DESIGN.md\`.

**Private process memory is not security isolation: everything still runs at CPL 0.**

Contents
- source tree (git archive of \`$COMMIT\`): kernel (\`kernel/proc.c vas.c frames.c pheap.c kctx.s\` are new), SDK
  (\`sdk/lib/qdosproc.c\`), tests (\`tests/r12a\`, \`tests/host/r12a\`), docs
- \`build/r12a/pc-vga/r12a-vga.img\` (QEMU kernel), \`build/r12a/pc/r12a.img\` (headless/Tiny386 kernel); the unchanged R12 kernels
  \`build/r12/pc-pg*/\` for comparison; firmware in \`build/pc-r11f-none-run/\`
- \`guest-programs/\`: R12A tests (MAPTEST, BIGBSS, HEAPT, STKT, FAULTT, SPAWNT, CHILDT) and the unchanged R12 programs
- \`volumes/r12a-initial.img\`, \`volumes/r12a-demo-initial.img\` (adds the SuperBASIC corpus)
- \`emulator/\`: patched Tiny386 sources and headless binary (from R11M-FIX1)
- \`logs/r12a/\`: per-stage transcripts, judge output, final disk images

Launch: \`sh scripts/r12a-launch-qemu.sh\`.  Host tests: \`sh tests/host/r12a/run.sh\`.
Gates: \`sh tests/r12a/run-s1.sh OUT\` … \`run-s3.sh\`, \`SB4=... sh tests/r12a/run-demo.sh OUT\`.

SuperBASIC is private research code and not freely redistributable; \`apps/superbasic/docs/PROVENANCE-LICENSING.md\` and
\`SHA256SUMS.sb4\` are shipped unchanged in meaning.
EOP
( cd "$W" && find . -type f ! -name SHA256SUMS | LC_ALL=C sort | sed 's#^\./##' | xargs -d '\n' sha256sum > SHA256SUMS )
( cd "$OUT" && tar -czf $NAME.tar.gz $NAME )
sha256sum "$OUT/$NAME.tar.gz" | tee "$OUT/$NAME.tar.gz.sha256"
