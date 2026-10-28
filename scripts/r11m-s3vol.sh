#!/bin/sh
# R11M: the Stage 2/3 test volume (console and job-control programs on top of the R11K SDK volume).  usage: sh scripts/r11m-s3vol.sh OUT.img [KIB]
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd); cd "$ROOT"
E=sdk/examples
sh scripts/r11m-mkvol.sh "${1:?usage}" "${2:-3072}" 1 512 CONT1.AOU=$E/CONT1.aout COUNTER.AOU=$E/COUNTER.aout TICKER.AOU=$E/TICKER.aout \
  CONINP.AOU=$E/CONINP.aout JOBT.AOU=$E/JOBT.aout WRITER.AOU=$E/WRITER.aout >/dev/null
