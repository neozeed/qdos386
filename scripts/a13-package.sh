#!/bin/sh
# Apollo 13 (R17): assemble qdos386-APOLLO13-R17-MEM-MULTICORE.tar.gz from a COMMITTED tree.
#   usage: sh scripts/a13-package.sh OUTDIR
# Contents:
#   - full buildable source, public-safe (SuperBASIC/Infocom stripped; no Elite: it is a separate private application);
#   - the opt-in R17 kernels, plus the byte-identical R16B, R16A and KCTX-FIX2 kernels;
#   - the CPL3 guest tools;
#   - a public-safe example system volume (superfloppy and MBR);
#   - the patch against R16B;
#   - small evidence summaries;
#   - the Apollo 13 docs and HANDOVER.md;
#   - SHA256SUMS.
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd); cd "$ROOT"
OUT=${1:?}; : "${QDOS386_TOOLS:?}"
export MTOOLS_SKIP_CHECK=1
git diff --quiet HEAD -- . ':!build' ':!logs' ':!dist' || { echo "tree not clean: commit first" >&2; exit 2; }
COMMIT=$(git rev-parse HEAD); NAME=qdos386-APOLLO13-R17-MEM-MULTICORE; BASE=refs/tags/r16b-foreground-smp
W=$OUT/$NAME; rm -rf "$W"; mkdir -p "$W"
git archive HEAD | tar -x -C "$W"
rm -rf "$W/third_party/infocom" "$W/apps/superbasic" "$W/logs"
find "$W" -type f \( -iname '*.z[3-8]' -o -iname 'zork*.dat' -o -iname '*.zstory' \) -delete 2>/dev/null || true
cat > "$W/REMOVED-RESTRICTED.txt" <<'EOF'
Restricted research/third-party sources removed from this public archive: third_party/infocom/ (Infocom interpreter
port, no redistribution licence) and apps/superbasic/ (private research).  EliteQL is a separate private application
and is not part of this source tree.  None of them is needed to build or run R17.  The development logs/ tree is
replaced by the small evidence/ summaries.
EOF

make -B qdos386-r17-pc  > /dev/null 2>&1
make -B qdos386-r16b-pc > /dev/null 2>&1
make -B qdos386-smp-pc  > /dev/null 2>&1
make -B qdos386-pc      > /dev/null 2>&1
sh scripts/a13-build-guest.sh > /dev/null 2>&1
K=$W/kernels; mkdir -p "$K/r16b-unchanged" "$K/r16a-unchanged" "$K/default-unchanged" "$W/guest" "$W/examples" "$W/patches" "$W/evidence"
cp build/qdos386-r17/pc-vga/qdos386-r17-vga.img build/qdos386-r17/pc/qdos386-r17.img "$K/"
cp build/qdos386-r16b/pc-vga/qdos386-r16b-vga.img build/qdos386-r16b/pc/qdos386-r16b.img "$K/r16b-unchanged/"
cp build/qdos386-smp/pc-vga/qdos386-smp-vga.img build/qdos386-smp/pc/qdos386-smp.img "$K/r16a-unchanged/"
cp build/qdos386/pc-vga/qdos386-vga.img build/qdos386/pc/qdos386.img "$K/default-unchanged/"
python3 tools/kctx-symmap.py build/qdos386-r17/pc-vga/kernel.aout > "$K/qdos386-r17-vga.map" 2>/dev/null || true
cp build/a13/MEMINFOP.aout build/a13/RAMCHKP.aout build/a13/SMPTP.aout build/a13/TOP17P.aout build/a13/CPUSET17P.aout \
   build/r16/R16TP.aout build/r16b/BOUNCEP.aout "$W/guest/"

sh scripts/a13-sysvol.sh "$OUT/_pub.img" > /dev/null
cp "$OUT/_pub.img" "$W/examples/system-r17-superfloppy.img"
sh scripts/r16a-wrap-mbr.sh "$OUT/_pub.img" "$W/examples/system-r17-mbr.img" > /dev/null
rm -f "$OUT/_pub.img"
for v in "$W/examples/"*.img; do
	img=$v; [ "${v%mbr.img}" != "$v" ] && { dd if="$v" bs=512 skip=2048 of="$OUT/_p.img" status=none; img=$OUT/_p.img; }
	if mdir -i "$img" -/ 2>/dev/null | grep -i -q -E 'BASIC\.AOU|BASTEST|INFOCOM|ZORK|STORY|\.Z[3-8]|ELITE'; then
		echo "REFUSING: restricted asset on $v" >&2; exit 3; fi
done
rm -f "$OUT/_p.img"

# evidence: the small text summaries only (no disk images, no raw transcripts)
for f in logs/a13/p1/*.summary logs/a13/p2/*.summary logs/a13/p3/*.summary logs/a13/p4/*.summary logs/a13/p4/*.png \
         logs/a13/A13-VERIFICATION.txt; do
	[ -f "$f" ] && cp "$f" "$W/evidence/$(echo "$f" | sed 's#^logs/a13/##; s#/#-#g')" || true
done

git diff "$BASE" HEAD -- kernel/ platform/ Makefile tests/a13 scripts/a13-*.sh > "$W/patches/r17-against-r16b.patch"
git diff --stat "$BASE" HEAD -- kernel/ platform/ Makefile tests/a13 scripts/a13-*.sh > "$W/patches/CHANGED-FILES.txt"
cp docs/APOLLO13-HANDOVER.md "$W/HANDOVER.md"
if find "$W" -path '*/third_party/infocom*' -o -path '*/apps/superbasic*' -o -iname '*.z[3-8]' -o -iname '*elite*' | grep -q .; then
	echo "REFUSING: a restricted source asset remains in the package" >&2; exit 4; fi
( cd "$W" && find . -type f ! -name SHA256SUMS | LC_ALL=C sort | sed 's#^\./##' | xargs -d '\n' sha256sum > SHA256SUMS )
( cd "$OUT" && tar -czf $NAME.tar.gz $NAME )
sha256sum "$OUT/$NAME.tar.gz" | tee "$OUT/$NAME.tar.gz.sha256"
echo "commit $COMMIT, base $BASE"
