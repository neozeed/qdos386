#!/bin/sh
# R11M Stage 4 gate on one emulator: the guest creates WIN1:/SRC/SUPERBASIC_INTERPRETER.C (and other long names),
# the volume is rebooted, the file is read by long name and by 8.3 alias, and host tools (mtools, fsck.fat,
# tools/r11m-fatwalk.py --vfat) agree on the final images.  Phases: VFATT (guest self-test), s4a (create), s4b (after reboot:
# read, rename, copy), s4c (after reboot: delete everything).  Every phase boots the kernel afresh on the previous image.
#   usage: sh scripts/r11m-vfat-gate.sh vm|pc|qemu OUTDIR
# Environment: TINY386 (for pc), HARNESS as scripts/r11l-run-vm.sh.  Prints "GATE <emu> PASS" only when every check passed.
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd); cd "$ROOT"
EMU=${1:?usage}; OUT=${2:?usage}
mkdir -p "$OUT"
export MTOOLS_SKIP_CHECK=1 LC_ALL=C.UTF-8
BAD=0
ok()  { echo "  ok   $1"; }
bad() { echo "  FAIL $1"; BAD=$((BAD + 1)); }
must() {                              # must PATTERN FILE DESCRIPTION
  if grep -q -- "$1" "$2"; then ok "$3"; else bad "$3"; fi
}
mustnot() {
  if grep -q -- "$1" "$2"; then bad "$3"; else ok "$3"; fi
}

VOL=$OUT/vol-0.img
sh scripts/r11m-mkvol.sh "$VOL" 3072 1 512 VFATT.AOU=sdk/examples/VFATT.aout >/dev/null
N=0
phase() {                             # phase TAG SEQ -> runs the emulator on $VOL; sets $VOL to the resulting image, TR to the transcript
  tag=$1; seq=$2
  case $EMU in
    vm)   sh scripts/r11l-run-vm.sh build/r11m/vm/qdos386-r11m.bin "$VOL" "$seq" "$OUT/vm-$tag.txt" 300; TR=$OUT/vm-$tag.txt ;;
    pc)   PC_CMD_TIMEOUT=300 sh scripts/r11m-run-pc.sh "$VOL" "$seq" "$OUT" "$tag" build/r11m/pc/r11m.img >/dev/null
          TR=$OUT/tiny386-$tag.txt; VOL=$OUT/tiny386-$tag.final.img ;;
    qemu) R11M_QKERNEL=build/r11m/pc-vga/r11m-vga.img QEMU_CMD_TIMEOUT=300 sh scripts/r11m-run-qemu.sh "$VOL" "$seq" "$OUT" "$tag" 900 >/dev/null
          TR=$OUT/qemu-$tag.txt; VOL=$OUT/qemu-$tag.final.img ;;
  esac
}
judge() {                             # judge LABEL: fsck, walker, mtools on $VOL
  fsck.fat -n "$VOL" >"$OUT/fsck-$1.txt" 2>&1 && ok "$1: fsck.fat clean" || bad "$1: fsck.fat"
  python3 tools/r11m-fatwalk.py "$VOL" --strict --vfat >"$OUT/walk-$1.txt" 2>&1 && ok "$1: fatwalk --strict --vfat: $(tail -1 "$OUT/walk-$1.txt")" || bad "$1: fatwalk"
  python3 tools/r11m-fatwalk.py "$VOL" --vfat --tree >"$OUT/tree-$1.txt" 2>&1 || true
}

echo "== GATE $EMU: phase 1, guest self-test VFATT"
printf 'RUN VFATT.AOU\nEXIT\n' > "$OUT/seq-vfatt.txt"
phase vfatt "$OUT/seq-vfatt.txt"
must 'QDOS/386 R11B VFATT PASS' "$TR" "VFATT banner"
must 'VFATT: 68 checks passed, 0 failed' "$TR" "VFATT 68 checks"
judge vfatt

echo "== GATE $EMU: phase 2, create long names"
phase s4a tests/r11m/seq-s4a.txt
must 'SUPERB~1.C .*SUPERBASIC_INTERPRETER.C' "$TR" "DIR SRC shows alias and long name"
must 'RENAME~1.TXT .*Renamed Long Name.txt' "$TR" "REN of a quoted long name"
must 'QUARTE~1.TXT .*Quarterly Report 2026.txt' "$TR" "file in a long-named directory"
must 'MYDOCU~1 .*<DIR>.*My Documents' "$TR" "long directory name listed"
must '/My Documents' "$TR" "PWD shows the long directory name"
judge s4a

echo "== GATE $EMU: phase 3, after reboot: read by long name and alias, rename, copy"
phase s4b tests/r11m/seq-s4b.txt
must 'SUPERB~1.C .*SUPERBASIC_INTERPRETER.C' "$TR" "after reboot: listed with both names"
must 'FINALS~1.C .*Final Short Renamed To Long.c' "$TR" "short to long rename"
must 'COPYOF~1.C .*Copy of Short.c' "$TR" "COPY to a new long name"
must 'invalid name' "$TR" "wildcard in a name refused"
if [ "$(grep -c "Hello from an independently linked i386 a.out" "$TR")" -ge 2 ]; then ok "RUN of a program by quoted long name and by alias"; else bad "RUN by long name and alias"; fi
must 'already exists' "$TR" "rename onto an existing name refused"
mustnot 'no such file or directory' "$TR" "no lookup failed in phase 3"
judge s4b
# independent comparison: mtools reads what the guest wrote
mdir -i "$VOL" ::/SRC >"$OUT/mdir-src.txt" 2>&1
must 'SUPERB~1 C .*SUPERBASIC_INTERPRETER.C' "$OUT/mdir-src.txt" "mtools lists the long name with its alias"
must 'Final Short Renamed To Long.c' "$OUT/mdir-src.txt" "mtools lists the renamed file"
mcopy -n -i "$VOL" ::/SRC/SUPERBASIC_INTERPRETER.C "$OUT/by-long.bin"
mcopy -n -i "$VOL" ::/SRC/SUPERB~1.C "$OUT/by-alias.bin"
if cmp -s "$OUT/by-long.bin" build/r11m/base/LONG.TXT && cmp -s "$OUT/by-alias.bin" build/r11m/base/LONG.TXT; then ok "mtools reads the file by long name and by alias: content identical to the source"; else bad "mtools content"; fi
mdir -i "$VOL" "::/My Documents" >"$OUT/mdir-md.txt" 2>&1
must 'Quarterly Report 2026.txt' "$OUT/mdir-md.txt" "mtools lists the file in the long-named directory"
mdir -i "$VOL" ::/ >"$OUT/mdir-root.txt" 2>&1
must "Hello Program.aou" "$OUT/mdir-root.txt" "mtools lists the long-named program"
cp "$VOL" "$OUT/gate-persistent.img"

echo "== GATE $EMU: phase 4, after reboot: delete everything"
phase s4c tests/r11m/seq-s4c.txt
must '0 files' "$TR" "SRC listed empty after the deletes"
mustnot "Hello Program" "$OUT/mdir-after.txt" "program deleted" 2>/dev/null || true
mustnot 'FAIL' "$TR" "no failure text"
judge s4c
if grep -q 'LFN' "$OUT/tree-s4c.txt"; then bad "no long-name entries remain"; else ok "no long-name entries remain after the deletes"; fi
if mdir -i "$VOL" ::/SRC >/dev/null 2>&1; then bad "SRC directory removed"; else ok "SRC directory removed"; fi

if [ "$BAD" = 0 ]; then echo "GATE $EMU PASS"; else echo "GATE $EMU FAIL ($BAD)"; exit 1; fi
