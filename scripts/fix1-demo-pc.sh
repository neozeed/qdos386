#!/bin/sh
# R11M-FIX1: the R11M integrated demonstration (consoles, job control, Phoon, long names, JOBT; phase A then phase B after a reboot) on the
# PATCHED Tiny386 full-machine headless emulator.  Both phases are ended by the guest (RUN SHUTDOWN.AOU), with no dummy read.
# Judged by scripts/r11m-demo-judge.py (the transcripts), scripts/fix1-check-image.sh (FAT copies, fsck.fat, fatwalk), mtools.
#   usage: TINY386=<patched tiny386_headless> sh scripts/fix1-demo-pc.sh OUTDIR
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd); cd "$ROOT"
: "${TINY386:?}"
OUT=${1:?usage}; mkdir -p "$OUT"
export MTOOLS_SKIP_CHECK=1 LC_ALL=C.UTF-8
K=build/r11m/pc-pg/r11m-pg.img
VOL=$OUT/demo-initial.img
sh scripts/fix1-vol.sh "$VOL" 3072
sha256sum "$VOL" > "$OUT/hash-before.txt"
sd() { sed '$d' "$1" > "$2"; echo '!RUN SHUTDOWN.AOU' >> "$2"; }
sd tests/r11m/seq-demo-pc-a.txt "$OUT/seq-a.txt"; sd tests/r11m/seq-demo-b.txt "$OUT/seq-b.txt"
ra=$(sh scripts/fix1-tiny-run.sh "$VOL" "$OUT/seq-a.txt" "$OUT" a wait "$K"); echo "$ra"
rb=$(sh scripts/fix1-tiny-run.sh "$OUT/tiny386-a.final.img" "$OUT/seq-b.txt" "$OUT" b wait "$K"); echo "$rb"
FIN=$OUT/tiny386-b.final.img; BAD=0
case "$ra" in *"exited=1 rc=0 IMAGE PASS"*) echo "ok   phase A ended by the guest, exit 0, image clean";; *) echo "FAIL phase A: $ra"; BAD=1;; esac
case "$rb" in *"exited=1 rc=0 IMAGE PASS"*) echo "ok   phase B ended by the guest, exit 0, image clean";; *) echo "FAIL phase B: $rb"; BAD=1;; esac
python3 scripts/r11m-demo-judge.py pc "$OUT/tiny386-a.txt" "$OUT/tiny386-b.txt" | tee "$OUT/judge.txt" || BAD=1
mcopy -n -i "$FIN" ::/SRC/SUPERBASIC_INTERPRETER.C "$OUT/mt-long.bin" && cmp -s "$OUT/mt-long.bin" build/r11m/base/LONG.TXT && echo "ok   mtools reads SRC/SUPERBASIC_INTERPRETER.C, identical to LONG.TXT" || { echo "FAIL mtools long"; BAD=1; }
mcopy -n -i "$FIN" "::/SRC/Release Notes.txt" "$OUT/mt-notes.bin" && cmp -s "$OUT/mt-notes.bin" build/r11m/base/README.TXT && echo "ok   mtools reads SRC/Release Notes.txt, identical to README.TXT" || { echo "FAIL mtools notes"; BAD=1; }
sha256sum "$VOL" | cmp -s - "$OUT/hash-before.txt" && echo "ok   the initial scratch image was not modified" || { echo "FAIL initial image changed"; BAD=1; }
if [ $BAD = 0 ]; then echo "FIX1 DEMO pc PASS"; else echo "FIX1 DEMO pc FAIL"; exit 1; fi
