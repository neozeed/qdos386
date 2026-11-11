#!/bin/sh
# R16B: assemble qdos386-R16B-FOREGROUND-SMP.tar.gz from a COMMITTED tree.
#   usage: sh scripts/r16b-package.sh OUTDIR
# Full buildable source (public-safe: SuperBASIC/Infocom sources stripped), the opt-in R16B kernels plus the
# byte-identical R16A and KCTX-FIX2 kernels, the CPL3 guest tools (R16T, TOP, CPUSET, marked BOUNCE), the focused test
# scripts, a public-safe example system volume carrying the tools (superfloppy and MBR), the minimal patch against
# R16A, selected evidence (Gate 3 logs, Task Manager screens, BOUNCE screendump), the docs + HANDOVER.md, SHA256SUMS.
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd); cd "$ROOT"
OUT=${1:?}; : "${QDOS386_TOOLS:?}"
export MTOOLS_SKIP_CHECK=1
git diff --quiet HEAD -- . ':!build' ':!logs' ':!dist' || { echo "tree not clean: commit first" >&2; exit 2; }
COMMIT=$(git rev-parse HEAD); NAME=qdos386-R16B-FOREGROUND-SMP; BASE=r16a-mbr-cpu-taskmanager
W=$OUT/$NAME; rm -rf "$W"; mkdir -p "$W"
git archive HEAD | tar -x -C "$W"
rm -rf "$W/third_party/infocom" "$W/apps/superbasic"
find "$W" -type f \( -iname '*.z[3-8]' -o -iname 'zork*.dat' -o -iname '*.zstory' \) -delete 2>/dev/null || true
cat > "$W/REMOVED-RESTRICTED.txt" <<'EOF'
Restricted research/third-party sources removed from this public archive: third_party/infocom/ (Infocom interpreter
port, no redistribution licence) and apps/superbasic/ (private research).  Not needed to build or run R16B.
EOF

make -B qdos386-r16b-pc > /dev/null 2>&1
make -B qdos386-smp-pc  > /dev/null 2>&1
make -B qdos386-pc      > /dev/null 2>&1
sh scripts/r16b-build-guest.sh > /dev/null 2>&1
mkdir -p "$W/kernels/r16a-unchanged" "$W/kernels/default-unchanged" "$W/guest" "$W/examples" "$W/patches" "$W/evidence"
cp build/qdos386-r16b/pc-vga/qdos386-r16b-vga.img build/qdos386-r16b/pc/qdos386-r16b.img "$W/kernels/"
cp build/qdos386-smp/pc-vga/qdos386-smp-vga.img build/qdos386-smp/pc/qdos386-smp.img "$W/kernels/r16a-unchanged/"
cp build/qdos386/pc-vga/qdos386-vga.img build/qdos386/pc/qdos386.img "$W/kernels/default-unchanged/"
python3 tools/kctx-symmap.py build/qdos386-r16b/pc-vga/kernel.aout > "$W/kernels/qdos386-r16b-vga.map" 2>/dev/null || true
cp build/r16/R16TP.aout build/r16b/TOPP.aout build/r16b/CPUSETP.aout build/r16b/BOUNCEP.aout "$W/guest/"

# public-safe example volume with the R16B tools (superfloppy + the R16A MBR layout)
sh scripts/qdos386-public-vol.sh "$OUT/_pub.img" > /dev/null
T=$(mktemp -d)
for p in R16T:build/r16/R16TP.aout TOP:build/r16b/TOPP.aout CPUSET:build/r16b/CPUSETP.aout BOUNCEP:build/r16b/BOUNCEP.aout; do
	cp "${p#*:}" "$T/x"; touch -d '2026-10-31 08:00:00' "$T/x"; mcopy -o -m -i "$OUT/_pub.img" "$T/x" "::/BIN/${p%%:*}.AOU"
done
rm -rf "$T"
cp "$OUT/_pub.img" "$W/examples/system-r16b-superfloppy.img"
sh scripts/r16a-wrap-mbr.sh "$OUT/_pub.img" "$W/examples/system-r16b-mbr.img" > /dev/null
rm -f "$OUT/_pub.img"
for v in "$W/examples/"*.img; do
	img=$v; [ "${v%mbr.img}" != "$v" ] && { dd if="$v" bs=512 skip=2048 of="$OUT/_p.img" status=none; img=$OUT/_p.img; }
	if mdir -i "$img" -/ 2>/dev/null | grep -i -q -E 'BASIC\.AOU|BASTEST|INFOCOM|ZORK|STORY|\.Z[3-8]'; then
		echo "REFUSING: restricted asset on $v" >&2; exit 3; fi
done
rm -f "$OUT/_p.img"

# evidence actually produced (only if present in this tree's logs)
for f in logs/r16b/R16B-VERIFICATION.txt logs/r16b/g3f-m0.summary logs/r16b/g3f-m1.summary logs/r16b/g3f-m2.summary \
         logs/r16b/g3f-m3.summary logs/r16b/s4c/bounce-focused.png logs/r16b/s4c/BOUNCE.LOG logs/r16b/s4/qemu-b.txt \
         logs/r16b/s4c/qemu-c.txt logs/r16b/soak/soak-summary.txt logs/r16b/soak15-summary.txt; do
	[ -f "$f" ] && cp "$f" "$W/evidence/$(echo "$f" | sed 's#^logs/r16b/##; s#/#-#g')" || true
done

git diff "$BASE" HEAD -- kernel/ Makefile tests/r16/r16t.c tests/r16/run-gate1.sh tests/r16b scripts/r16b-*.sh > "$W/patches/r16b-against-r16a.patch"
git diff --stat "$BASE" HEAD -- kernel/ Makefile tests/r16/r16t.c tests/r16/run-gate1.sh tests/r16b scripts/r16b-*.sh > "$W/patches/CHANGED-FILES.txt"
mkdir -p "$W/docs-r16b"
cp docs/R16B-SCHEDULER-POLICY.md docs/R16B-MIGRATION-ABI.md docs/R16B-RUNTIME-REPORT.md docs/R16B-KNOWN-LIMITATIONS.md docs/R16B-AUDIT.md "$W/docs-r16b/"
cp docs/R16B-HANDOVER.md "$W/HANDOVER.md"
if find "$W" -path '*/third_party/infocom*' -o -path '*/apps/superbasic*' -o -iname '*.z[3-8]' | grep -q .; then
	echo "REFUSING: a restricted source asset remains in the package" >&2; exit 4; fi
( cd "$W" && find . -type f ! -name SHA256SUMS | LC_ALL=C sort | sed 's#^\./##' | xargs -d '\n' sha256sum > SHA256SUMS )
( cd "$OUT" && tar -czf $NAME.tar.gz $NAME )
sha256sum "$OUT/$NAME.tar.gz" | tee "$OUT/$NAME.tar.gz.sha256"
echo "commit $COMMIT, base $BASE"
