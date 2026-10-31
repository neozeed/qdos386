#!/bin/sh
# R11M Stage 5: the integrated demonstration on one emulator (paging kernel, writable FAT16 scratch volume).
#   usage: sh scripts/r11m-demo.sh vm|pc|qemu OUTDIR
# Phase A: PWD/DIR/MD/CD/COPY/TYPE with long names, two programs linked at the same origin on consoles 1 and 2 with focus switching,
# PS/KILL, Phoon, an SDK utility, JOBT.  Phase B boots the kernel again on the resulting image: persistence by long name and alias.
# Judges: scripts/r11m-demo-judge.py (transcripts), fsck.fat, tools/r11m-fatwalk.py, mtools (content compared with the sources).
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd); cd "$ROOT"
EMU=${1:?usage}; OUT=${2:?usage}; mkdir -p "$OUT"
export MTOOLS_SKIP_CHECK=1 LC_ALL=C.UTF-8
VOL=$OUT/demo-initial.img
sh scripts/r11m-demovol.sh "$VOL" 3072
sha256sum "$VOL" > "$OUT/hash-before.txt"
case $EMU in
  vm)   K=build/r11m/vm-pg/qdos386-r11m-pg.bin; SA=tests/r11m/seq-demo-vm-a.txt
        cp "$VOL" "$OUT/work.img"
        sh scripts/r11l-run-vm.sh $K "$OUT/work.img" $SA "$OUT/vm-a.txt" 600; TA=$OUT/vm-a.txt
        sh scripts/r11l-run-vm.sh $K "$OUT/work.img" tests/r11m/seq-demo-b.txt "$OUT/vm-b.txt" 600; TB=$OUT/vm-b.txt; FIN=$OUT/work.img ;;
  pc)   K=build/r11m/pc-pg/r11m-pg.img; SA=tests/r11m/seq-demo-pc-a.txt
        PC_CMD_TIMEOUT=300 sh scripts/r11m-run-pc.sh "$VOL" $SA "$OUT" a $K >/dev/null
        PC_CMD_TIMEOUT=300 sh scripts/r11m-run-pc.sh "$OUT/tiny386-a.final.img" tests/r11m/seq-demo-b.txt "$OUT" b $K >/dev/null
        TA=$OUT/tiny386-a.txt; TB=$OUT/tiny386-b.txt; FIN=$OUT/tiny386-b.final.img ;;
  qemu) K=build/r11m/pc-pg-vga/r11m-pg-vga.img; SA=tests/r11m/seq-demo-a.txt
        R11M_QKERNEL=$K QEMU_CMD_TIMEOUT=300 sh scripts/r11m-run-qemu.sh "$VOL" $SA "$OUT" a 1800 >/dev/null
        R11M_QKERNEL=$K QEMU_CMD_TIMEOUT=300 sh scripts/r11m-run-qemu.sh "$OUT/qemu-a.final.img" tests/r11m/seq-demo-b.txt "$OUT" b 900 >/dev/null
        TA=$OUT/qemu-a.txt; TB=$OUT/qemu-b.txt; FIN=$OUT/qemu-b.final.img ;;
esac
BAD=0
python3 scripts/r11m-demo-judge.py "$EMU" "$TA" "$TB" | tee "$OUT/judge.txt" || BAD=1
fsck.fat -n "$FIN" > "$OUT/fsck.txt" 2>&1 && echo "ok   fsck.fat clean" || { echo "FAIL fsck.fat"; BAD=1; }
python3 tools/r11m-fatwalk.py "$FIN" --strict --vfat > "$OUT/walk.txt" 2>&1 && echo "ok   fatwalk: $(tail -1 "$OUT/walk.txt")" || { echo "FAIL fatwalk"; BAD=1; }
mcopy -n -i "$FIN" ::/SRC/SUPERBASIC_INTERPRETER.C "$OUT/mt-long.bin" && cmp -s "$OUT/mt-long.bin" build/r11m/base/LONG.TXT && echo "ok   mtools reads SRC/SUPERBASIC_INTERPRETER.C, identical to LONG.TXT" || { echo "FAIL mtools long"; BAD=1; }
mcopy -n -i "$FIN" "::/SRC/Release Notes.txt" "$OUT/mt-notes.bin" && cmp -s "$OUT/mt-notes.bin" build/r11m/base/README.TXT && echo "ok   mtools reads SRC/Release Notes.txt, identical to README.TXT" || { echo "FAIL mtools notes"; BAD=1; }
sha256sum "$FIN" > "$OUT/hash-after.txt"; sha256sum "$VOL" | cmp -s - "$OUT/hash-before.txt" && echo "ok   the initial scratch image was not modified (runs used copies)" || { echo "FAIL initial image changed"; BAD=1; }
if [ $BAD = 0 ]; then echo "DEMO-ALL $EMU PASS"; else echo "DEMO-ALL $EMU FAIL"; exit 1; fi
