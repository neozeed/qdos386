#!/bin/sh
# Assemble qdos386-R11G-ZORK1.tar.gz from the r11g branch HEAD plus the built, tested outputs.
# The story file and every image or variant that contains it are NOT included (see MANIFEST.txt).
# Requires a clean tree at HEAD on branch r11g, and the outputs of the final runs
# (scripts/r11g-build.sh, the story-free volume, and the evidence directory).
#   usage: NOSTORY=<story-free volume> EVIDENCE=<final-run evidence dir> \
#          sh scripts/make-r11g-package.sh <output-dir>
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd)
cd "$ROOT"
: "${NOSTORY:?set NOSTORY to the story-free volume (win1-r11g-missing.img)}"
: "${EVIDENCE:?set EVIDENCE to the final-run evidence directory}"
OUTDIR=${1:?usage: make-r11g-package.sh <output-dir>}
mkdir -p "$OUTDIR"
OUTDIR=$(cd "$OUTDIR" && pwd)
PKG=$OUTDIR/pkg
P=$PKG/qdos386-R11G
STORY_SHA=37084966477dff679282de42974b2077156b1bd68fad92a65d4ea94d8eb64d79
PROD_VOL_SHA=e47ddf039cb81c3cc2c356f93db27c4c90da63b1185e844b287c1e20e604d7ea

rm -rf "$PKG" && mkdir -p "$PKG"
test "$(git branch --show-current)" = r11g
test -z "$(git status --porcelain)"
git archive --format=tar --prefix=qdos386-R11G/ HEAD | tar -x -C "$PKG"
COMMIT=$(git rev-parse HEAD)

mkdir -p "$P/sdk/lib" "$P/build/r11g" "$P/images" "$P/evidence/matrix" "$P/evidence/regression" \
	"$P/evidence/negative" "$P/evidence/qemu" "$P/evidence/host-ref"
cp sdk/lib/libc.a sdk/lib/libm.a sdk/lib/libgcc-qd.a sdk/lib/libqdos.a "$P/sdk/lib/"
cp build/r11g/infocom.aout "$P/build/r11g/"
cp "$NOSTORY" "$P/images/win1-r11g-missing.img"

E=$EVIDENCE
cp -r "$E/matrix-final2/." "$P/evidence/matrix/"
cp "$E/matrix-final2.log" "$P/evidence/matrix/"
cp -r "$E/regression2/." "$P/evidence/regression/"
cp "$E/regression2.log" "$P/evidence/regression/"
cp -r "$E/negative-run4/." "$P/evidence/negative/"
cp "$E/negative-run4.log" "$P/evidence/negative/"
cp -r "$E/qemu/." "$P/evidence/qemu/"
cp "$E/qemu.log" "$P/evidence/qemu/"
cp "$E/host-ref/game1.txt" "$E/host-ref/game2.txt" "$P/evidence/host-ref/"

# The story must not be in the release, by hash or by size.
if find "$PKG" -type f -size 86838c | grep -q .; then
	echo "refusing: a file of story size is in the package" >&2; exit 3
fi
if find "$PKG" -type f -exec sha256sum {} + | grep -q "^$STORY_SHA"; then
	echo "refusing: the story file hash is in the package" >&2; exit 3
fi
if find "$PKG" -type f -exec sha256sum {} + | grep -q "^$PROD_VOL_SHA"; then
	echo "refusing: the production volume (contains the story) is in the package" >&2; exit 3
fi

INT_SHA=$(sha256sum "$P/build/r11g/infocom.aout" | cut -c1-64)
VOL_SHA=$(sha256sum "$P/images/win1-r11g-missing.img" | cut -c1-64)
cat > "$P/MANIFEST.txt" <<EOF
qdos386-R11G-ZORK1
Source commit: $COMMIT (branch r11g, base R11F d614fa6)

Included:
  sdk/ and third_party/infocom/   source tree of this commit (git archive); original Task Force '87
                                  sources unmodified in third_party/infocom/orig, port patch separate
  sdk/lib/*.a                     libraries built from this tree
  build/r11g/infocom.aout         the interpreter, SHA-256 $INT_SHA
  images/win1-r11g-missing.img    story-free FAT16 volume (R11F set + INFOCOM.AOU), SHA-256 $VOL_SHA
  evidence/                       transcripts, logs and screenshots of the final runs (scripted input)
  docs/                           R11G audit, port, runtime report, SDK notes

Not included:
  zork1_z3 (story)                SHA-256 $STORY_SHA. Its licence is not yet established for private
                                  or public distribution. Supply your own copy to run the game.
  win1-r11g.img (production)      tested volume with the story; SHA-256 $PROD_VOL_SHA
  negative-test variants          derived from the story bytes; rebuilt by scripts/r11g-negative-volumes.sh
  emulators, BIOS, boot disk      use the R11F release (qdos386-R11F): its build/pc-r11f-none and
                                  build/pc-r11f-none-run boot disk and firmware are required by
                                  scripts/r11g-run-tiny386.sh and the volume build

To rebuild the production volume, copy from the R11F release into this tree at the same paths:
build/fat16/win1-r11f.img and the build/r11f application images that scripts/r11f-volume.sh uses. Then run
ZORK_STORY=<path to your zork1_z3> QDOS386_TOOLS=<historical GCC 1.40 tree> sh scripts/r11g-volume.sh.
The result must have SHA-256 $PROD_VOL_SHA for the story file above.
EOF

(cd "$PKG" && find qdos386-R11G -type f ! -name SHA256SUMS | sort | xargs sha256sum) > "$P/SHA256SUMS"
(cd "$PKG" && tar -czf "$OUTDIR/qdos386-R11G-ZORK1.tar.gz" qdos386-R11G)
sha256sum "$OUTDIR/qdos386-R11G-ZORK1.tar.gz" > "$OUTDIR/qdos386-R11G-ZORK1.tar.gz.sha256"
echo "source commit $COMMIT"
cat "$OUTDIR/qdos386-R11G-ZORK1.tar.gz.sha256"
