# qdos386-R13B-SHELL-USABILITY

Base: frozen R13A `951352c73d20f4be2f593c974fbb47e6061154aa` (archive `6c966941...`). Branch `r13b-shell-usability`.
Final source commit: `d9721897b4262a96a27e422e74c89cb5b428f06e`. The SHA-256 of this archive is reported with the delivery (it cannot be inside the
archive); `SHA256SUMS` lists every other file.

Read `docs/R13B-RUNTIME-REPORT.md` first, then `docs/R13B-KNOWN-LIMITATIONS.md`, `docs/R13B-EXEC-RESOLUTION-ABI.md`,
`docs/R13B-TASK-LAUNCHER.md`, `docs/R13B-CLOCK-ABI.md`, `docs/R13B-AUDIT.md`.

**Everything still runs at CPL 0: nothing here is security isolation.**

Contents
- source tree (git archive of `d9721897b4262a96a27e422e74c89cb5b428f06e`): shell `apps/shell` (`shell_r13b.inc`: CLS, DATE, TIME, programs by name;
  boot shell -DR13B_SHELL at 0x200000, `SH.AOU` -DR13B_DYN at 0x240000); kernel `kernel/rtc.c` (CMOS RTC),
  `kernel/clock_boot.c`, `kernel/taskmgr.c` (F2), `kernel/imgload.c` (`img_launch_as`), `kernel/vcon.c` (focus
  return), all under `-DQDOS_R13B`; SDK examples `sdk/examples/r13b` (ARGV, TIMEQ); tests `tests/r13b` (gates,
  demo, Tiny386, regressions, host RTC vectors, Phoon host oracle); `logs/r13b`; docs
- `build/r13b/pc-vga/r13b-vga.img` (QEMU), `build/r13b/pc/r13b.img` (headless/Tiny386), `build/r13b/e2100|e2101|e2106`
  (build-time epoch test kernels), `build/apps/shell-f.aout` (boot shell), `build/apps/sh.aout` (SH.AOU); the frozen
  R13A kernels and shell-e, unchanged; firmware
- `guest-programs/`; `volumes/r13b-system.img` (R13B boot shell, /BIN/SH.AOU, ARGV, TIMEQ, the R13A fonts)
- `emulator/`: Tiny386 (unchanged from R12C)

Launch: `sh scripts/r12b-launch-qemu.sh volumes/r13b-system.img build/r13b/pc-vga/r13b-vga.img` (QEMU's RTC gives the
date), then e.g. `DATE`, `PHOON`, `START SH /NEW`, `CLS`, Ctrl+Esc then F2. Gates: `sh tests/r13b/run-host.sh`,
`sh tests/r13b/run-gate.sh 1..4 OUT`, `run-demo.sh`, `run-tiny.sh`, `run-regressions.sh`.

MINERVA.FNT (on the volume, optional) is Minerva 1.98's font data (GPL-2.0, third_party/minerva). SuperBASIC
(`apps/superbasic`, `BASIC.AOU`) is private research code and not freely redistributable;
`apps/superbasic/docs/PROVENANCE-LICENSING.md` applies. The example volume contains `/BIN/BASIC.AOU`: do not publish
it as a public image without removing it.
