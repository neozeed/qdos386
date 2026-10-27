#!/bin/sh
# QDOS/386 R9 verification matrix.
#
#   build: the R5-R8 default images (regression), the R9 volume and images,
#          the R9 test-suite images (R7 volume, R9 layout)
#   ROUNDS rounds, each:
#     R8 regression  (default layout, R8 volume): i386VM suite, Tiny386 suite,
#                    i386VM SCRIPTED session, Tiny386 PS/2 session, EOF session
#     R9             (R9 layout, 16 MiB i386VM): i386VM suite, Tiny386 suite,
#                    i386VM SCRIPTED session, Tiny386 PS/2 session, EOF session
#   timer-off negative for the R9 shell image
#   FAT16 volume hashes before and after the rounds, and SHA-256 of every image
#
# Labels: "SCRIPTED" = bytes piped to the i386VM harness; "PS/2 via -term" =
# keys driven by session.py through the Tiny386 keyboard path.  Neither is
# human keyboard input.
#
# Environment:
#   I386VM   the historical toolchain tree (bin/i386-gcc, as, i386-ld)
#   VMP      i386VM harness, 8 MiB, with the R7 input patch (R8 regression)
#   VMR9     i386VM harness, 16 MiB, with the R7 input patch and the R9 RAM patch
#   TINY     tiny386_headless
#   FW       directory with bios.bin and vgabios.bin
# Output: logs/r9/matrix/ and logs/r9-matrix.txt
set -eu
HERE=$(cd "$(dirname "$0")/.." && pwd)
cd "$HERE"
SCR=${SCR:-/tmp/claude-0/-home-claude/84e7419d-3488-562f-8de9-18b16224538f/scratchpad}
I386VM=${I386VM:-$SCR/x/i386vm/i386vm}
VMP=${VMP:-$SCR/x/i386vm-r7/bin/i386vm}
VMR9=${VMR9:-$SCR/x/i386vm-r9/i386vm-r9}
TINY=${TINY:-$SCR/tiny-build/tiny386_headless}
FW=${FW:-$SCR/firmware-known-good}
export I386VM
OUT=logs/r9/matrix
ROUNDS=${ROUNDS:-3}
mkdir -p "$OUT"

# ---- builds ----------------------------------------------------------------
# R8 regression (default layout) and the R8 volume, as in scripts/r8-matrix.sh
make i386vm pc apps i386vm-r8-shell pc-r8-shell i386vm-r8-shell-notimer r8-fat16
make build/pc/run.ini SEABIOS="$FW" >/dev/null
make build/pc-r8/run.ini PCD=build/pc-r8 PCIMG=r8shell.img PCWIN=build/fat16/win1-r8.img SEABIOS="$FW" >/dev/null
# R9 volume and shell images (16 MiB i386VM and 8 MiB Tiny386)
make r9-fat16 r9-i386vm-shell r9-i386vm-shell-notimer r9-pc-shell
make build/pc-r9/run.ini PCD=build/pc-r9 PCIMG=r9shell.img PCWIN=build/fat16/win1-r9.img SEABIOS="$FW" >/dev/null
# R9 test-suite images: the R2-R8 kernel tests on the R9 layout, R7 volume
make i386vm I386D=build/i386vm-r9t I386FLAGS=-DQDOS_LAYOUT_R9 I386IMG=qdos386-r9-suite.bin \
	I386WIN=build/fat16/win1.img IMGENV="QDOS_LAYOUT=r9"
make pc PCD=build/pc-r9t PCFLAGS=-DQDOS_LAYOUT_R9 PCIMG=r9suite.img \
	PCBOOT=build/pc-r9/boot.bin PCWIN=build/fat16/win1.img
make build/pc-r9t/run.ini PCD=build/pc-r9t PCIMG=r9suite.img PCWIN=build/fat16/win1.img SEABIOS="$FW" >/dev/null

# FAT16 volumes must not change during the matrix.
sha256sum build/fat16/win1.img build/fat16/win1-r8.img build/fat16/win1-r9.img > "$OUT/fat16-before.sha256"

# ---- suite verdict: every banner present, no FAIL line ---------------------
suite_verdict() {
	f=$1
	missing=""
	for p in "R4 CHANNEL PASS" "R5 A.OUT EXEC PASS" "R6 FAT16 EXEC PASS" \
	         "R7 CONSOLE PASS" "R8 SHELL SELFTEST PASS"; do
		grep -q "QDOS/386 $p" "$f" || missing="$missing [$p]"
	done
	fails=$(grep -c ": FAIL" "$f" || true)
	r8=$(grep -c "^CHECK.*R8" "$f" || true)
	checks=$(grep -c "^CHECK " "$f" || true)
	if [ -z "$missing" ] && [ "$fails" = 0 ] && [ "$r8" -ge 20 ]; then
		echo "SUITE PASS ($checks CHECK lines, $r8 R8 loader checks, 0 FAIL)"
	else
		echo "SUITE FAIL (missing:$missing; FAIL lines: $fails; R8 checks: $r8)"
	fi
}

session_verdict() {
	# $1 = log file: the session script prints its own summary line.
	if grep -q "SESSION PASS" "$1"; then
		echo "PASS ($(grep -a '^summary:' "$1" | tail -1 | sed 's/summary: //'))"
	else
		echo "FAIL ($(grep -a '^summary:' "$1" | tail -1))"
	fi
}

