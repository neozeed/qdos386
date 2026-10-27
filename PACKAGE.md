# QDOS/386 R11D package

Source commit: 2d409fbb3f8fe86b201e422a6bba496b5de8e14f (branch r11d, from the frozen R11C checkpoint 8b55cbf).
Contents: source tree (git archive of the commit), built SDK libraries and example
applications, disk images, i386VM shell image, run/ directories, logs/, SHA256SUMS.

Run (interactive VGA, Tiny386): cd run/interactive && tiny386_headless interactive.ini
Run (diagnostic, Tiny386): cd run/diagnostic && tiny386_headless diagnostic.ini

Rebuild the data volume: QDOS386_TOOLS=<GCC 1.40 tree> sh scripts/r11d-volume.sh <LGTEST4.aout>
Expected win1-r11d.img sha256 (with LGTEST4): see logs/r11d/volume.txt

Runtime matrix: QDOS386_TOOLS=... TINY386=... FW=... I386VM=... I386VM_HARNESS=... LGTEST=... sh scripts/r11d-run-all.sh
QEMU smoke: DISPLAY=:99 FW=... sh scripts/r11d-qemu-smoke.sh OUTDIR

Tests are SCRIPTED. Results: docs/R11D-RUNTIME-REPORT.md. Coverage: docs/R11D-C89-COVERAGE.md.
