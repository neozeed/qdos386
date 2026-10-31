#!/bin/sh
# R11H: link the kernels with symbols (a.out, not --raw) so that the memory-map check can read
# _bss_end.  The i386VM sources are the same list the Makefile uses for kernel-raw.bin; the PC
# kernel is build/pc-r11h/kernel.aout (make pc PCD=build/pc-r11h).
#   usage: QDOS386_TOOLS=<tree> sh scripts/r11h-maps-link.sh
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd); cd "$ROOT"
: "${QDOS386_TOOLS:?set QDOS386_TOOLS}"
SRCS="kernel/region.c kernel/main.c kernel/jobs.c kernel/clock.c kernel/clock_boot.c kernel/fpu.c kernel/memory.c kernel/channel.c kernel/irq.c kernel/descriptors.c kernel/aout.c kernel/fat16.c kernel/win1.c kernel/cin.c kernel/kbd.c kernel/imgload.c kernel/shellboot.c tests/test_jobs.c tests/test_memory.c tests/test_channels.c tests/test_app.c tests/test_fs.c tests/test_console.c tests/test_dir.c tests/test_shell.c tests/test_r8.c kernel/context.s platform/i386vm/platform.c platform/i386vm/disk.c platform/i386vm/halt.s"
mkdir -p build/r11h-maps
"$QDOS386_TOOLS/bin/i386-gcc" -DQDOS_KCTX_PTRBITS -DR7_SHELL_MODE -DQDOS_LAYOUT_R9 -Ikernel -Itests -Iplatform $SRCS -o build/r11h-maps/i386-r9.aout 2>/dev/null
"$QDOS386_TOOLS/bin/i386-gcc" -DQDOS_KCTX_PTRBITS -DR7_SHELL_MODE -Ikernel -Itests -Iplatform $SRCS -o build/r11h-maps/i386-r8.aout 2>/dev/null
ls -l build/r11h-maps/*.aout
