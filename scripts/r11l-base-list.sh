#!/bin/sh
# R11L: the files of the R11K volume that every R11L scratch volume starts from (SHELL.AOU and the
# SDK programs), extracted by tools/r11j-extract-volume.py.  Prints NAME=path arguments.
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd); cd "$ROOT"
rm -rf build/r11l/base
python3 tools/r11j-extract-volume.py build/fat16/win1-r11k.img build/r11l/base
