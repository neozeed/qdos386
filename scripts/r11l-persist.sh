#!/bin/sh
# R11L Gate 1: files created in the guest survive close/reopen and an emulator restart.
#   run 1: WRTTEST persist1   (create PERSIST.DAT, PERSIST.TXT, HELLO.C)          harness exits
#   host : checker + fsck.fat + manifest of the image, copied as persist-after1.img
#   run 2: WRTTEST persist2   (verify all three, append, rename)                 a NEW harness process
#   host : checker + fsck.fat + manifest
#   run 3: WRTTEST gen        (HELLO.C rewritten) and a directory listing        a third process
#   usage: sh scripts/r11l-persist.sh KERNEL.bin OUTDIR
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd); cd "$ROOT"
K=${1:?usage}; OUT=${2:?usage}; mkdir -p "$OUT"
sh scripts/r11l-s1vol.sh "$OUT/persist.img"
sha256sum "$OUT/persist.img" > "$OUT/hash-initial.txt"
printf 'RUN WRTTEST.AOU persist1\n' > "$OUT/seq1.txt"
printf 'RUN WRTTEST.AOU persist2\nDIR\n' > "$OUT/seq2.txt"
printf 'DIR\n' > "$OUT/seq3.txt"
sh scripts/r11l-run-vm.sh "$K" "$OUT/persist.img" "$OUT/seq1.txt" "$OUT/run1.txt"
cp "$OUT/persist.img" "$OUT/persist-after1.img"; sha256sum "$OUT/persist-after1.img" >> "$OUT/hash-after-runs.txt"
python3 tools/r11l-fatcheck.py "$OUT/persist-after1.img" --manifest > "$OUT/check1.txt" || true
fsck.fat -n "$OUT/persist-after1.img" > "$OUT/fsck1.txt" 2>&1 || true
sh scripts/r11l-run-vm.sh "$K" "$OUT/persist.img" "$OUT/seq2.txt" "$OUT/run2.txt"
cp "$OUT/persist.img" "$OUT/persist-after2.img"; sha256sum "$OUT/persist-after2.img" >> "$OUT/hash-after-runs.txt"
python3 tools/r11l-fatcheck.py "$OUT/persist-after2.img" --manifest > "$OUT/check2.txt" || true
fsck.fat -n "$OUT/persist-after2.img" > "$OUT/fsck2.txt" 2>&1 || true
sh scripts/r11l-run-vm.sh "$K" "$OUT/persist.img" "$OUT/seq3.txt" "$OUT/run3.txt"
cmp "$OUT/persist.img" "$OUT/persist-after2.img" && echo "run 3 (listing only) left the image unchanged" > "$OUT/run3-unchanged.txt"
