#!/bin/bash
# Run the R1 image on the i386VM harness and check the exit status.
# Usage: scripts/run-i386vm.sh [image] ; I386VM selects the harness tree.
set -eu
cd "$(dirname "$0")/.."
I386VM="${I386VM:-$HOME/i386vm}"
IMG="${1:-build/qdos386.bin}"
"$I386VM/bin/i386vm" "$IMG"