for n in $(seq 1 "$ROUNDS"); do
	echo "== round $n =="

	# R8 regression on the default layout.
	"$VMP" build/i386vm/qdos386-r6.bin </dev/null >"$OUT/r8-i386vm-suite-run$n.log" 2>&1 || true
	suite_verdict "$OUT/r8-i386vm-suite-run$n.log" | sed "s/^/R8 i386vm suite run $n: /"
	(cd build/pc && "$TINY" run.ini </dev/null >"../../$OUT/r8-pc-suite-run$n.log" 2>&1) || true
	suite_verdict "$OUT/r8-pc-suite-run$n.log" | sed "s/^/R8 pc suite run $n: /"
	python3 tests/interactive/session.py --emu i386vm --vm "$VMP" --steps r8 \
		--image build/i386vm-r8/qdos386-r8-shell.bin \
		--log "$OUT/r8-scripted-i386vm-run$n.log" --tag "run $n" >/dev/null 2>&1 || true
	echo "R8 i386vm SCRIPTED session run $n: $(session_verdict "$OUT/r8-scripted-i386vm-run$n.log")"
	python3 tests/interactive/session.py --emu pc --tiny "$TINY" --steps r8 --dir build/pc-r8 \
		--log "$OUT/r8-term-pc-run$n.log" --tag "run $n" >/dev/null 2>&1 || true
	echo "R8 pc PS/2 session run $n: $(session_verdict "$OUT/r8-term-pc-run$n.log")"
	python3 tests/interactive/session.py --emu i386vm --mode eof --steps r8 --vm "$VMP" \
		--image build/i386vm-r8/qdos386-r8-shell.bin \
		--log "$OUT/r8-scripted-i386vm-eof-run$n.log" --tag "run $n" >/dev/null 2>&1 || true
	echo "R8 i386vm EOF session run $n: $(session_verdict "$OUT/r8-scripted-i386vm-eof-run$n.log")"

	# R9.
	"$VMR9" build/i386vm-r9t/qdos386-r9-suite.bin </dev/null >"$OUT/r9-i386vm-suite-run$n.log" 2>&1 || true
	suite_verdict "$OUT/r9-i386vm-suite-run$n.log" | sed "s/^/R9 i386vm suite run $n: /"
	(cd build/pc-r9t && "$TINY" run.ini </dev/null >"../../$OUT/r9-pc-suite-run$n.log" 2>&1) || true
	suite_verdict "$OUT/r9-pc-suite-run$n.log" | sed "s/^/R9 pc suite run $n: /"
	python3 tests/interactive/session.py --emu i386vm --vm "$VMR9" --steps r9 \
		--image build/i386vm-r9/qdos386-r9-shell.bin \
		--log "$OUT/r9-scripted-i386vm-run$n.log" --tag "run $n" >/dev/null 2>&1 || true
	echo "R9 i386vm SCRIPTED session run $n: $(session_verdict "$OUT/r9-scripted-i386vm-run$n.log")"
	python3 tests/interactive/session.py --emu pc --tiny "$TINY" --steps r9 --dir build/pc-r9 \
		--log "$OUT/r9-term-pc-run$n.log" --tag "run $n" >/dev/null 2>&1 || true
	echo "R9 pc PS/2 session run $n: $(session_verdict "$OUT/r9-term-pc-run$n.log")"
	python3 tests/interactive/session.py --emu i386vm --mode eof --steps r9 --vm "$VMR9" \
		--image build/i386vm-r9/qdos386-r9-shell.bin \
		--log "$OUT/r9-scripted-i386vm-eof-run$n.log" --tag "run $n" >/dev/null 2>&1 || true
	echo "R9 i386vm EOF session run $n: $(session_verdict "$OUT/r9-scripted-i386vm-eof-run$n.log")"
done

# Timer-off negative (R9): the shell prints its prompt, then no input is read.
printf "DIR\n" | timeout 300 "$VMR9" build/i386vm-r9-notimer/qdos386-r9-shell-notimer.bin \
	>"$OUT/r9-i386vm-shell-notimer.log" 2>&1 || true
if grep -q "SHELL: exited" "$OUT/r9-i386vm-shell-notimer.log"; then
	echo "timer-off negative: FAIL (the shell exited without a timer)"
elif grep -q "23 files" "$OUT/r9-i386vm-shell-notimer.log"; then
	echo "timer-off negative: FAIL (input was read without a timer)"
else
	echo "timer-off negative: ok (no input read, no exit; $(tail -1 "$OUT/r9-i386vm-shell-notimer.log"))"
fi

sha256sum -c --quiet "$OUT/fat16-before.sha256" 2>/dev/null || true
sha256sum build/fat16/win1.img build/fat16/win1-r8.img build/fat16/win1-r9.img > "$OUT/fat16-after.sha256"
if cmp -s "$OUT/fat16-before.sha256" "$OUT/fat16-after.sha256"; then
	echo "FAT16 volumes unchanged during the matrix: ok"
else
	echo "FAT16 volumes unchanged during the matrix: FAIL"
fi

# Hashes of everything used.
sha256sum build/i386vm/qdos386-r6.bin build/pc/r6pc.img build/fat16/win1.img \
	build/fat16/win1-r8.img build/i386vm-r8/qdos386-r8-shell.bin build/pc-r8/r8shell.img \
	build/fat16/win1-r9.img build/i386vm-r9t/qdos386-r9-suite.bin build/pc-r9t/r9suite.img \
	build/i386vm-r9/qdos386-r9-shell.bin build/pc-r9/r9shell.img \
	build/i386vm-r9-notimer/qdos386-r9-shell-notimer.bin \
	build/pc-r9/boot.bin \
	"$FW/bios.bin" "$FW/vgabios.bin" "$VMP" "$VMR9" "$TINY" > "$OUT/images.sha256"
cat "$OUT/images.sha256"
