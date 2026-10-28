#!/bin/sh
# R11L: rebuild everything the self-hosting stages use, in dependency order (host build; see docs/R11L-TOOLCHAIN-PORT.md):
#   SDK libraries (libqdos, libc) -> guest utilities (ENTER, DEL, CC driver, WRTTEST) -> guest tools relinked
#   (AS, CCCP, CC1, LD; the objects come from r11l-try-{gas,gcc,ld}.sh) -> measurement tools -> R11L kernels (plain, paging).
# Needs: QDOS386_TOOLS (historical tree), I386VM (i386VM tree for the harness kernel build).  Output: build/r11l/...
# usage: sh scripts/r11l-build-all.sh [kernels|libs|apps|tools|ms]...   (default: all)
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd); cd "$ROOT"
: "${QDOS386_TOOLS:?}"; : "${I386VM:?}"
what=${*:-libs apps tools ms kernels}
has() { case " $what " in *" $1 "*) return 0;; esac; return 1; }
if has libs; then
	make -C sdk/lib QDOS386_TOOLS=$QDOS386_TOOLS > /dev/null
	make -C sdk/libc QDOS386_TOOLS=$QDOS386_TOOLS > /dev/null
	echo "libs: $(ls -l sdk/lib/libc.a sdk/lib/libqdos.a | awk '{printf "%s %s; ", $9, $5}')"
fi
if has apps; then
	mkdir -p build/r11l/apps build/r11l/stage
	for p in enter del qdcc sum cmp ren; do
		U=$(echo $p | tr a-z A-Z); [ $p = qdcc ] && U=CC
		sh sdk/bin/qdos386-gcc -O -o build/r11l/apps/$U.AOU sdk/examples/r11l/$p.c > /dev/null
	done
	sh sdk/bin/qdos386-gcc -O -o build/r11l/stage/WRTTEST.AOU sdk/examples/r11l/wrttest.c > /dev/null
	make build/apps/shell-l.aout I386VM=$I386VM > /dev/null
	echo "apps: $(ls build/r11l/apps | tr '\n' ' ')"
fi
if has tools; then sh scripts/r11l-link-tools.sh; fi
if has ms; then sh scripts/r11l-build-ms.sh > /dev/null; echo "ms tools built"; fi
if has kernels; then
	for t in r11l-i386vm-fs r11l-i386vm-fs-pg r11l-pc-fs r11l-pc-fs-pg; do make $t I386VM=$I386VM > /dev/null 2>&1 || { echo "kernel target $t FAILED"; exit 1; }; done
	ls -l build/r11l/vm-fs/*.bin build/r11l/vm-fs-pg/*.bin build/r11l/pc-fs/*.img build/r11l/pc-fs-vga/*.img build/r11l/pc-fs-pg/*.img build/r11l/pc-fs-pg-vga/*.img | awk '{print $9, $5}'
fi
