#!/bin/sh
# R11L: stage-by-stage independent comparison of what the GUEST tools left on a volume against the HOST tools,
# each host stage fed with the GUEST's own previous-stage output, so a difference is localised to one stage:
#   cccp:  host cccp(NAME.C)      vs guest NAME.I        (byte-identical expected)
#   cc1:   host cc1(guest NAME.I) vs guest NAME.S        (identical, or equivalent modulo the string pool: r11l-sdiff.py)
#   as:    host as(guest NAME.S)  vs guest NAME.O        (byte-identical expected)
#   ld:    host ld(guest NAME.O)  vs guest NAME.AOU      (byte-identical expected)
# Classification: independent comparison.  usage: QDOS386_TOOLS=... sh scripts/r11l-xcheck.sh VOL.img SRCDIR NAME...
# Exit 0 if every stage is identical or equivalent.
set -u
ROOT=$(cd "$(dirname "$0")/.." && pwd); cd "$ROOT"
T=${QDOS386_TOOLS:?}; IMG=$(realpath "${1:?usage}"); SRC=$(realpath "${2:?}"); shift 2
rc=0
for n in "$@"; do
	W=$(mktemp -d); cd "$W"
	for h in "$ROOT"/sdk/include/*.h; do cp "$h" "$(basename "$h")"; done
	for x in "$SRC"/*.h; do [ -f "$x" ] && cp "$x" "$(basename "$x")"; done
	cp "$SRC/$n.C" "$n.C"; cp "$ROOT/sdk/lib/crt0_libc.s" CRT0.S
	for l in libc.a libm.a libgcc-qd.a libqdos.a; do cp "$ROOT/sdk/lib/$l" "$(echo $l | sed 's/-qd//' | tr a-z A-Z)"; done
	cp "$ROOT/sdk/ld/slot1.ld" SLOT1.LD
	for e in I S O $([ -n "${XCHECK_NOLD:-}" ] || echo AOU); do mcopy -n -i "$IMG" "::$n.$e" "G.$e" 2>/dev/null || { echo "$n: guest $n.$e MISSING"; rc=1; }; done
	"$T/bin/cccp" -nostdinc -undef -Di386 -D__i386__ -I. "$n.C" H.I
	CC1=${R11L_CC1:-$T/bin/cc1}
	cp G.I "$n.I"; "$CC1" -quiet -O "$n.I" -o H.S
	"$T/bin/as" -o H.O G.S
	"$T/bin/as" -o CRT0.O CRT0.S
	[ -n "${XCHECK_NOLD:-}" ] || "$T/bin/i386-ld" -N -T SLOT1.LD -e _start -o H.AOU CRT0.O G.O LIBM.A LIBC.A LIBGCC.A LIBQDOS.A
	r=; cmp -s H.I G.I && r="cccp=IDENTICAL" || { r="cccp=DIFFERENT"; rc=1; }
	if cmp -s H.S G.S; then r="$r cc1=IDENTICAL"; else python3 "$ROOT/tools/r11l-sdiff.py" H.S G.S > /dev/null; case $? in 1) r="$r cc1=EQUIVALENT(string-pool)";; *) r="$r cc1=DIFFERENT"; rc=1;; esac; fi
	cmp -s H.O G.O && r="$r as=IDENTICAL" || { r="$r as=DIFFERENT"; rc=1; }
	if [ -z "${XCHECK_NOLD:-}" ]; then cmp -s H.AOU G.AOU && r="$r ld=IDENTICAL" || { r="$r ld=DIFFERENT"; rc=1; }; else r="$r ld=n/a"; fi
	echo "$n: $r"
	cd "$ROOT"; rm -rf "$W"
done
exit $rc
