#!/bin/sh
# R16A: assemble qdos386-R16A-MBR-CPU-TASKMANAGER.tar.gz from a COMMITTED tree.
#   usage: sh scripts/r16a-package.sh OUTDIR
# Full buildable source (public-safe: SuperBASIC/Infocom sources stripped), the R16A kernels + the byte-identical
# KCTX-FIX2 kernels, the CPL3 guest, the host unit test, the fixture/QEMU scripts, a clean public-safe
# MBR-partitioned FAT16 example data image + the matching superfloppy + a public-safe MBR-wrapped system volume,
# the R16A patch against the R16 baseline, the four R16A docs + HANDOVER.md, transcripts, and SHA256SUMS.
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd); cd "$ROOT"
OUT=${1:?}; : "${QDOS386_TOOLS:?}"
export MTOOLS_SKIP_CHECK=1
git diff --quiet HEAD -- . ':!build' ':!logs' ':!dist' || { echo "tree not clean: commit first" >&2; exit 2; }
COMMIT=$(git rev-parse HEAD); NAME=qdos386-R16A-MBR-CPU-TASKMANAGER; BASE=r16-bkl-smp-experiment
W=$OUT/$NAME; rm -rf "$W"; mkdir -p "$W"
git archive HEAD | tar -x -C "$W"

# public-safe source: strip the restricted porting trees (as the R16 package does); kernels do not compile them
rm -rf "$W/third_party/infocom" "$W/apps/superbasic"
find "$W" -type f \( -iname '*.z[3-8]' -o -iname 'zork*.dat' -o -iname '*.zstory' \) -delete 2>/dev/null || true
cat > "$W/REMOVED-RESTRICTED.txt" <<'EOF'
Restricted research/third-party sources removed from this public archive: third_party/infocom/ (Infocom interpreter
port, no redistribution licence) and apps/superbasic/ (private research).  Not needed to build or run R16A.
EOF

# kernels: R16A (the feature build) + the byte-identical KCTX-FIX2 defaults
make -B qdos386-smp-pc > /dev/null 2>&1
make -B qdos386-pc     > /dev/null 2>&1
sh scripts/r16-build-guest.sh > /dev/null 2>&1
mkdir -p "$W/kernels" "$W/guest" "$W/examples" "$W/patches" "$W/transcripts"
cp build/qdos386-smp/pc-vga/qdos386-smp-vga.img build/qdos386-smp/pc/qdos386-smp.img "$W/kernels/"
cp build/qdos386/pc-vga/qdos386-vga.img        build/qdos386/pc/qdos386.img          "$W/kernels/"
cp build/r16/R16T.aout build/r16/R16TP.aout "$W/guest/"

# host unit test transcript
sh scripts/r16a-hosttest.sh "$OUT/_host" > "$W/transcripts/host-test.txt" 2>&1 || true

# --- example data images (public-safe) ---
# 1) a bare MBR-partitioned FAT16 data disk + its superfloppy twin (HELLO.TXT etc.; no restricted assets)
sh scripts/r16a-mkdata.sh "$OUT/_fix" > /dev/null
cp "$OUT/_fix/data-mbr.img" "$W/examples/data-mbr.img"
cp "$OUT/_fix/data-superfloppy.img" "$W/examples/data-superfloppy.img"
# 2) a public-safe SYSTEM volume wrapped as an MBR FAT16 partition, for the boot demo
sh scripts/qdos386-public-vol.sh "$OUT/_sys.img" > /dev/null
T=$(mktemp -d); cp build/r16/R16TP.aout "$T/x"; touch -d '2026-10-30 08:00:00' "$T/x"
mcopy -o -m -i "$OUT/_sys.img" "$T/x" "::/BIN/R16T.AOU"; rm -rf "$T"
cp "$OUT/_sys.img" "$W/examples/system-superfloppy.img"
sh scripts/r16a-wrap-mbr.sh "$OUT/_sys.img" "$W/examples/system-mbr.img" > /dev/null
# refuse any restricted asset on the example volumes
for v in "$W/examples/"*.img; do
	if mdir -i "$v" -/ 2>/dev/null | grep -i -q -E 'BASIC\.AOU|BASTEST|INFOCOM|ZORK|STORY|\.Z[3-8]'; then
		echo "REFUSING: restricted asset on $v" >&2; exit 3; fi
done

# transcripts/screens actually produced
for f in logs/r16a/boot/qemu-g.txt logs/r16a/tm/qemu-tm3.txt logs/r16a/persist/qemu-b.txt; do
	[ -f "$f" ] && cp "$f" "$W/transcripts/$(basename "$f")" 2>/dev/null || true
done
sed -n '/=== SCREEN tm/,/SCREEN-END/p' logs/r16a/tm/qemu-tm3.txt 2>/dev/null | sed 's/[^[:print:]]//g' > "$W/transcripts/taskmgr-cpu-column.txt" || true

# the R16A patch against the R16 baseline
F="kernel/mbr.c kernel/mbr.h kernel/win1.c kernel/jobs.c kernel/jobs.h kernel/taskmgr.c kernel/smp.c Makefile tests/r16/r16t.c tests/host/r16a scripts/r16a-mkdata.sh scripts/r16a-wrap-mbr.sh scripts/r16a-hosttest.sh"
git diff "$BASE" HEAD -- $F > "$W/patches/r16a-against-r16.patch"
git diff --stat "$BASE" HEAD -- $F > "$W/patches/CHANGED-FILES.txt"

# docs
cp docs/R16A-DESIGN.md docs/R16A-RUNTIME-REPORT.md docs/R16A-KNOWN-LIMITATIONS.md "$W/"
cp docs/R16A-HANDOVER.md "$W/HANDOVER.md"

# final restricted-asset gate over the whole package
if find "$W" -path '*/third_party/infocom*' -o -path '*/apps/superbasic*' -o -iname '*.z[3-8]' | grep -q .; then
	echo "REFUSING: a restricted source asset remains in the package" >&2; exit 4; fi

rm -rf "$OUT/_host" "$OUT/_fix" "$OUT/_sys.img"
( cd "$W" && find . -type f ! -name SHA256SUMS | LC_ALL=C sort | sed 's#^\./##' | xargs -d '\n' sha256sum > SHA256SUMS )
( cd "$OUT" && tar -czf $NAME.tar.gz $NAME )
sha256sum "$OUT/$NAME.tar.gz" | tee "$OUT/$NAME.tar.gz.sha256"
echo "commit $COMMIT, base $BASE"
