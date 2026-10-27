# QDOS/386 R11F package

Source commit: d614fa69d070fba9acd12fca3965e8e96a4e9a4e (branch r11f, from the frozen R11E commit 7a62af1).
Contents: source tree (git archive of the commit), built SDK libraries and example
applications, PHOON.aout, disk images, run/ directories, SHA256SUMS.
FAT16 volume (images/win1-r11f.img) sha256: d319f3214efea391bc9c85d9048dac0bc5e77c42eeaf2d492fb9a1e820564375

QEMU VGA (scripted smoke test, scripts/r11f-qemu-smoke.sh; the kernel is images/r9shell-vga.img,
the volume images/win1-r11f.img):
  qemu-system-i386 -machine pc -cpu pentium -m 16 -bios <bios.bin> -vga std -display gtk \
    -drive file=hda.img,format=raw,if=ide,index=0 -drive file=hdb.img,format=raw,if=ide,index=1 -boot order=c

Clock modes (see docs/R11F-FIXED-CLOCK.md):
  run/none  no clock (regression)      run/pin  pinned at 2026-10-27 12:00:00 UTC
  run/adv   advancing from 2000-01-01

Run (Tiny386, pinned clock): cd run/pin && tiny386_headless run.ini
Then at the shell prompt:   RUN PHOON.AOU        RUN PHOON.AOU "11 Jan 2024 11:57:00"

Evidence and limits: docs/R11F-RUNTIME-REPORT.md, docs/R11F-KNOWN-LIMITATIONS.md,
docs/R11F-TIME-ABI.md, docs/R11F-PHOON-PORT.md, docs/R11F-C89-COVERAGE.md.
All runs were scripted.
