# QDOS/386 R11C package

Source commit: 8b55cbfbc3aac69240d1388a999db32575225e40 (branch r11c, from R11B acceptance and the R11C Phase 0 audit 4907fee).

Contents: the source tree (git archive of the commit), the built SDK libraries (libc, libm,
libqdos, libgcc-qd) and example applications, disk images, the i386VM shell image,
ready-to-run directories under run/, logs under logs/, and SHA256SUMS for every file except
SHA256SUMS itself.

Run (interactive VGA, Tiny386): cd run/interactive && tiny386_headless interactive.ini
Run (diagnostic, Tiny386): cd run/diagnostic && tiny386_headless diagnostic.ini
Graphical QEMU configurations: launch/launch-qemu-vga.sh and launch/launch-qemu-diagnostic.sh

Rebuild the data volume from source: QDOS386_TOOLS=<GCC 1.40 tree> sh scripts/r11c-volume.sh <LGTEST4.aout>
Expected win1-r11c.img sha256: 0771f1c786bfd467ec5e5203b0bcfb3a7ce877acbb1617f3a38780b2a25c988d

Runtime matrix: QDOS386_TOOLS=... TINY386=... FW=... I386VM=... I386VM_HARNESS=... LGTEST=... sh scripts/r11c-run-all.sh
QEMU smoke: DISPLAY=:99 FW=... sh scripts/r11c-qemu-smoke.sh OUTDIR

Tests are SCRIPTED (keystrokes or piped input). No run in this package is a human session.
The results are in docs/R11C-RUNTIME-REPORT.md. The R2-R10 full regression matrices were not
rerun for R11C (see that report).
