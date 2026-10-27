#!/bin/sh
# Assemble qdos386-R11F.tar.gz from the r11f branch HEAD plus the built outputs that were
# tested.  Requires a clean tree at HEAD on branch r11f, and a completed matrix and volume
# build (scripts/r11f-volume.sh, scripts/r11f-images.sh, scripts/r11f-run-all.sh).
#   usage: FW=<dir with bios.bin, vgabios.bin> TINY=<tiny386_headless> \
#          I386VM_HARNESS=<i386vm-r9 binary> I386VM=<toolchain tree> \
#          sh scripts/make-r11f-package.sh <output-dir>
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd)
cd "$ROOT"
: "${FW:?}" "${TINY:?}" "${I386VM_HARNESS:?}" "${I386VM:?}"
OUTDIR=${1:?usage: make-r11f-package.sh <output-dir>}
mkdir -p "$OUTDIR"
OUTDIR=$(cd "$OUTDIR" && pwd)
PKG=$OUTDIR/pkg
P=$PKG/qdos386-R11F

rm -rf "$PKG" && mkdir -p "$PKG"
test "$(git branch --show-current)" = r11f
test -z "$(git status --porcelain)"
git archive --format=tar --prefix=qdos386-R11F/ HEAD | tar -x -C "$PKG"
COMMIT=$(git rev-parse HEAD)

# Built, tested outputs (not in git).
mkdir -p "$P/sdk/lib" "$P/sdk/examples" "$P/build/apps" "$P/images" "$P/run"
mkdir -p "$P/control"
cp sdk/lib/libc.a sdk/lib/libm.a sdk/lib/libgcc-qd.a sdk/lib/libqdos.a "$P/sdk/lib/"
# The wrong-rate control (TMABIW) is test-only: it is NOT a production example.
for f in build/r11f/*.aout; do
	case "$(basename "$f")" in TMABIW.aout) ;; *) cp "$f" "$P/sdk/examples/" ;; esac
done
cp build/apps/PHOON.aout "$P/build/apps/"
cp build/fat16/win1-r11f.img "$P/images/"
cp build/pc-r11f-vga/r9shell-vga.img "$P/images/r9shell-vga.img"
cp build/i386vm-r11f-pin/qdos386-r11f-pin.bin "$P/images/"
cp build/i386vm-r11f-none/qdos386-r11f-none.bin "$P/images/"
cp build/i386vm-r11f-adv/qdos386-r11f-adv.bin "$P/images/"
for MODE in none pin adv; do
	cp build/pc-r11f-$MODE/r11f-$MODE.img "$P/images/"
done

# Run directories: each has the BIOS pair, its kernel as hda, the R11F volume as hdb.
for MODE in none pin adv; do
	D=$P/run/$MODE
	mkdir -p "$D"
	cp "$FW/bios.bin" "$FW/vgabios.bin" "$D/"
	cp "build/pc-r11f-$MODE/r11f-$MODE.img" "$D/hda.img"
	cp build/fat16/win1-r11f.img "$D/win1-r11f.img"
	printf '[pc]\nbios = bios.bin\nvga_bios = vgabios.bin\nmem_size = 8M\nhda = hda.img\nhdb = win1-r11f.img\n\n[cpu]\nfpu = 1\n' > "$D/run.ini"
done

# Test-only control (never a production image): the wrong-rate clock test and the volume and
# i386VM kernel that carry it.  Expected: TMABII PASS, TMABIW FAIL.
cp build/r11f/TMABIW.aout "$P/control/TMABIW.aout"
cp build/fat16/win1-r11f-control.img "$P/control/win1-r11f-control.img"
cp build/i386vm-r11f-control/qdos386-r11f-control.bin "$P/control/"
cat > "$P/control/README.txt" <<'EOC'
TEST-ONLY CONTROL.  Not a release image.  TMABIW assumes 110 ticks per second, 10% more than
the i386VM timer (100/1).  On qdos386-r11f-control.bin with win1-r11f-control.img, RUN TMABII.AOU
must PASS and RUN TMABIW.AOU must FAIL (its bracket checks).  See docs/R11F-RUNTIME-REPORT.md.
EOC

# Original Phoon source (unchanged) and the portability patch are in the git tree under
# third_party/phoon.  The built PHOON.aout is in build/apps.

# Logs of the matrix are in the git tree under logs/r11f (the runtime report cites them).

EXT="$P/EXTERNAL-DEPENDENCIES.txt"
{
  echo "External inputs used to build and test R11F (not in the source archive)."
  echo "Hashes are sha256 of the exact files used."
  echo
  echo "Tiny386 (vga-window dev-tree build, used for all R11F runs):"
  sha256sum "$TINY" | sed 's|  .*|  tiny386_headless|'
  echo "BIOS (also in run/*/bios.bin):"
  sha256sum "$FW/bios.bin" | sed 's|  .*|  bios.bin|'
  echo "VGA BIOS (also in run/*/vgabios.bin):"
  sha256sum "$FW/vgabios.bin" | sed 's|  .*|  vgabios.bin|'
  echo "i386VM harness (i386vm-r9, used by scripts/r11f-run-all.sh):"
  sha256sum "$I386VM_HARNESS" | sed 's|  .*|  i386vm-r9|'
  echo "GCC 1.40 / GAS / i386-ld toolchain (QDOS386_TOOLS). NOT included:"
  for f in i386-gcc cc1 cccp as i386-ld i386vm; do
    sha256sum "$I386VM/bin/$f" | sed 's|  .*|  bin/'"$f"'|'
  done
  echo "Host Phoon reference (not shipped): gcc -std=gnu89 on x86-64 with a time() shim."
  echo "PyEphem (ephem), used only by logs/r11f/moon-check.py: not shipped."
} > "$EXT"

