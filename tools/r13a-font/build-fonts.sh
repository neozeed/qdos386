#!/bin/sh
# Compatibility entry point; canonical font inputs, tables and tooling live in source/.
# Usage: sh tools/r13a-font/build-fonts.sh [OUTDIR]
set -eu
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
exec sh "$ROOT/source/tools/r13a-font/build-fonts.sh" "$@"
