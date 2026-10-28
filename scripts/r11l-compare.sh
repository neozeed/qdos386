#!/bin/sh
# R11L: compare the intermediates a guest run left on a volume with the host oracle.
#   usage: sh scripts/r11l-compare.sh VOL.img NAME...      (oracle dirs logs/r11l/oracle/<name>/; prints one line per file)
# Exit status 0 only if every compared file is byte-identical.
ROOT=$(cd "$(dirname "$0")/.." && pwd); cd "$ROOT"
IMG=${1:?usage}; shift; bad=0; T=$(mktemp -d)
for n in "$@"; do
	d=logs/r11l/oracle/$(echo $n | tr A-Z a-z)
	for ext in I S O AOU; do
		f=$n.$ext
		if mcopy -n -i "$IMG" "::$f" "$T/$f" 2>/dev/null && [ -f "$T/$f" ]; then
			if cmp -s "$T/$f" "$d/$f"; then echo "IDENTICAL $f $(sha256sum < "$T/$f" | cut -c1-16) $(wc -c < "$T/$f") bytes"
			else echo "DIFFERENT $f guest=$(sha256sum < "$T/$f" | cut -c1-16) oracle=$(sha256sum < "$d/$f" | cut -c1-16)"; bad=1; fi
		else echo "MISSING   $f"; bad=1; fi
	done
done
rm -rf "$T"; exit $bad
