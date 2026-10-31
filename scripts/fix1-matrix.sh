#!/bin/sh
# R11M-FIX1 Tiny386 full-machine (headless) shutdown/persistence matrix.  Bounded: 3 independent rounds in parallel (4 boots each)
# plus the negative/forced cases.  Needs: TINY386 = the PATCHED tiny386_headless (patches/tiny386-fix1-shutdown.patch), the R11M
# kernels (make r11m-kernels), the SDK examples (make -C sdk/examples SHUTDOWN.aout ...), QDOS386_TOOLS.
#   usage: TINY386=... sh scripts/fix1-matrix.sh OUTDIR [ROUNDS]
# Cases (every line "  ok" or "  FAIL"; the last line is "FIX1 TINY386 MATRIX PASS" only when nothing failed):
#   round N, boot 1  JOBT (job table, WRITER killed -> chain freed in both FAT copies) then SHUTDOWN
#   round N, boot 2  s4a: mkdir, long names, copies, renames, then SHUTDOWN                       (fresh process on boot 1's image)
#   round N, boot 3  s4b: read by long name/alias after reboot, renames, copy, then SHUTDOWN
#   round N, boot 4  s4c: delete everything (chains freed), rmdir, then SHUTDOWN; mtools agrees
#   neg-wrong-magic  SHUTDOWN /W: emulator keeps running; the real request then ends it
#   neg-refuse       a started job exists: SHUTDOWN refuses; after KILL it succeeds
#   neg-no-writes    shutdown right after boot: exit 0, image byte-identical to the initial volume
#   neg-fsync-fail   injected fsync EIO: exit status 3 and a message (no silent success)
#   neg-fflush-fail  injected fflush EIO: exit status 3 and a message
#   info-fsize       injected EFBIG write errors (informational, not judged: see docs/R11M-FIX1-DURABILITY-REPORT.md)
#   term / kill      SIGTERM (clean) and SIGKILL (the forced case: the old mismatch may reproduce; reported, not a failure)
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd); cd "$ROOT"
: "${TINY386:?set TINY386 to the patched tiny386_headless}"
OUT=${1:?usage}; ROUNDS=${2:-3}
mkdir -p "$OUT"; OUT=$(cd "$OUT" && pwd)
export MTOOLS_SKIP_CHECK=1 LC_ALL=C.UTF-8
REAL=$TINY386
KERN=${FIX1_KERNEL:-build/r11m/pc-pg/r11m-pg.img}
sh scripts/fix1-vol.sh "$OUT/initial.img" 3072
sd() { sed '$d' "$1" > "$2"; echo '!RUN SHUTDOWN.AOU' >> "$2"; }     # replace the final EXIT by the shutdown request

one() {                                # one VOL SEQ DIR TAG END -> sets IMG, RES (the RESULT line)
  RES=$(sh scripts/fix1-tiny-run.sh "$1" "$2" "$3" "$4" "$5" "$KERN")
  IMG=$3/tiny386-$4.final.img; TR=$3/tiny386-$4.txt
}

round() {
  r=$1; D=$OUT/round-$r; mkdir -p "$D"; L=$D/judge.txt; : > "$L"; B=0
  ok()  { echo "  ok   r$r: $1" >> "$L"; }
  bad() { echo "  FAIL r$r: $1" >> "$L"; B=$((B+1)); }
  must() { if grep -q -- "$1" "$2"; then ok "$3"; else bad "$3"; fi; }
  gone() { # gone TAG: the emulator exited by itself, status 0, image clean, flush message present
    case "$RES" in *"exited=1 rc=0 IMAGE PASS"*) ok "$1: guest shutdown, exit 0, FAT copies equal, fsck.fat clean, fatwalk 0 errors";; *) bad "$1: $RES";; esac
    grep -q "guest shutdown request; disk images flushed and closed" "$TR.end" && ok "$1: emulator reported flush+close" || bad "$1: no flush/close report"
  }
  cp "$OUT/initial.img" "$D/vol.img"; V=$D/vol.img
  one "$V" tests/fix1/seq-jobt-shutdown.txt "$D" "r${r}b1" wait; V=$IMG
  gone b1; must 'QDOS/386 R11B JOBT PASS' "$TR" "b1: JOBT banner"
  must 'JOBT: 69 checks passed, 0 failed' "$TR" "b1: JOBT 69 checks"
  sd tests/r11m/seq-s4a.txt "$D/seq-b2.txt"; one "$V" "$D/seq-b2.txt" "$D" "r${r}b2" wait; V=$IMG
  gone b2
  must 'SUPERB~1.C .*SUPERBASIC_INTERPRETER.C' "$TR" "b2: DIR SRC lists alias and long name"
  must 'RENAME~1.TXT .*Renamed Long Name.txt' "$TR" "b2: rename to a long name"
  must 'MYDOCU~1 .*<DIR>.*My Documents' "$TR" "b2: long directory name"
  sd tests/r11m/seq-s4b.txt "$D/seq-b3.txt"; one "$V" "$D/seq-b3.txt" "$D" "r${r}b3" wait; V=$IMG
  gone b3
  must 'SUPERB~1.C .*SUPERBASIC_INTERPRETER.C' "$TR" "b3: after reboot the long name and its alias are listed"
  must 'COPYOF~1.C' "$TR" "b3: copy after reboot"
  sd tests/r11m/seq-s4c.txt "$D/seq-b4.txt"; one "$V" "$D/seq-b4.txt" "$D" "r${r}b4" wait; V=$IMG
  gone b4
  mdir -i "$V" :: 2>/dev/null | grep -q "SRC" && bad "b4: SRC still present (mdir)" || ok "b4: mtools mdir: SRC removed"
  mdir -i "$V" ::/ 2>/dev/null | grep -qiE "LONG +TXT" && ok "b4: mtools mdir: unrelated files intact" || bad "b4: mdir lost LONG.TXT"
  if [ $B -eq 0 ]; then echo "ROUND $r PASS" >> "$L"; else echo "ROUND $r FAIL" >> "$L"; fi
}

