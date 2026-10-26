#!/usr/bin/env bash
# QDOS/386 R5: build both targets and run each three times, plus the
# timer-disabled negative.  Exit status is 0 only if every run passes.
#
# External inputs (not redistributed here):
#   I386VM   i386VM harness tree (bin/i386-gcc, bin/as, bin/i386-ld, bin/i386vm)
#   TINY386  tiny386_headless binary built from the Tiny386 tree
#   SEABIOS  directory containing bios.bin and vgabios.bin
# Logs go to logs/.
set -u
cd "$(dirname "$0")/.."
: "${I386VM:=$HOME/i386vm}"
: "${TINY386:=$HOME/tiny386/tiny386_headless}"
: "${SEABIOS:=$HOME/seabios/out}"
export I386VM TINY386 SEABIOS
mkdir -p logs
make i386vm pc apps I386VM="$I386VM" TINY386="$TINY386" SEABIOS="$SEABIOS" || exit 2
make build/pc/run.ini I386VM="$I386VM" TINY386="$TINY386" SEABIOS="$SEABIOS" >/dev/null || exit 2
status=0
PASS5="QDOS/386 R5 A.OUT EXEC PASS"
for n in 1 2 3; do
  "$I386VM/bin/i386vm" build/i386vm/qdos386-r5.bin > "logs/i386vm-run$n.log" 2>&1
  rc=$?
  grep -qx "$PASS5" "logs/i386vm-run$n.log" && [ $rc -eq 0 ] || status=1
  echo "i386VM run $n: exit $rc, $(grep -c '^CHECK .*: ok$' logs/i386vm-run$n.log) checks ok, $(grep -c 'FAIL$' logs/i386vm-run$n.log) FAIL"
  (cd build/pc && timeout 900 "$TINY386" run.ini > "../../logs/pc-run$n.txt" 2>&1)
  rc=$?
  grep -qx "$PASS5" "logs/pc-run$n.txt" || status=1
  echo "PC run $n: exit $rc, $(grep -c '^CHECK .*: ok$' logs/pc-run$n.txt) checks ok, $(grep -c 'FAIL$' logs/pc-run$n.txt) FAIL"
done
make notimer I386VM="$I386VM" >/dev/null || exit 2
"$I386VM/bin/i386vm" build/i386vm/qdos386-r5-notimer.bin > logs/i386vm-notimer.log 2>&1
rc=$?
if grep -q "A.OUT EXEC PASS" logs/i386vm-notimer.log; then
  echo "NEGATIVE (timer off): FALSE PASS"; status=1
else
  echo "NEGATIVE (timer off): no PASS, exit $rc ($(grep -c 'instruction limit exceeded' logs/i386vm-notimer.log) limit message)"
fi
exit $status
