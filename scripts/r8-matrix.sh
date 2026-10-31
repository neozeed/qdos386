#!/bin/sh
# QDOS/386 R8 verification matrix.
#
#   build the test images, the R7 shell images (regression) and the R8 images
#   ROUNDS rounds, each:
#     i386VM test suite (R2-R8 kernel tests on the R7 volume)
#     Tiny386 test suite
#     i386VM R8 shell session on the R8 volume (scripted pipe)
#     Tiny386 R8 shell session on the R8 volume (PS/2 via -term, scripted)
#     i386VM R8 EOF session
#   timer-off negative for the R8 shell image
#   SHA-256 of every image and of the firmware and harnesses used
#
# Labels: "SCRIPTED" = bytes piped to the i386VM harness; "PS/2 via -term" =
# keys driven by session.py through the Tiny386 keyboard path.  Neither is
# human keyboard input.
#
# Environment (defaults match the build tree used for R8):
#   I386VM   i386VM tree (toolchain for make)
#   VMP      patched i386VM harness (input status register 0x800004)
#   TINY     tiny386_headless
#   FW       directory with bios.bin and vgabios.bin
# Output: logs/r8/matrix/
set -eu
HERE=$(cd "$(dirname "$0")/.." && pwd)
cd "$HERE"
SCR=/tmp/claude-0/-home-claude/84e7419d-3488-562f-8de9-18b16224538f/scratchpad
I386VM=${I386VM:-$SCR/x/i386vm/i386vm}
VMP=${VMP:-$SCR/x/i386vm-r7/bin/i386vm}
TINY=${TINY:-$SCR/tiny-build/tiny386_headless}
FW=${FW:-$SCR/firmware-known-good}
export I386VM
OUT=logs/r8/matrix
ROUNDS=${ROUNDS:-3}
mkdir -p "$OUT"

make i386vm pc i386vm-shell pc-shell i386vm-r8-shell pc-r8-shell i386vm-r8-shell-notimer apps r8-fat16
make build/pc/run.ini SEABIOS="$FW" >/dev/null
make build/pc-r8/run.ini PCD=build/pc-r8 PCIMG=r8shell.img PCWIN=build/fat16/win1-r8.img SEABIOS="$FW" >/dev/null

# Suite check: every banner, no FAIL line.  Prints a verdict line.
suite_verdict() {
	f=$1
	missing=""
	for p in "R4 CHANNEL PASS" "R5 A.OUT EXEC PASS" "R6 FAT16 EXEC PASS" \
	         "R7 CONSOLE PASS" "R8 SHELL SELFTEST PASS"; do
		grep -q "QDOS/386 $p" "$f" || missing="$missing [$p]"
	done
	fails=$(grep -c ": FAIL" "$f" || true)
	r8=$(grep -c "^CHECK R8 " "$f" || true)
	checks=$(grep -c "^CHECK " "$f" || true)
	if [ -z "$missing" ] && [ "$fails" = 0 ] && [ "$r8" -ge 22 ]; then
		echo "SUITE PASS ($checks CHECK lines, $r8 R8 loader checks, 0 FAIL)"
	else
		echo "SUITE FAIL (missing:$missing; FAIL lines: $fails; R8 checks: $r8)"
	fi
}

for n in $(seq 1 "$ROUNDS"); do
	echo "== round $n =="
	"$VMP" build/i386vm/qdos386-r6.bin </dev/null >"$OUT/i386vm-suite-run$n.log" 2>&1 || true
	suite_verdict "$OUT/i386vm-suite-run$n.log" | sed "s/^/i386vm suite run $n: /"

	(cd build/pc && "$TINY" run.ini </dev/null >"../../$OUT/pc-suite-run$n.log" 2>&1) || true
	suite_verdict "$OUT/pc-suite-run$n.log" | sed "s/^/pc suite run $n: /"

	python3 tests/interactive/session.py --emu i386vm --vm "$VMP" --steps r8 \
		--image build/i386vm-r8/qdos386-r8-shell.bin \
		--log "$OUT/scripted-i386vm-r8-run$n.log" --tag "run $n" || true
	python3 tests/interactive/session.py --emu pc --tiny "$TINY" --steps r8 --dir build/pc-r8 \
		--log "$OUT/term-pc-r8-run$n.log" --tag "run $n" || true
	python3 tests/interactive/session.py --emu i386vm --mode eof --steps r8 --vm "$VMP" \
		--image build/i386vm-r8/qdos386-r8-shell.bin \
		--log "$OUT/scripted-i386vm-r8-eof-run$n.log" --tag "run $n" || true
done

# Timer-off negative: the shell prints its prompt, then no input is ever read.
# The run must not reach the shell's exit line, and must not list the volume.
printf "DIR\n" | timeout 300 "$VMP" build/i386vm-r8-notimer/qdos386-r8-shell-notimer.bin \
	>"$OUT/i386vm-r8-shell-notimer.log" 2>&1 || true
if grep -q "SHELL: exited" "$OUT/i386vm-r8-shell-notimer.log"; then
	echo "timer-off negative: FAIL (the shell exited without a timer)"
elif grep -q "18 files" "$OUT/i386vm-r8-shell-notimer.log"; then
	echo "timer-off negative: FAIL (input was read without a timer)"
else
	echo "timer-off negative: ok (no input read, no exit; $(tail -1 "$OUT/i386vm-r8-shell-notimer.log"))"
fi

# Hashes of everything used.
sha256sum build/i386vm/qdos386-r6.bin build/pc/r6pc.img build/fat16/win1.img \
	build/fat16/win1-r8.img build/i386vm-r8/qdos386-r8-shell.bin build/pc-r8/r8shell.img \
	build/i386vm-r8-notimer/qdos386-r8-shell-notimer.bin \
	build/apps/args.aout build/apps/large.aout build/apps/shell.aout \
	"$FW/bios.bin" "$FW/vgabios.bin" "$VMP" "$TINY" > "$OUT/images.sha256"
cat "$OUT/images.sha256"
