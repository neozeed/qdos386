#!/bin/sh
# R16: assemble qdos386-R16-BKL-SMP-EXPERIMENT.tar.gz from a COMMITTED tree.
#   usage: sh scripts/r16-package.sh OUTDIR
# Full source (git archive of HEAD) + the R16 SMP kernels (opt-in CPL3/BKL/SMP), the ordinary UP kernels
# (= KCTX-FIX2, byte-identical) and the R15 AMP0 kernels (byte-identical) to show preservation, the CPL3 guest test
# program (plain + protected-marked), a public-safe volume carrying R16T (NO SuperBASIC/Infocom/story), the symbol
# map, the R16 patch against the KCTX-FIX2 mainline, the five design docs + HANDOVER.md, and SHA256SUMS.  Prints the
# archive SHA-256.  Optional extras if their env vars are set: TINY386_SRC (emulator), TOOLCHAIN_ZIP (toolchain src).
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd); cd "$ROOT"
OUT=${1:?}; : "${QDOS386_TOOLS:?}"
git diff --quiet HEAD -- . ':!build' ':!logs' || { echo "tree not clean: commit first" >&2; exit 2; }
COMMIT=$(git rev-parse HEAD); NAME=qdos386-R16-BKL-SMP-EXPERIMENT
BASE=kctx-fix2-mainline
W=$OUT/$NAME; rm -rf "$W"; mkdir -p "$W"
git archive HEAD | tar -x -C "$W"

# PUBLIC-SAFE SOURCE: strip the restricted porting sources before anything else.  The R16 mandate forbids a public
# archive containing private SuperBASIC or Infocom assets, so they are removed from the source tree as well as kept
# off the volume.  The kernel targets (qdos386-pc / -smp-pc / -amp-pc) do not compile these, so a fresh-extract
# rebuild of the kernels is unaffected.
rm -rf "$W/third_party/infocom" "$W/apps/superbasic"
# any story-file binaries (Infocom .z3..z8 or ZORK data), wherever they might sit
find "$W" -type f \( -iname '*.z[3-8]' -o -iname 'zork*.dat' -o -iname '*.zstory' \) -delete 2>/dev/null || true
cat > "$W/REMOVED-RESTRICTED.txt" <<'EOF'
This public archive has the restricted research/third-party sources removed:
  third_party/infocom/   the Task Force '87 Infocom interpreter port - no licence granted to redistribute
  apps/superbasic/       SuperBASIC - private research code, not freely redistributable
  any Infocom story files (*.z3..z8)
None are needed to build or run the R16 experiment: the kernel targets (make qdos386-pc, qdos386-smp-pc,
qdos386-amp-pc) and the CPL3 test program do not reference them, and the public volume (volumes/r16-public.img)
carries none of them. They live only in the private development tree.
EOF

# kernels: R16 (the experiment), the ordinary UP (= KCTX-FIX2), and R15 AMP0 - the last two must be byte-identical
make -B qdos386-smp-pc > /dev/null 2>&1
make -B qdos386-pc     > /dev/null 2>&1
make -B qdos386-amp-pc > /dev/null 2>&1
sh scripts/r16-build-guest.sh > /dev/null 2>&1
mkdir -p "$W/kernels" "$W/guest" "$W/volumes" "$W/patches"
cp build/qdos386-smp/pc-vga/qdos386-smp-vga.img build/qdos386-smp/pc/qdos386-smp.img "$W/kernels/"
cp build/qdos386/pc-vga/qdos386-vga.img        build/qdos386/pc/qdos386.img          "$W/kernels/"
cp build/qdos386-amp/pc-vga/qdos386-amp-vga.img build/qdos386-amp/pc/qdos386-amp.img "$W/kernels/"
python3 tools/kctx-symmap.py build/qdos386-smp/pc-vga/kernel.aout > "$W/kernels/qdos386-smp-vga.map" 2>/dev/null || true

# guest CPL3 test program: plain (ring-0 control) and protected-marked
cp build/r16/R16T.aout "$W/guest/R16T.aout"
cp build/r16/R16TP.aout "$W/guest/R16TP.aout"

# public-safe volume (NO SuperBASIC / Infocom / story), then add the protected R16T as R16T.AOU
sh scripts/qdos386-public-vol.sh "$W/volumes/r16-public.img" > /dev/null
T=$(mktemp -d); cp build/r16/R16TP.aout "$T/x"; touch -d '2026-10-30 08:00:00' "$T/x"
MTOOLS_SKIP_CHECK=1 mcopy -o -m -i "$W/volumes/r16-public.img" "$T/x" "::/BIN/R16T.AOU"; rm -rf "$T"
# refuse to ship a volume with any restricted asset
if MTOOLS_SKIP_CHECK=1 mdir -i "$W/volumes/r16-public.img" -/ 2>/dev/null | grep -i -q -E 'BASIC\.AOU|BASTEST|INFOCOM|ZORK|STORY|\.Z[3-8]'; then
	echo "REFUSING: restricted asset on the public volume" >&2; exit 3; fi
fsck.fat -n "$W/volumes/r16-public.img" > /dev/null 2>&1 || true

# the R16 patch against the KCTX-FIX2 mainline
git diff "$BASE" HEAD -- kernel/ Makefile sdk/ tools/r16-protect-mark.py scripts/r16-build-guest.sh tests/r16 > "$W/patches/r16-against-kctx-fix2.patch"
git diff --stat "$BASE" HEAD -- kernel/ Makefile sdk/ tools/r16-protect-mark.py scripts/r16-build-guest.sh tests/r16 > "$W/patches/CHANGED-FILES.txt"

# docs: the five design docs at the top level + HANDOVER.md
cp docs/R16-ARCHITECTURE.md docs/R16-CPL3-ABI.md docs/R16-AUDIT.md docs/R16-SMP-RUNTIME-REPORT.md docs/R16-KNOWN-LIMITATIONS.md "$W/"
cp docs/R16-HANDOVER.md "$W/HANDOVER.md"

# optional extras
if [ "${TINY386_SRC:-}" != "" ] && [ -d "${TINY386_SRC:-/nonexistent}" ]; then
	mkdir -p "$W/emulator/bin"; [ -f "$TINY386_SRC/tiny386_headless" ] && cp "$TINY386_SRC/tiny386_headless" "$W/emulator/bin/" || true
fi
if [ "${TOOLCHAIN_ZIP:-}" != "" ] && [ -f "${TOOLCHAIN_ZIP:-/nonexistent}" ]; then
	mkdir -p "$W/toolchain"; cp "$TOOLCHAIN_ZIP" "$W/toolchain/"; fi

# final gate: no restricted source tree or story-file asset anywhere in the package
if find "$W" -path '*/third_party/infocom*' -o -path '*/apps/superbasic*' -o -iname '*.z[3-8]' | grep -q .; then
	echo "REFUSING: a restricted SuperBASIC/Infocom asset remains in the package" >&2; exit 4; fi

( cd "$W" && find . -type f ! -name SHA256SUMS | LC_ALL=C sort | sed 's#^\./##' | xargs -d '\n' sha256sum > SHA256SUMS )
( cd "$OUT" && tar -czf $NAME.tar.gz $NAME )
sha256sum "$OUT/$NAME.tar.gz" | tee "$OUT/$NAME.tar.gz.sha256"
echo "commit $COMMIT, base $BASE"
