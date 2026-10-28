#!/bin/sh
# R11L Gates 2 and 3 (guest-native GAS 1.38, a.out ld, cccp, cc1) on one platform, judged against the host.
#   usage: QDOS386_TOOLS=... [TINY386=...] sh scripts/r11l-gate23.sh vm|pc|qemu OUTDIR
# Runs tests/r11l/seq-stage3.txt (HELLO, MACROS, FUNCS, FPT through cccp, cc1, as, ld and run; three malformed sources
# through cccp and cc1) on a scratch copy of a fresh volume, then:
#   * independent comparison, stage by stage, of every intermediate left on the volume with the host tools
#     (scripts/r11l-xcheck.sh: identical, or equivalent modulo cc1's string pool) and, for HELLO, the whole-file oracle
#     (scripts/r11l-compare.sh);
#   * guest executed: the programs' own exit statuses and PASS/FAIL lines, and the exit statuses of the malformed-source runs
#     against the host oracle's (logs/r11l/oracle/negative.txt);
#   * fsck.fat and tools/r11l-fatcheck.py on the final volume.
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd); cd "$ROOT"
P=${1:?usage}; O=${2:?usage}; mkdir -p "$O"
V=$O/gate23-$P.initial.img
SRCS=; for f in tests/r11l/src/*; do SRCS="$SRCS $(basename "$f" | tr a-z A-Z)=$f"; done
sh scripts/r11l-pipevol.sh "$V" $SRCS > /dev/null
cp "$V" "$O/gate23-$P.final.img"
S=tests/r11l/seq-stage3.txt
case $P in
vm) sh scripts/r11l-run-vm.sh build/r11l/vm-fs/qdos386-r11l-fs.bin "$O/gate23-vm.final.img" $S "$O/gate23-vm.txt" 900
    T=$O/gate23-vm.txt ;;
pc) PC_CMD_TIMEOUT=900 PC_INPUT_PAUSE=3 sh scripts/r11l-run-pc.sh "$V" $S "$O" gate23 build/r11l/pc-fs/r11l-fs.img > /dev/null
    mv "$O/tiny386-gate23.final.img" "$O/gate23-pc.final.img"; T=$O/tiny386-gate23.txt ;;
qemu) R11L_QKERNEL=build/r11l/pc-fs-vga/r11l-fs-vga.img sh scripts/r11l-run-qemu.sh "$V" $S "$O" gate23 3000 > /dev/null
    mv "$O/qemu-gate23.final.img" "$O/gate23-qemu.final.img"; T=$O/qemu-gate23.txt ;;
esac
F=$O/gate23-$P.final.img
rc=0
echo "== stage-wise comparison with the host tools ($P)"
sh scripts/r11l-xcheck.sh "$F" tests/r11l/src HELLO MACROS FUNCS FPT || rc=1
echo "== HELLO against the whole-file oracle"
sh scripts/r11l-compare.sh "$F" HELLO || rc=1
echo "== guest transcript judge"
python3 -I tests/r11l/gate23_judge.py "$T" logs/r11l/oracle/negative.txt | tee "$O/gate23-$P.judge.txt" | grep -v '^PASS' || true
grep -q 'GATE23 SUMMARY fail=0' "$O/gate23-$P.judge.txt" || rc=1
echo "== volume integrity"
fsck.fat -n "$F" > "$O/gate23-$P.fsck.txt" 2>&1 && echo "fsck.fat clean" || { echo "fsck.fat ERRORS"; rc=1; }
python3 tools/r11l-fatcheck.py "$F" | tail -1
exit $rc
