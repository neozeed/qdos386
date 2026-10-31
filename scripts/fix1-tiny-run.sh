#!/bin/sh
# R11M-FIX1: one bounded Tiny386 (full-machine headless) run on a COPY of a scratch volume, then independent image checks.
#   usage: TINY386=<tiny386_headless> sh scripts/fix1-tiny-run.sh VOL.img SEQ OUTDIR TAG END [KERNEL_IMG]
#   END = kill | term | wait   (how the emulator is ended after the last command; see tests/interactive/run_pc_multi.py)
#         "wait": the sequence must end with a "!RUN SHUTDOWN.AOU" line; the emulator must exit by itself within PC_END_WAIT s (30)
# Writes OUTDIR/tiny386-TAG.{txt,stdout,txt.end,final.img,check.txt}.  Prints "RESULT TAG END exited=.. rc=.. IMAGE PASS|FAIL".
# The extra-read workaround of scripts/r11m-run-pc.sh is NOT used here.
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd); cd "$ROOT"
V=${1:?usage}; S=${2:?}; O=${3:?}; TAG=${4:?}; E=${5:?}; K=${6:-build/r11m/pc-pg/r11m-pg.img}
mkdir -p "$O"
PC_END=$E PC_END_WAIT=${PC_END_WAIT:-30} PC_INPUT_PAUSE=${PC_INPUT_PAUSE:-3} PC_CMD_TIMEOUT=${PC_CMD_TIMEOUT:-900} \
  sh scripts/r11l-run-pc.sh "$V" "$S" "$O" "$TAG" "$K" > "$O/tiny386-$TAG.run.log" 2>&1 || true
end=$(head -1 "$O/tiny386-$TAG.txt.end" 2>/dev/null || echo "END mode=$E exited=? rc=?")
img="$O/tiny386-$TAG.final.img"
if sh scripts/fix1-check-image.sh "$img" "$O/tiny386-$TAG.fsck.txt" > "$O/tiny386-$TAG.check.txt" 2>&1; then r=PASS; else r=FAIL; fi
echo "RESULT $TAG $end IMAGE $r"
