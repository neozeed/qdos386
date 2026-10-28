#!/bin/sh
# R11K: build every kernel image.  Default (non-paging) images with the ORIGINAL boot sector are not
# touched by this script (they are the R11H/R11I images, rebuilt by their own targets; R11K checks that
# they are byte-identical, see scripts/r11k-default-check.sh).
#   release paging : build/r11k/vm-paging/qdos386-r11k-paging.bin   i386VM block mode, kernel only
#                    build/r11k/pc-paging/r11k-paging.img           Tiny386 PC, R11K boot sector
#                    build/r11k/pc-paging-vga/r11k-paging-vga.img   VGA console (QEMU), R11K boot sector
#   release non-paging with the R11K boot sector (so the new loader is shown independent of paging):
#                    build/r11k/pc-default/r11k-default.img  and  build/r11k/pc-default-vga/r11k-default-vga.img
#   TEST ONLY      : test-vm-flaunch / test-pc-flaunch  -DQDOS_TEST_FAULT_LAUNCH=3   (third image load fails)
#                    test-vm-cap                        -DQDOS_TEST_PG_POOL_BYTES=0x200000
#                    test-vm-intcr3 / test-pc-intcr3    -DQDOS_TEST_PG_INT_AROUND_CR3
#                    test-vm-share                      -DQDOS_TEST_PG_SHARE_FRAMES (NEGATIVE CONTROL: must FAIL)
#                    test-vm-tasks / test-pc-tasks      -DQDOS_TEST_PG_TASKS (kernel poll/schedule tasks)
#                    test-pc-bigk-paging(-vga)          paging kernel padded to a large size (-DQDOS_TEST_BIGK)
# Never use a TEST ONLY image as a release image.
#   usage: I386VM=<historical tree> sh scripts/r11k-images.sh
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd)
cd "$ROOT"
: "${I386VM:?set I386VM to the historical tree}"
export QDOS386_TOOLS=${QDOS386_TOOLS:-$I386VM}
R9="-DR7_SHELL_MODE -DQDOS_LAYOUT_R9"
PG="-DQDOS_PAGING"
NB=${NO_BIGK:-0}
vm() {  # vm DIR IMG EXTRA_FLAGS
  make i386vm I386VM="$I386VM" I386D=build/r11k/$1 I386FLAGS="$R9 -DQDOS_I386VM_BLOCK $PG $3" \
    I386IMG=$2 I386WIN= IMGENV="QDOS_LAYOUT=r9" I386EXTRA="kernel/paging.c kernel/paging.s" >/dev/null 2>&1
  echo "built build/r11k/$1/$2 $(sha256sum build/r11k/$1/$2 | cut -c1-16)"
}
pc() {  # pc DIR IMG EXTRA_FLAGS   (paging, R11K boot sector)
  make pc I386VM="$I386VM" PCD=build/r11k/$1 PCFLAGS="-DQDOS_LAYOUT_R9 -DR7_SHELL_MODE $PG $3" \
    PCIMG=$2 PCBOOT=build/pc-r9/boot2.bin PCMKIMG=platform/pc/make-image2.py \
    PCEXTRA="build/r11k/$1/paging.o build/r11k/$1/paging_s.o" >/dev/null 2>&1
  echo "built build/r11k/$1/$2 $(sha256sum build/r11k/$1/$2 | cut -c1-16)  kernel.raw $(stat -c %s build/r11k/$1/kernel.raw) B"
}
pcd() { # pcd DIR IMG EXTRA_FLAGS   (non-paging default kernel, R11K boot sector)
  make pc I386VM="$I386VM" PCD=build/r11k/$1 PCFLAGS="-DQDOS_LAYOUT_R9 -DR7_SHELL_MODE $3" \
    PCIMG=$2 PCBOOT=build/pc-r9/boot2.bin PCMKIMG=platform/pc/make-image2.py >/dev/null 2>&1
  echo "built build/r11k/$1/$2 $(sha256sum build/r11k/$1/$2 | cut -c1-16)  kernel.raw $(stat -c %s build/r11k/$1/kernel.raw) B"
}
make build/pc-r9/boot.bin build/pc-r9/boot2.bin I386VM="$I386VM" >/dev/null 2>&1 || make build/pc-r9/boot2.bin I386VM="$I386VM" >/dev/null
vm vm-paging qdos386-r11k-paging.bin ""
pc pc-paging r11k-paging.img ""
pc pc-paging-vga r11k-paging-vga.img "-DQDOS_CON_VGA"
pcd pc-default r11k-default.img ""
pcd pc-default-vga r11k-default-vga.img "-DQDOS_CON_VGA"
vm test-vm-flaunch qdos386-r11k-test-flaunch.bin "-DQDOS_TEST_FAULT_LAUNCH=3"
pc test-pc-flaunch r11k-test-flaunch.img "-DQDOS_TEST_FAULT_LAUNCH=3"
vm test-vm-cap qdos386-r11k-test-cap.bin "-DQDOS_TEST_PG_POOL_BYTES=0x200000"
vm test-vm-intcr3 qdos386-r11k-test-intcr3.bin "-DQDOS_TEST_PG_INT_AROUND_CR3"
pc test-pc-intcr3 r11k-test-intcr3.img "-DQDOS_TEST_PG_INT_AROUND_CR3"
vm test-vm-share qdos386-r11k-test-share.bin "-DQDOS_TEST_PG_SHARE_FRAMES"
vm test-vm-tasks qdos386-r11k-test-tasks.bin "-DQDOS_TEST_PG_TASKS"
pc test-pc-tasks r11k-test-tasks.img "-DQDOS_TEST_PG_TASKS"
pc test-pc-tasks-vga r11k-test-tasks-vga.img "-DQDOS_CON_VGA -DQDOS_TEST_PG_TASKS"