for r in $(seq 1 "$ROUNDS"); do round "$r" & done

# ---- negative and forced cases (parallel with the rounds)
NL=$OUT/neg.txt; : > "$NL"
nok()  { echo "  ok   $1" >> "$NL"; }
nbad() { echo "  FAIL $1" >> "$NL"; }
neg() {
  D=$OUT/neg; mkdir -p "$D"
  # wrong magic: emulator must keep running; the shell is still alive afterwards
  printf 'RUN SHUTDOWN.AOU /W\nPS\n!RUN SHUTDOWN.AOU\n' > "$D/seq-wrong.txt"
  cp "$OUT/initial.img" "$D/v1.img"; one "$D/v1.img" "$D/seq-wrong.txt" "$D" wrong wait
  grep -q 'SHUTDOWN: no response' "$TR" && nok "wrong-magic: emulator ignored the bad string (helper reports no response)" || nbad "wrong-magic: emulator reacted"
  grep -q 'job(s) in the kernel table' "$TR" && nok "wrong-magic: shell still alive afterwards (PS answered)" || nbad "wrong-magic: no PS answer"
  case "$RES" in *"exited=1 rc=0"*) nok "wrong-magic: the real request afterwards ended the emulator with status 0";; *) nbad "wrong-magic: $RES";; esac
  # refusal while a job runs
  printf 'START COUNTER.AOU 100000 25 /CON:1\nRUN SHUTDOWN.AOU\nKILL 65539\n@PS\n!RUN SHUTDOWN.AOU\n' > "$D/seq-refuse.txt"
  cp "$OUT/initial.img" "$D/v2.img"; one "$D/v2.img" "$D/seq-refuse.txt" "$D" refuse wait
  grep -q 'SHUTDOWN: refused, 1 other job' "$TR" && nok "refuse: SHUTDOWN refuses while a started job exists" || nbad "refuse: no refusal"
  case "$RES" in *"exited=1 rc=0 IMAGE PASS"*) nok "refuse: after KILL the shutdown succeeds, image clean";; *) nbad "refuse: $RES";; esac
  # no writes at all
  cp "$OUT/initial.img" "$D/v3.img"; one "$D/v3.img" tests/fix1/seq-shutdown-only.txt "$D" nowrite wait
  case "$RES" in *"exited=1 rc=0 IMAGE PASS"*) nok "no-writes: orderly exit, status 0";; *) nbad "no-writes: $RES";; esac
  [ "$(sha256sum < "$IMG")" = "$(sha256sum < "$OUT/initial.img")" ] && nok "no-writes: volume byte-identical to the initial volume" || nbad "no-writes: volume changed"
  # injected faults (the guest writes a file so there is something to flush)
  printf 'COPY README.TXT FOO.TXT\n!RUN SHUTDOWN.AOU\n' > "$D/seq-fault.txt"
  for mode in fsync fflush; do
    cp "$OUT/initial.img" "$D/f-$mode.img"
      RES=$(env FIX1_REAL="$REAL" FIX1_MODE=$mode FIX1_SHIM="$ROOT/build/fix1/failsync.so" TINY386="$ROOT/tests/fix1/tiny-wrap.sh" \
            sh scripts/fix1-tiny-run.sh "$D/f-$mode.img" "$D/seq-fault.txt" "$D" "f$mode" wait "$KERN" 2>&1 | tail -1) || true
    case "$RES" in *"exited=1 rc=3"*) nok "fault-$mode: emulator exited by itself with status 3 (failure not silent)";; *) nbad "fault-$mode: $RES";; esac
    grep -q 'FLUSH FAILED' "$D/tiny386-f$mode.txt.end" && nok "fault-$mode: 'FLUSH FAILED' reported" || nbad "fault-$mode: no failure report"
  done
  cp "$OUT/initial.img" "$D/f-fsize.img"
  RES=$(env FIX1_REAL="$REAL" FIX1_MODE=fsize TINY386="$ROOT/tests/fix1/tiny-wrap.sh" PC_END_WAIT=20 \
        sh scripts/fix1-tiny-run.sh "$D/f-fsize.img" "$D/seq-fault.txt" "$D" ffsize wait "$KERN" 2>&1 | tail -1) || true
  echo "  info fsize (EFBIG injection, not judged): $RES" >> "$NL"
  # SIGTERM and SIGKILL after JOBT, no shutdown request
  for e in term kill; do
    cp "$OUT/initial.img" "$D/e-$e.img"; one "$D/e-$e.img" tests/fix1/seq-jobt.txt "$D" "e$e" "$e"
    echo "  info $e: $RES" >> "$NL"
  done
  case "$(cat "$D/tiny386-eterm.check.txt")" in *"IMAGE PASS"*) nok "SIGTERM: image clean (handler leaves the loop and flushes)";; *) nbad "SIGTERM: image not clean";; esac
}
neg &
wait
for r in $(seq 1 "$ROUNDS"); do cat "$OUT/round-$r/judge.txt"; done > "$OUT/judge-rounds.txt"
cat "$OUT/judge-rounds.txt" "$NL" > "$OUT/judge.txt"
if grep -q '  FAIL' "$OUT/judge.txt"; then echo "FIX1 TINY386 MATRIX FAIL" >> "$OUT/judge.txt"; else echo "FIX1 TINY386 MATRIX PASS" >> "$OUT/judge.txt"; fi
cat "$OUT/judge.txt"
