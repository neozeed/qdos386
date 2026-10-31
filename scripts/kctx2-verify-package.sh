#!/bin/sh
# KCTX-FIX2: verify a FRESH extraction of qdos386-KCTX-FIX2-MAINLINE.tar.gz, using only the package (and the host's
# QEMU, mtools, fsck.fat and the toolchain at QDOS386_TOOLS).
#   usage: sh scripts/kctx2-verify-package.sh EXTRACTED_PKG_DIR OUTDIR
#  1 manifest   2 negative-control kernels read-only and = the R14 release   3 `make` (default goal) rebuilds the
#  packaged current kernels bit-for-bit and they contain the exact-ESP resume   4 `make r14-pc` reproduces the negative
#  control (frozen history)   5 probes rebuild identical   6 the public-safe source tree builds the same kernels
#  7 the public volume holds no restricted file   8 QEMU smoke (packaged kernel, public volume + probes)
#  9 Tiny386 smoke (packaged emulator and headless kernel)   10 the negative control still fails on the misaligned probe
set -u
P=$(cd "${1:?}" && pwd); O=$(mkdir -p "${2:?}" && cd "$2" && pwd); : "${QDOS386_TOOLS:?}"
cd "$P"; L=$O/package-verify.txt; : > "$L"; n=0; f=0; export MTOOLS_SKIP_CHECK=1 QEMU_NO_SHADOW=1
say() { echo "$*" | tee -a "$L"; }
ok() { n=$((n+1)); if [ "$1" = 0 ]; then say "PASS $2"; else f=$((f+1)); say "FAIL $2"; fi; }
sha256sum -c --quiet SHA256SUMS > "$O/manifest.txt" 2>&1; ok $? "manifest: $(wc -l < SHA256SUMS) files match SHA256SUMS"
[ ! -w kernels/negative-control/r14-vga.img ] || [ "$(id -u)" = 0 ]; m=$(stat -c %a kernels/negative-control/r14-vga.img kernels/negative-control/r14.img | tr '\n' ' ')
h=$(sha256sum kernels/negative-control/r14-vga.img kernels/negative-control/r14.img | cut -c1-16 | tr '\n' ' ')
[ "$m" = "444 444 " ] && [ "$h" = "8bfd4d5887e47f55 1ad61b13fffc5a21 " ]; ok $? "negative control: original R14 kernels, mode 0444 ($m), $h"
make > "$O/make.log" 2>&1; ok $? "make (default goal = qdos386-pc)"
for p in build/qdos386/pc-vga/qdos386-vga.img:kernels/qdos386-vga.img build/qdos386/pc/qdos386.img:kernels/qdos386.img; do
	cmp -s "${p%%:*}" "${p#*:}"; ok $? "rebuild identical: ${p#*:} ($(sha256sum < "${p#*:}" | cut -c1-16))"
done
for d in pc-vga pc; do python3 tools/kctx2-resume-check.py build/qdos386/$d/kernel.aout build/qdos386/$d/kernel.raw exact >> "$O/resume.txt"; ok $? "build/qdos386/$d: resume_dispatch is EXACT (no andl \$-4)"; done
make r14-pc > "$O/make-r14.log" 2>&1
cmp -s build/r14/pc-vga/r14-vga.img kernels/negative-control/r14-vga.img && cmp -s build/r14/pc/r14.img kernels/negative-control/r14.img
ok $? "frozen history: make r14-pc rebuilds the negative-control R14 kernels bit-for-bit"
python3 tools/kctx2-resume-check.py build/r14/pc-vga/kernel.aout build/r14/pc-vga/kernel.raw ptrbits >> "$O/resume.txt"; ok $? "frozen r14-pc keeps the historical decode (PTRBITS)"
sh tests/kctx/build.sh build/kctx > "$O/probe-build.log" 2>&1
for b in ALIGNT spin_misaligned spin_aligned; do cmp -s build/kctx/$b.aout probe/$b.aout; ok $? "rebuild identical: probe/$b.aout"; done
# the package is not a git checkout: the public-safe tree is derived from its files exactly as
# scripts/qdos386-public-source.sh derives it from a commit (same three paths removed)
rm -rf "$O/pub"; mkdir -p "$O/pub"; tar -C "$P" --exclude=./build --exclude=./kernels --exclude=./volumes --exclude=./emulator \
  --exclude=./toolchain --exclude=./probe --exclude=./patches -cf - . | tar -C "$O/pub" -xf -
rm -rf "$O/pub/apps/superbasic" "$O/pub/third_party/infocom/orig" "$O/pub/third_party/infocom/r11g-platform.patch"
[ ! -e "$O/pub/apps/superbasic" ] && [ ! -e "$O/pub/third_party/infocom/orig" ]; ok $? "public-safe source tree: apps/superbasic and the Infocom interpreter source absent"
( cd "$O/pub" && make > "$O/make-pub.log" 2>&1 ) && cmp -s "$O/pub/build/qdos386/pc-vga/qdos386-vga.img" kernels/qdos386-vga.img && cmp -s "$O/pub/build/qdos386/pc/qdos386.img" kernels/qdos386.img
ok $? "public-safe source tree: make builds the same two kernels bit-for-bit"
rm -rf "$O/pub"
mdir -/ -b -i volumes/qdos386-public.img :: > "$O/public-vol.txt" 2>&1
! grep -i -q -E '/BASIC\.AOU$|BASTEST|INFOCOM|ZORK|\.Z[3-8]$|STORY' "$O/public-vol.txt" && fsck.fat -n volumes/qdos386-public.img > /dev/null 2>&1
ok $? "public volume: no SuperBASIC / Infocom / story file ($(wc -l < "$O/public-vol.txt") entries), fsck clean"
cp volumes/qdos386-public.img "$O/smoke.img"; T=$(mktemp -d)
for b in ALIGNT:ALIGNT SPMIS:spin_misaligned SPALI:spin_aligned; do cp "probe/${b#*:}.aout" "$T/x"; mcopy -o -i "$O/smoke.img" "$T/x" "::/BIN/${b%%:*}.AOU"; done
rm -rf "$T"
printf '%s\n' LEAK SPALI SPMIS "ALIGNT 1 1000000000 0" "ALIGNT 2 200000 1" "ALIGNT 3 300 2" "ALIGNT 3 1000000000 0" \
  "START ALIGNT 2 200000000 0 /CON:1" "ALIGNT 1 300 2" "%sleep 30" "CONDUMP 1" "CD /GAMES/HACK" "DIR" "CD /" LEAK > "$O/smoke.seq"
R12_QKERNEL=kernels/qdos386-vga.img QEMU_MON_PORT=4995 QEMU_CMD_TIMEOUT=90 sh scripts/r12-run-qemu.sh "$O/smoke.img" "$O/smoke.seq" "$O" smoke 900 > /dev/null 2>&1
python3 tests/kctx/judge_probe.py "$O/qemu-smoke.txt" > "$O/smoke-judge.txt" 2>&1; ok $? "QEMU smoke, packaged qdos386-vga.img + public volume: $(tail -1 "$O/smoke-judge.txt")"
sed 's/^/    /' "$O/smoke-judge.txt" >> "$L"
mkdir -p build/pc-r11f-none-run; cp kernels/bios.bin kernels/vgabios.bin build/pc-r11f-none-run/
printf '%s\n' "RUN LEAK" SPALI SPMIS "ALIGNT 1 100000000 0" "ALIGNT 2 50000 1" "ALIGNT 3 200 2" SYNC "RUN LEAK" "!RUN SHUTDOWN" > "$O/tiny.seq"
TINY386=$P/emulator/bin/tiny386_headless sh scripts/fix1-tiny-run.sh "$O/smoke.img" "$O/tiny.seq" "$O" tsmoke wait kernels/qdos386.img > "$O/tiny-result.txt" 2>&1
T=$O/tiny386-tsmoke.txt
c=$(grep -c "result=OK" "$T" 2>/dev/null); s=$(grep -c "exit status 0" "$T" 2>/dev/null)
[ "$c" -ge 3 ] && [ "$s" -ge 5 ] && ! grep -q PGFAULT "$T" && grep -q "exited=1" "$O/tiny386-tsmoke.txt.end" && grep -q "IMAGE PASS" "$O/tiny-result.txt"
ok $? "Tiny386 smoke, packaged emulator + qdos386.img: controls exit 0, ALIGNT residues 1-3 OK ($c), no PGFAULT, guest shutdown, clean image"
printf '%s\n' SPALI SPMIS > "$O/neg.seq"
QEMU_MON_PORT=4996 QEMU_CMD_TIMEOUT=60 R12_QKERNEL=kernels/negative-control/r14-vga.img sh scripts/r12-run-qemu.sh "$O/smoke.img" "$O/neg.seq" "$O" neg 300 > /dev/null 2>&1
grep -aq "PGFAULT: #DF double fault" "$O/qemu-neg.txt"; ok $? "negative control: original R14 kernel still double-faults on spin_misaligned"
rm -f "$O"/*.img
say "PACKAGE VERIFY: $((n-f))/$n PASS"
[ $f = 0 ]
