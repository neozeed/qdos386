#!/bin/sh
# R11M-FIX1: the R11M demonstration volume (scripts/r11m-demovol.sh) plus SHUTDOWN.AOU.  The R11M volumes themselves are unchanged.
#   usage: sh scripts/fix1-vol.sh OUT.img [KIB]
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd); cd "$ROOT"
E=sdk/examples
sh scripts/r11m-mkvol.sh "${1:?usage}" "${2:-3072}" 1 512 CONT1.AOU=$E/CONT1.aout COUNTER.AOU=$E/COUNTER.aout TICKER.AOU=$E/TICKER.aout \
  CONINP.AOU=$E/CONINP.aout JOBT.AOU=$E/JOBT.aout WRITER.AOU=$E/WRITER.aout PATHT.AOU=$E/PATHT.aout DIRFUL.AOU=$E/DIRFUL.aout \
  VFATT.AOU=$E/VFATT.aout SHUTDOWN.AOU=$E/SHUTDOWN.aout >/dev/null
