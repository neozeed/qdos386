#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
mkdir -p build/tools
BUILD194="$PWD/build/ld194" bash third_party/binutils-1.94/build-host64.sh >build/tools/ld194-build.log 2>&1
cp build/ld194/i386-ld build/tools/ld194
