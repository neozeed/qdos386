#!/bin/sh
# R11M: the Stage 5 demonstration / matrix volume: the R11K SDK volume with the R11M shell plus every R11M guest test program.
#   usage: sh scripts/r11m-demovol.sh OUT.img [KIB]
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd); cd "$ROOT"
E=sdk/examples
sh scripts/r11m-mkvol.sh "${1:?usage}" "${2:-3072}" 1 512 CONT1.AOU=$E/CONT1.aout COUNTER.AOU=$E/COUNTER.aout TICKER.AOU=$E/TICKER.aout \
  CONINP.AOU=$E/CONINP.aout JOBT.AOU=$E/JOBT.aout WRITER.AOU=$E/WRITER.aout PATHT.AOU=$E/PATHT.aout DIRFUL.AOU=$E/DIRFUL.aout \
  VFATT.AOU=$E/VFATT.aout >/dev/null
