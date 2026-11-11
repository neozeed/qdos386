#!/bin/sh
# R16B test volume: the R14A system volume plus the protected R16T, TOP, CPUSET and a CPL3-marked BOUNCE in /BIN.  With "mbr" as the second
# argument the result is wrapped as an MBR FAT16 partition at LBA 2048 (the R16A data-disk layout).
#   usage: sh scripts/r16b-sysvol.sh OUT.img [mbr]
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd); cd "$ROOT"
OUT=${1:?}; T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
R14_BIN_EXTRA="R16T.AOU=build/r16/R16TP.aout TOP.AOU=build/r16b/TOPP.aout CPUSET.AOU=build/r16b/CPUSETP.aout BOUNCEP.AOU=build/r16b/BOUNCEP.aout" \
	sh scripts/r14a-sysvol.sh "$T/sys.img" > /dev/null
if [ "${2:-}" = mbr ]; then sh scripts/r16a-wrap-mbr.sh "$T/sys.img" "$OUT" > /dev/null; else cp "$T/sys.img" "$OUT"; fi
