#!/bin/sh
# R11L: scripted i386VM run on the writable block harness.  The disk is opened read-write only when
# MODE=rw (default rw); MODE=ro runs the same kernel on a read-only device.
#   usage: sh scripts/r11l-run-vm.sh KERNEL.bin DISK.img SEQUENCE OUT.txt [TIMEOUT_S]
# Environment: QDOS_BLK_FAULT=io:N | exit:N passes through to the harness.
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd); cd "$ROOT"
H=${R11L_HARNESS:-build/r11l/harness/i386vm-r9-blockrw}
K=${1:?usage}; D=${2:?}; S=${3:?}; O=${4:?}; T=${5:-900}
if [ "${MODE:-rw}" = rw ]; then RW=rw; else RW=; fi
rc=0
# sequence lines may carry a one-character prefix ("!" console input for the running program, "@" short wait):
# the harness takes plain lines, so exactly one such prefix is removed
sed 's/^[!@]//' "$S" > "$O.in"
timeout "$T" "$H" "$K" "$D" $RW < "$O.in" > "$O" 2>&1 || rc=$?
echo "harness rc=$rc" >> "$O"