WIN_SHA=$(sha256sum "$P/images/win1-r11f.img" | cut -c1-64)
cat > "$P/PACKAGE.md" <<EOM
# QDOS/386 R11F package

Source commit: $COMMIT (branch r11f, from the frozen R11E commit 7a62af1).
Contents: source tree (git archive of the commit), built SDK libraries and example
applications, PHOON.aout, disk images, run/ directories, SHA256SUMS.
FAT16 volume (images/win1-r11f.img) sha256: $WIN_SHA

QEMU VGA (scripted smoke test, scripts/r11f-qemu-smoke.sh; the kernel is images/r9shell-vga.img,
the volume images/win1-r11f.img):
  qemu-system-i386 -machine pc -cpu pentium -m 16 -bios <bios.bin> -vga std -display gtk \\
    -drive file=hda.img,format=raw,if=ide,index=0 -drive file=hdb.img,format=raw,if=ide,index=1 -boot order=c

Clock modes (see docs/R11F-FIXED-CLOCK.md):
  run/none  no clock (regression)      run/pin  pinned at 2026-10-27 12:00:00 UTC
  run/adv   advancing from 2000-01-01

Run (Tiny386, pinned clock): cd run/pin && tiny386_headless run.ini
Then at the shell prompt:   RUN PHOON.AOU        RUN PHOON.AOU "11 Jan 2024 11:57:00"

Evidence and limits: docs/R11F-RUNTIME-REPORT.md, docs/R11F-KNOWN-LIMITATIONS.md,
docs/R11F-TIME-ABI.md, docs/R11F-PHOON-PORT.md, docs/R11F-C89-COVERAGE.md.
All runs were scripted.
EOM

cd "$PKG"
find qdos386-R11F -type f ! -name SHA256SUMS | sort | xargs sha256sum > "$P/SHA256SUMS"
tar -czf "$OUTDIR/qdos386-R11F.tar.gz" qdos386-R11F
sha256sum "$OUTDIR/qdos386-R11F.tar.gz" > "$OUTDIR/qdos386-R11F.tar.gz.sha256"
echo "package: $OUTDIR/qdos386-R11F.tar.gz"
