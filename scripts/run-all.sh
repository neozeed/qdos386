#!/usr/bin/env bash
# QDOS/386 R6: build both targets and run each three times, plus the
# timer-disabled negative.  Exit status is 0 only if every run passes.
#
# Each run must print the seven R6 lines in order (see check_order), and the
# FAT16 disk image must be byte-identical to its hash before the run (the
# driver issues reads only).
#
# External inputs (not redistributed here):
#   I386VM   i386VM harness tree (bin/i386-gcc, bin/as, bin/i386-ld, bin/i386vm)
#   TINY386  tiny386_headless binary built from the Tiny386 tree (unmodified)
#   SEABIOS  directory containing bios.bin and vgabios.bin
# Logs go to logs/r6/.
set -u
cd "$(dirname "$0")/.."
: "${I386VM:=$HOME/i386vm}"
: "${TINY386:=$HOME/tiny386/tiny386_headless}"
: "${SEABIOS:=$HOME/seabios/out}"
export I386VM TINY386 SEABIOS
mkdir -p logs/r6
make i386vm pc apps fat16 I386VM="$I386VM" TINY386="$TINY386" SEABIOS="$SEABIOS" || exit 2
make build/pc/run.ini I386VM="$I386VM" TINY386="$TINY386" SEABIOS="$SEABIOS" >/dev/null || exit 2

DISK=build/fat16/win1.img
PASS6="QDOS/386 R6 FAT16 EXEC PASS"
disk_sha() { sha256sum "$DISK" | cut -d' ' -f1; }
SHA0=$(disk_sha)

# The R6 lines, in the order the guest must print them.  Greedy subsequence
# match: each expected line must appear after the previous one.
check_order() {
  awk '
    BEGIN {
      s[1] = "QDOS/386 R6"
      s[2] = "WIN1_: FAT16 mounted"
      s[3] = "WIN1_HELLO.AOU: opened"
      s[4] = "HELLO: loaded from filesystem"
      s[5] = "Hello from an independently linked i386 a.out!"
      s[6] = "HELLO: exited, status 7"
      s[7] = "QDOS/386 R6 FAT16 EXEC PASS"
      k = 1
    }
    { sub(/\r$/, "") }
    k <= 7 && $0 == s[k] { k++ }
    END { exit (k == 8) ? 0 : 1 }
  ' "$1"
}

status=0
for n in 1 2 3; do
  "$I386VM/bin/i386vm" build/i386vm/qdos386-r6.bin > "logs/r6/i386vm-run$n.log" 2>&1
  rc=$?
  ok=1
  [ $rc -eq 0 ] || ok=0
  check_order "logs/r6/i386vm-run$n.log" || ok=0
  [ "$(disk_sha)" = "$SHA0" ] || ok=0
  [ $ok -eq 1 ] || status=1
  echo "i386VM run $n: exit $rc, $(grep -c '^CHECK .*: ok$' logs/r6/i386vm-run$n.log) checks ok, $(grep -c 'FAIL$' logs/r6/i386vm-run$n.log) FAIL, order $([ $ok -eq 1 ] && echo ok || echo BAD)"

  (cd build/pc && timeout 900 "$TINY386" run.ini > "../../logs/r6/pc-run$n.txt" 2>&1)
  rc=$?
  ok=1
  [ $rc -eq 0 ] || ok=0
  check_order "logs/r6/pc-run$n.txt" || ok=0
  [ "$(disk_sha)" = "$SHA0" ] || ok=0
  [ $ok -eq 1 ] || status=1
  echo "PC run $n: exit $rc, $(grep -c '^CHECK .*: ok$' logs/r6/pc-run$n.txt) checks ok, $(grep -c 'FAIL$' logs/r6/pc-run$n.txt) FAIL, order $([ $ok -eq 1 ] && echo ok || echo BAD)"
done

make notimer I386VM="$I386VM" >/dev/null || exit 2
"$I386VM/bin/i386vm" build/i386vm/qdos386-r6-notimer.bin > logs/r6/i386vm-notimer.log 2>&1
rc=$?
if grep -q "EXEC PASS" logs/r6/i386vm-notimer.log; then
  echo "NEGATIVE (timer off): FALSE PASS"; status=1
else
  echo "NEGATIVE (timer off): no PASS, exit $rc ($(grep -c 'instruction limit exceeded' logs/r6/i386vm-notimer.log) limit message)"
fi
echo "FAT16 disk sha256 before runs: $SHA0"
echo "FAT16 disk sha256 after runs:  $(disk_sha)"
[ "$(disk_sha)" = "$SHA0" ] || status=1
exit $status
