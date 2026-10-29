# qdos386-R13-VIRTUAL-DISPLAYS

Base: frozen R12C `7abb678b24764541845ef936adc45e9110f1a52b` (archive `96e7f6fc...`). Branch `r13-virtual-displays`.
Final source commit: `c3683d58e02817570f66919aead2451dd13e51a6`. The SHA-256 of this archive is printed by the packaging script and reported with
the delivery (it cannot be inside the archive); `SHA256SUMS` lists every other file.

Read `docs/R13-RUNTIME-REPORT.md` first, then `docs/R13-KNOWN-LIMITATIONS.md`, `docs/R13-DISPLAY-ABI.md`,
`docs/R13-FONT-PROVENANCE.md`, `docs/R13-MEMORY-BUDGET.md`, `docs/R13-AUDIT.md`.

**Everything still runs at CPL 0: nothing here is security isolation.**

Contents
- source tree (git archive of `c3683d58e02817570f66919aead2451dd13e51a6`): kernel (`kernel/surface.c`, `kernel/gsurf.c`, `kernel/font.c`,
  `kernel/font_minerva.c` (generated), `platform/pc/vgapres.c` are new; sessions in `kernel/vcon.c`), shell
  (`apps/shell`, -DR13_SHELL), SDK (`sdk/lib/qdossurf.c`, `sdk/examples/r13`), font extractor
  (`tools/r13-font`), `third_party/minerva` (the two Minerva 1.98 font modules, GPL-2.0, and COPYING), tests
  (`tests/r13`), docs, `fs/r13-system/ETC/STARTUP.CMD`
- `build/r13/pc-vga/r13-vga.img` (QEMU kernel, Minerva font), `build/r13/pc/r13.img` (headless/Tiny386 kernel),
  `build/r13/pc-vga-q5/r13-vga-q5.img` (QEMU kernel with the QDOS5x9 fallback font, no GPL material),
  `build/apps/shell-d.aout`; the unchanged R12C, R12B, R12A and R12 kernels and shells; firmware
- `guest-programs/`: the R13 builds (SURFT, COLOUR, BOUNCE, TITLET, BASIC, EDIT, ...); `guest-programs-r12b/` and
  `guest-programs-r12a/`: the frozen binaries used by the regression runs
- `volumes/r13-system.img`: the example system volume (R12B layout, R13 shell, SURFT/COLOUR/BOUNCE in /BIN)
- `emulator/`: Tiny386 with the R11M-FIX1 and R12C patches (unchanged from R12C)
- `logs/r13/`: per-stage transcripts, judge output, QEMU screendumps (PNG), Tiny386 evidence, image checks

Launch: `sh scripts/r12b-launch-qemu.sh volumes/r13-system.img build/r13/pc-vga/r13-vga.img`, then e.g.
`START COLOUR /NEW`, `START BOUNCE /NEW` and Ctrl+Esc. Gates: `sh tests/r13/run-host.sh`, `run-s1.sh OUT` ...
`run-s5.sh`, `run-tiny.sh`, `run-regressions.sh`.

The default R13 kernels contain the Minerva 1.98 font data (GPL-2.0): see docs/R13-FONT-PROVENANCE.md before
distributing them; `r13-vga-q5.img` carries none. SuperBASIC (`apps/superbasic`, `BASIC.AOU`, `BASTEST.AOU`) is
private research code and not freely redistributable; `apps/superbasic/docs/PROVENANCE-LICENSING.md` and
`SHA256SUMS.sb4` apply. The example system volume contains `/BIN/BASIC.AOU`: do not publish it as a public image
without removing it.
