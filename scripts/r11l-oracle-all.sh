#!/bin/sh
# R11L Stage 3: host oracle for the whole positive corpus (every intermediate kept, one directory per program).
#   usage: QDOS386_TOOLS=... sh scripts/r11l-oracle-all.sh        -> logs/r11l/oracle/<NAME>/
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd); cd "$ROOT"
for n in HELLO MACROS FUNCS FPT; do
	extra=; [ $n = MACROS ] && extra="$ROOT/tests/r11l/src/macros.h"
	ORACLE_EXTRA=$extra sh scripts/r11l-oracle.sh "$ROOT/tests/r11l/src/$n.C" "$ROOT/logs/r11l/oracle/$(echo $n | tr A-Z a-z)" $n > /dev/null
	echo "oracle $n: $(grep "$n.AOU" logs/r11l/oracle/$(echo $n | tr A-Z a-z)/ORACLE.SHA256)"
done
