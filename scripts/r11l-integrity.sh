#!/bin/sh
# R11L: disk-integrity report for every FAT16 scratch image under a directory (final images of the runs): fsck.fat -n and
# tools/r11l-fatcheck.py, with the SHA-256 of the image.  Images of deliberately corrupted or interrupted volumes
# (interrupt sweeps, fault injection) are not listed here: their own reports are in the matrix directories.
#   usage: sh scripts/r11l-integrity.sh DIR OUTFILE
set -u
ROOT=$(cd "$(dirname "$0")/.." && pwd); cd "$ROOT"
D=${1:?usage}; O=${2:?usage}; : > "$O"; bad=0
for f in $(find "$D" -name '*.img' ! -path '*interrupt*' ! -path '*/persist/*' | LC_ALL=C sort); do
	if fsck.fat -n "$f" > /dev/null 2>&1; then a=clean; else a=ERRORS; bad=1; fi
	b=$(python3 tools/r11l-fatcheck.py "$f" 2>&1 | tail -1)
	case "$b" in *"0 error(s), 0 warning(s)"*) ;; *) bad=1 ;; esac
	echo "$(sha256sum "$f" | cut -c1-16) fsck.fat=$a | $b" >> "$O"
done
echo "images: $(wc -l < "$O"), problems: $bad" | tee -a "$O"
exit $bad
