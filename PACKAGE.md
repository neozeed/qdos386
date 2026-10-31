# qdos386-R13A-GRAPHICS-USABILITY

Base: frozen R13 `c3683d58e02817570f66919aead2451dd13e51a6` (archive `bac427c6...`). Branch
`r13a-graphics-usability`. Final source commit: `951352c73d20f4be2f593c974fbb47e6061154aa`. The SHA-256 of this archive is reported with the delivery
(it cannot be inside the archive); `SHA256SUMS` lists every other file.

Read `docs/R13A-RUNTIME-REPORT.md` first, then `docs/R13A-KNOWN-LIMITATIONS.md`, `docs/R13A-GRAPHICS-ABI.md`,
`docs/R13A-FONT-FORMAT.md`, `docs/R13A-FONT-PROVENANCE.md`, `docs/R13A-AUDIT.md`.

**Everything still runs at CPL 0: nothing here is security isolation.**

Contents
- source tree (git archive of `951352c73d20f4be2f593c974fbb47e6061154aa`): font registry `kernel/fontreg.c`, generated built-in fonts
  `kernel/font_recovery.c` / `kernel/font_console.c` (public-domain / unrestricted X11 fonts, sources and notice in
  `fonts/src`), key events `kernel/kev.c`, R13A presenter `platform/pc/vgapres13a.c`, surface/session changes
  under `-DQDOS_R13A`; shell `apps/shell/shell_r13a.inc` (-DR13A_SHELL); SDK `sdk/lib/qdosgfx.c`,
  `sdk/include/qdos386.h`, `sdk/examples/r13a/gfxa`; font tools `tools/r13a-font` (QFNT writer/parser, PCF and
  Minerva converters, broken-file generator, Minerva scanner); `fonts/` (RECOVERY.FNT, CON8X16.FNT);
  `third_party/minerva/` (the GPL-2.0 font modules, COPYING and the optional **MINERVA.FNT**); tests `tests/r13a`;
  `logs/r13a` (transcripts, judge output, screendumps as PNG, Tiny386 evidence, image checks)
- `build/r13a/pc-vga/r13a-vga.img` (QEMU kernel), `build/r13a/pc/r13a.img` (headless/Tiny386 kernel) - both the
  fallback-only build: no Minerva bytes; `build/apps/shell-e.aout` (R13A shell); the frozen R13 kernels and
  shell-d and the R12C kernels and shell-c, unchanged; firmware
- `guest-programs/`: GFXA and the programs the tests use (R13A tree builds)
- `volumes/r13a-system.img` (R13A shell, /SYSTEM/FONTS with RECOVERY, CON8X16 and the optional MINERVA.FNT + notice)
  and `volumes/r13a-system-nominerva.img` (the same without Minerva)
- `emulator/`: Tiny386 (unchanged from R12C)

Launch: `sh scripts/r12b-launch-qemu.sh volumes/r13a-system.img build/r13a/pc-vga/r13a-vga.img`, then e.g.
`CONSOLE GFX /ALL`, `FONT /SYSTEM/FONTS/MINERVA.FNT /CON`, `START GFXA pattern 0 /NEW`, `START COLOUR /NEW`,
Ctrl+Esc. Gates: `sh tests/r13a/run-host.sh`, `sh tests/r13a/run-gate.sh 1..4 OUT`, `run-demo.sh`, `run-tiny.sh`,
`run-regressions.sh`.

MINERVA.FNT is Minerva 1.98's font data (GPL-2.0, see third_party/minerva/COPYING and docs/R13A-FONT-PROVENANCE.md);
it is optional and the R13A kernels do not contain it. SuperBASIC (`apps/superbasic`, `BASIC.AOU`) is private
research code and not freely redistributable; `apps/superbasic/docs/PROVENANCE-LICENSING.md` applies. Both example
volumes contain `/BIN/BASIC.AOU`: do not publish them as public images without removing it.
