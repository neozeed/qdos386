#!/bin/sh
# Verify an extracted qdos386-R11G-ZORK1 release from a fresh directory:
#   1. SHA256SUMS matches every file, and the file list matches exactly (no extra or missing files).
#   2. The interpreter is rebuilt from the extracted source and must equal the shipped infocom.aout.
#   3. Scripted Tiny386 smoke test on the shipped story-free volume: the interpreter must report the
#      missing story, and the shell must still run DIR afterwards.
# The boot disk and firmware come from the R11F release (see MANIFEST.txt).
#   usage: TINY386=<tiny386_headless> R11F_BUILD=<dir with pc-r11f-none and pc-r11f-none-run> \
#          sh <extracted>/scripts/r11g-fresh-check.sh <extracted-release-dir> <historical-tree> <outdir>
set -eu
REL=$(cd "$1" && pwd)
TOOLS=$2
OUT=$3
: "${TINY386:?set TINY386}" "${R11F_BUILD:?set R11F_BUILD}"
mkdir -p "$OUT"
OUT=$(cd "$OUT" && pwd)
LOG=$OUT/fresh-check.txt
: > "$LOG"
say() { echo "$*" | tee -a "$LOG"; }
fail() { say "FRESH CHECK: FAIL ($*)"; exit 1; }

say "release: $REL"
say "tiny386 sha256: $(sha256sum "$TINY386" | cut -c1-64)"

# 1. Manifest. SHA256SUMS paths are relative to the parent of the release directory.
R=$REL
PARENT=$(dirname "$REL")
NAME=$(basename "$REL")
cd "$PARENT"
sha256sum -c "$NAME/SHA256SUMS" > "$OUT/sha256-check.txt" 2>&1 || fail "SHA256SUMS mismatch, see sha256-check.txt"
awk '{print $2}' "$NAME/SHA256SUMS" | sort > "$OUT/listed.txt"
find "$NAME" -type f ! -name SHA256SUMS | sort > "$OUT/present.txt"
cmp -s "$OUT/listed.txt" "$OUT/present.txt" || fail "file list differs from SHA256SUMS"
say "manifest: $(wc -l < "$OUT/listed.txt") files, all hashes match"
cd "$R"
# 2. Rebuild the interpreter.
sh "$R/scripts/r11g-build.sh" "$R/sdk" "$TOOLS" "$OUT/build" > "$OUT/build.log" 2>&1 || fail "build failed, see build.log"
REBUILT=$(sha256sum "$OUT/build/infocom.aout" | cut -c1-64)
SHIPPED=$(sha256sum "$R/build/r11g/infocom.aout" | cut -c1-64)
say "rebuilt infocom.aout: $REBUILT"
say "shipped  infocom.aout: $SHIPPED"
[ "$REBUILT" = "$SHIPPED" ] || fail "rebuilt interpreter differs from shipped a.out"

# 3. Smoke test on the story-free volume.
mkdir -p "$R/build"
cp -r "$R11F_BUILD/pc-r11f-none" "$R11F_BUILD/pc-r11f-none-run" "$R/build/"
TINY386=$TINY386 sh "$R/scripts/r11g-run-tiny386.sh" "$OUT/smoke" "$R/images/win1-r11g-missing.img" \
	> "$OUT/smoke.log" 2>&1 || fail "smoke run failed, see smoke.log"
T=$OUT/smoke/tiny386-game.txt
grep -q "Failed to Open File WIN1_ZORK1_Z3" "$T" || fail "missing-story message not seen"
# The shell must still list the disk after the failed launch.
tr -d '\r' < "$T" | awk '/Failed to Open File WIN1_ZORK1_Z3/{seen=1} seen && /SHELL\.AOU/{ok=1} END{exit ok?0:1}' \
	|| fail "shell did not run DIR after the failed launch"
say "smoke: missing-story message reported, shell survived (DIR after failure)"
say "FRESH CHECK: PASS"
