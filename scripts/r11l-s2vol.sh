#!/bin/sh
# R11L Stage 2/3/4/5: build a scratch FAT16 volume holding the R11K base programs (SHELL.AOU etc.),
# the guest-native tools (${R11L_TOOLDIR:-build/r11l/guest}/*.AOU) and any extra NAME=path files.
#   usage: sh scripts/r11l-s2vol.sh OUT.img [SIZE_KIB [SPC [ROOT]]] -- [NAME=path ...]
# Defaults 8192 KiB, spc 2, 512 root entries.  Files get a fixed modification time.
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd); cd "$ROOT"
OUT=${1:?usage}; shift
KIB=8192; SPC=2; RT=512
[ "${1:-}" = "--" ] || { KIB=${1:-$KIB}; shift; [ "${1:-}" = "--" ] || { SPC=${1:-$SPC}; shift; [ "${1:-}" = "--" ] || { RT=${1:-$RT}; shift; }; }; }
[ "${1:-}" = "--" ] && shift
# SHELL.AOU: the 160-character-line build (apps/shell/shell.c -DSH_LINE_MAX=160); R11L_SHELL=r11k keeps the R11K one
BASE=$(sh scripts/r11l-base-list.sh | grep -v 'BIG.BIN\|^BAD\|^CHAIN\|^EMPTY' | tr '\n' ' ')
if [ "${R11L_SHELL:-long}" = long ]; then
	BASE=$(echo $BASE | tr ' ' '\n' | grep -v '^SHELL.AOU=' | tr '\n' ' ')
	BASE="SHELL.AOU=build/apps/shell-l.aout $BASE"
fi
TOOLS=
for t in AS CCCP CC1 LD; do [ -f ${R11L_TOOLDIR:-build/r11l/guest}/$t.AOU ] && TOOLS="$TOOLS $t.AOU=${R11L_TOOLDIR:-build/r11l/guest}/$t.AOU"; done
for t in CC ENTER DEL SUM CMP REN; do [ -f build/r11l/apps/$t.AOU ] && TOOLS="$TOOLS $t.AOU=build/r11l/apps/$t.AOU"; done
find build/r11l/apps build/r11l/base ${R11L_TOOLDIR:-build/r11l/guest} build/apps -type f | xargs touch -d '2026-10-28 08:00:00'
sh scripts/r11l-mkvol.sh "$OUT" "$KIB" "$SPC" "$RT" R11LS2 $BASE $TOOLS "$@"
