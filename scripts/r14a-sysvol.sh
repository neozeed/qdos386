#!/bin/sh
# R14A example system volume: the R14 volume plus the native Hack 1.03 playground /GAMES/HACK:
#   HACK.AOU (build/r14a/hack/src/HACK.AOU), the game's text files from the source tree (data, help, hh, rumors - the LF
#   copies that match this source; the QL test/ copies differ, docs/R14A-HACK-INTAKE.md), the empty perm and record files of the Hack
#   distribution (Makefile: "the distribution directory also contains the empty files perm and record") and the save/
#   directory that SAVEF ("save/%d%s") needs.  Level, save and bones_NN files are created by the game.
#   usage: sh scripts/r14a-sysvol.sh OUT.img [KIB] [HACKDIR]     (R14A_NO_SAVEDIR=1 leaves save/ out, for the error test)
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd); cd "$ROOT"
export MTOOLS_SKIP_CHECK=1 LC_ALL=C.UTF-8
OUT=${1:?usage}; KIB=${2:-8192}; H=${3:-build/r14a/hack/src}
sh scripts/r14-sysvol.sh "$OUT" "$KIB" >/dev/null
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
put() { cp "$1" "$T/f"; touch -d '2026-10-29 08:00:00' "$T/f"; mcopy -o -m -i "$OUT" "$T/f" "$2"; }
mmd -i "$OUT" ::/GAMES 2>/dev/null || true
mmd -i "$OUT" ::/GAMES/HACK
put "$H/HACK.AOU" ::/GAMES/HACK/HACK.AOU
for f in data help hh rumors; do put "third_party/hack103-ql/orig/$f" "::/GAMES/HACK/$f"; done
: > "$T/perm"; put "$T/perm" ::/GAMES/HACK/perm; put "$T/perm" ::/GAMES/HACK/record   # empty, as the Hack distribution
[ "${R14A_NO_SAVEDIR:-0}" = 1 ] || mmd -i "$OUT" ::/GAMES/HACK/save
fsck.fat -n "$OUT" >/dev/null
python3 tools/r11m-fatwalk.py "$OUT" --vfat --strict | tail -1
echo "R14A system volume $OUT: fsck clean"
