#!/bin/sh
# R11J: build every paging-POC kernel image.  Default (non-POC) images are not touched.
#   release POC : build/r11j/vm-poc/qdos386-r11j-poc.bin  (i386VM block mode, kernel only)
#                 build/r11j/pc-poc/r11j-poc.img          (Tiny386 PC)
#                 build/r11j/pc-poc-vga/r11j-poc-vga.img  (VGA console, QEMU)
#   TEST ONLY   : build/r11j/test-vm-flaunch  -DQDOS_TEST_FAULT_LAUNCH=3 (third image load fails)
#                 build/r11j/test-pc-flaunch  the same for the PC
#                 build/r11j/test-vm-cap      -DQDOS_TEST_PG_POOL_BYTES=0x200000 (2 MiB frame pool)
#                 build/r11j/test-vm-intcr3   -DQDOS_TEST_PG_INT_AROUND_CR3 (int 0x27 right before
#                 build/r11j/test-pc-intcr3     and right after every CR3 load)
#                 build/r11j/test-vm-share    -DQDOS_TEST_PG_SHARE_FRAMES: NEGATIVE CONTROL, every
#                                             copy maps the first copy's frames; the proof must FAIL
# Load count for QDOS_TEST_FAULT_LAUNCH: 1 = shell at boot, 2 = RUN SAMELNCH, 3 = its first
# QT_PG_START.  Never use a TEST ONLY image as a release image.
#   usage: I386VM=<historical tree> sh scripts/r11j-images.sh
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd)
cd "$ROOT"
: "${I386VM:?set I386VM to the historical tree}"
export QDOS386_TOOLS=${QDOS386_TOOLS:-$I386VM}
R9="-DR7_SHELL_MODE -DQDOS_LAYOUT_R9"
POC="-DQDOS_PAGING_POC"
vm() {  # vm DIR IMG EXTRA_FLAGS
  make i386vm I386VM="$I386VM" I386D=build/r11j/$1 I386FLAGS="$R9 -DQDOS_I386VM_BLOCK $POC $3" \
    I386IMG=$2 I386WIN= IMGENV="QDOS_LAYOUT=r9" I386EXTRA="kernel/paging.c kernel/paging.s" >/dev/null
  echo "built build/r11j/$1/$2 $(sha256sum build/r11j/$1/$2 | cut -c1-16)"
}
pc() {  # pc DIR IMG EXTRA_FLAGS
  make pc I386VM="$I386VM" PCD=build/r11j/$1 PCFLAGS="-DQDOS_LAYOUT_R9 -DR7_SHELL_MODE $POC $3" \
    PCIMG=$2 PCBOOT=build/pc-r9/boot.bin PCEXTRA="build/r11j/$1/paging.o build/r11j/$1/paging_s.o" >/dev/null
  echo "built build/r11j/$1/$2 $(sha256sum build/r11j/$1/$2 | cut -c1-16)"
}
make build/pc-r9/boot.bin I386VM="$I386VM" >/dev/null
vm vm-poc qdos386-r11j-poc.bin ""
pc pc-poc r11j-poc.img ""
pc pc-poc-vga r11j-poc-vga.img "-DQDOS_CON_VGA"
vm test-vm-flaunch qdos386-r11j-test-flaunch.bin "-DQDOS_TEST_FAULT_LAUNCH=3"
pc test-pc-flaunch r11j-test-flaunch.img "-DQDOS_TEST_FAULT_LAUNCH=3"
vm test-vm-cap qdos386-r11j-test-cap.bin "-DQDOS_TEST_PG_POOL_BYTES=0x200000"
vm test-vm-intcr3 qdos386-r11j-test-intcr3.bin "-DQDOS_TEST_PG_INT_AROUND_CR3"
pc test-pc-intcr3 r11j-test-intcr3.img "-DQDOS_TEST_PG_INT_AROUND_CR3"
vm test-vm-share qdos386-r11j-test-share.bin "-DQDOS_TEST_PG_SHARE_FRAMES"
