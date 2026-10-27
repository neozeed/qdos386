#!/bin/sh
# QDOS/386 R7 verification matrix.
#
#   build the default (R2-R7 test suite) and R7 shell images
#   three rounds, each: i386VM suite, Tiny386 suite, i386VM shell session
#     (scripted pipe), Tiny386 shell session (PS/2 via -term), i386VM EOF
#   timer-off negatives for the shell image
#   SHA-256 of every image and of the firmware used
#
# Environment (defaults match the build tree used for R7):
#   I386VM   i386VM tree (toolchain for make)     
#   VMP      patched i386VM harness (input status register 0x800004)
#   TINY     tiny386_headless
#   FW       directory with bios.bin and vgabios.bin
# Output: logs/r7/phase5/
set -eu
HERE=$(cd "$(dirname "$0")/.." && pwd)
cd "$HERE"
SCR=/tmp/claude-0/-home-claude/84e7419d-3488-562f-8de9-18b16224538f/scratchpad
I386VM=${I386VM:-$SCR/x/i386vm/i386vm}
VMP=${VMP:-$SCR/x/i386vm-r7/bin/i386vm}
TINY=${TINY:-$SCR/tiny-build/tiny386_headless}
FW=${FW:-$SCR/firmware-known-good}
export I386VM
OUT=logs/r7/phase5
ROUNDS=${ROUNDS:-3}
mkdir -p "$OUT"

make i386vm pc i386vm-shell pc-shell i386vm-shell-notimer
make build/pc/run.ini SEABIOS="$FW" >/dev/null
make build/pc-shell/run.ini PCD=build/pc-shell PCIMG=r7shell.img SEABIOS="$FW" >/dev/null

# Suite check: every release banner, no FAIL line.  Prints a verdict line.
suite_verdict() {
	f=$1
	missing=""
	for p in "R4 CHANNEL PASS" "R5 A.OUT EXEC PASS" "R6 FAT16 EXEC PASS" \
	         "R7 CONSOLE PASS" "R7 SHELL SELFTEST PASS"; do
		grep -q "QDOS/386 $p" "$f" || missing="$missing [$p]"
	done
	fails=$(grep -c ": FAIL" "$f" || true)
	checks=$(grep -c "^CHECK " "$f" || true)
	if [ -z "$missing" ] && [ "$fails" = 0 ]; then
		echo "R7 SUITE PASS ($checks CHECK lines, 0 FAIL)"
	else
		echo "R7 SUITE FAIL (missing:$missing; FAIL lines: $fails)"
	fi
}

for n in $(seq 1 "$ROUNDS"); do
	echo "== round $n =="
	"$VMP" build/i386vm/qdos386-r6.bin </dev/null >"$OUT/i386vm-suite-run$n.log" 2>&1 || true
	suite_verdict "$OUT/i386vm-suite-run$n.log" | sed "s/^/i386vm suite run $n: /"

	(cd build/pc && "$TINY" run.ini </dev/null >"../../$OUT/pc-suite-run$n.log" 2>&1) || true
	suite_verdict "$OUT/pc-suite-run$n.log" | sed "s/^/pc suite run $n: /"

	python3 tests/interactive/session.py --emu i386vm --vm "$VMP" \
		--image build/i386vm-shell/qdos386-r7-shell.bin \
		--log "$OUT/scripted-i386vm-shell-run$n.log" --tag "run $n" || true
	python3 tests/interactive/session.py --emu pc --tiny "$TINY" --dir build/pc-shell \
		--log "$OUT/term-pc-shell-run$n.log" --tag "run $n" || true
	python3 tests/interactive/session.py --emu i386vm --mode eof --vm "$VMP" \
		--image build/i386vm-shell/qdos386-r7-shell.bin \
		--log "$OUT/scripted-i386vm-eof-run$n.log" --tag "run $n" || true
done

# Timer-off negative: the shell prints its prompt, then no input is ever read.
# The run must not reach the shell's exit line; the harness reports the
# instruction limit instead.
printf 'DIR\n' | timeout 300 "$VMP" build/i386vm-shell-notimer/qdos386-r7-shell-notimer.bin \
	>"$OUT/i386vm-shell-notimer.log" 2>&1 || true
if grep -q "SHELL: exited" "$OUT/i386vm-shell-notimer.log"; then
	echo "timer-off negative: FAIL (the shell exited without a timer)"
elif grep -q "10 files" "$OUT/i386vm-shell-notimer.log"; then
	echo "timer-off negative: FAIL (input was read without a timer)"
else
	echo "timer-off negative: ok (no input read, no exit; $(tail -1 "$OUT/i386vm-shell-notimer.log"))"
fi

# Hashes of everything used.
sha256sum build/i386vm/qdos386-r6.bin build/pc/r6pc.img build/fat16/win1.img \
	build/i386vm-shell/qdos386-r7-shell.bin build/pc-shell/r7shell.img \
	build/i386vm-shell-notimer/qdos386-r7-shell-notimer.bin \
	"$FW/bios.bin" "$FW/vgabios.bin" "$VMP" "$TINY" > "$OUT/images.sha256"
cat "$OUT/images.sha256"
