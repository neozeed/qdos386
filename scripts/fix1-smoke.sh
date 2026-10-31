#!/bin/sh
# R11M-FIX1 clean-extraction smoke: run from the ROOT OF AN EXTRACTED PACKAGE (qdos386-R11M-FIX1-TINY386-SHUTDOWN/).  Bounded (about 5 minutes).
#   1. every file listed in SHA256SUMS verifies
#   2. QEMU (primary; skipped with SMOKE_QEMU=0 or if qemu-system-i386 is missing): VFAT sequences s4a then s4b after a reboot on the packaged volume;
#      long name and alias listed after reboot, fsck.fat, fatwalk and mtools agree
#   3. Tiny386 full-machine headless (the packaged patched emulator emulator/bin/tiny386_headless, or $TINY386): s4a then s4b, EACH ended by the guest
#      (RUN SHUTDOWN.AOU), no dummy read; exit status 0, flush report, FAT copies equal, fsck.fat, fatwalk; then a shutdown right after boot
#      (volume byte-identical)
#   4. CPU-only i386VM: NOT RUN (legacy diagnostic, scripts/legacy-i386vm.sh)
#   usage: sh scripts/fix1-smoke.sh OUTDIR
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd); cd "$ROOT"
OUT=${1:?usage}; mkdir -p "$OUT"; OUT=$(cd "$OUT" && pwd)
export MTOOLS_SKIP_CHECK=1 LC_ALL=C.UTF-8
BAD=0; RAN=0
ok()  { echo "ok   $1"; }
bad() { echo "FAIL $1"; BAD=$((BAD + 1)); }
VOL0=volumes/fix1-initial.img
if sha256sum -c SHA256SUMS --quiet > "$OUT/sha256-check.txt" 2>&1; then ok "SHA256SUMS: $(wc -l < SHA256SUMS) files verify"; else bad "SHA256SUMS"; fi
sd() { sed '$d' "$1" > "$2"; echo '!RUN SHUTDOWN.AOU' >> "$2"; }

if [ "${SMOKE_QEMU:-1}" = 1 ] && command -v qemu-system-i386 >/dev/null; then
  RAN=$((RAN + 1)); mkdir -p "$OUT/qemu"
  R11M_QKERNEL=build/r11m/pc-vga/r11m-vga.img QEMU_CMD_TIMEOUT=300 sh scripts/r11m-run-qemu.sh "$VOL0" tests/r11m/seq-s4a.txt "$OUT/qemu" s4a 1800 >/dev/null
  R11M_QKERNEL=build/r11m/pc-vga/r11m-vga.img QEMU_CMD_TIMEOUT=300 sh scripts/r11m-run-qemu.sh "$OUT/qemu/qemu-s4a.final.img" tests/r11m/seq-s4b.txt "$OUT/qemu" s4b 1800 >/dev/null
  grep -q "SUPERB~1.C .*SUPERBASIC_INTERPRETER.C" "$OUT/qemu/qemu-s4b.txt" && ok "QEMU: after reboot the long name and its alias are listed" || bad "QEMU listing"
  if sh scripts/fix1-check-image.sh "$OUT/qemu/qemu-s4b.final.img" "$OUT/qemu/check.fsck.txt" > "$OUT/qemu/check.txt" 2>&1; then ok "QEMU: FAT copies equal, fsck.fat clean, fatwalk 0 errors"; else bad "QEMU image check"; fi
  mcopy -n -i "$OUT/qemu/qemu-s4b.final.img" ::/SRC/SUPERBASIC_INTERPRETER.C "$OUT/qemu/mt-long.bin" 2>/dev/null && mcopy -n -i "$VOL0" ::/LONG.TXT "$OUT/qemu/mt-src.bin" \
    && cmp -s "$OUT/qemu/mt-long.bin" "$OUT/qemu/mt-src.bin" && ok "QEMU: mtools: SRC/SUPERBASIC_INTERPRETER.C identical to LONG.TXT" || bad "QEMU mtools content"
else
  echo "NA   QEMU (SMOKE_QEMU=0 or qemu-system-i386 missing)"
fi

TINY=${TINY386:-emulator/bin/tiny386_headless}
if [ -x "$TINY" ]; then
  RAN=$((RAN + 1)); export TINY386=$(cd "$(dirname "$TINY")" && pwd)/$(basename "$TINY"); mkdir -p "$OUT/pc"
  sd tests/r11m/seq-s4a.txt "$OUT/pc/seq-a.txt"; sd tests/r11m/seq-s4b.txt "$OUT/pc/seq-b.txt"
  K=build/r11m/pc-pg/r11m-pg.img
  r=$(sh scripts/fix1-tiny-run.sh "$VOL0" "$OUT/pc/seq-a.txt" "$OUT/pc" a wait "$K")
  case "$r" in *"exited=1 rc=0 IMAGE PASS"*) ok "Tiny386: s4a ended by SHUTDOWN.AOU, exit 0, image clean ($r)";; *) bad "Tiny386 s4a: $r";; esac
  r=$(sh scripts/fix1-tiny-run.sh "$OUT/pc/tiny386-a.final.img" "$OUT/pc/seq-b.txt" "$OUT/pc" b wait "$K")
  case "$r" in *"exited=1 rc=0 IMAGE PASS"*) ok "Tiny386: s4b after reboot ended by SHUTDOWN.AOU, exit 0, image clean";; *) bad "Tiny386 s4b: $r";; esac
  grep -q "SUPERB~1.C .*SUPERBASIC_INTERPRETER.C" "$OUT/pc/tiny386-b.txt" && ok "Tiny386: after reboot the long name and its alias are listed" || bad "Tiny386 listing"
  grep -q "guest shutdown request; disk images flushed and closed" "$OUT/pc/tiny386-b.txt.end" && ok "Tiny386: emulator reported flush and close" || bad "Tiny386 flush report"
  r=$(sh scripts/fix1-tiny-run.sh "$VOL0" tests/fix1/seq-shutdown-only.txt "$OUT/pc" n wait "$K")
  case "$r" in *"exited=1 rc=0 IMAGE PASS"*) ok "Tiny386: shutdown right after boot, exit 0";; *) bad "Tiny386 no-write: $r";; esac
  [ "$(sha256sum < "$OUT/pc/tiny386-n.final.img")" = "$(sha256sum < "$VOL0")" ] && ok "Tiny386: no-write volume byte-identical" || bad "Tiny386 no-write changed the volume"
else
  echo "NA   Tiny386 (no emulator/bin/tiny386_headless and TINY386 not set)"
fi
echo "NA   CPU-only i386VM (legacy diagnostic, not a gate; scripts/legacy-i386vm.sh)"
[ $RAN -ge 1 ] || bad "no emulator ran"
[ $BAD = 0 ] && echo "FIX1 SMOKE PASS" || { echo "FIX1 SMOKE FAIL ($BAD)"; exit 1; }
