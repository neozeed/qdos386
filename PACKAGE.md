# qdos386-R14-SERIAL-QLTERM

Base: frozen R13B `d9721897b4262a96a27e422e74c89cb5b428f06e` (archive `c13171d2...`). Branch `r14-serial-qlterm`.
Final source commit: `43a1f4481dc080bdd4604f5a53694c78e7739aa0`. The SHA-256 of this archive is reported with the delivery (it cannot be inside the
archive); `SHA256SUMS` lists every other file.

Read `docs/R14-RUNTIME-REPORT.md` first, then `docs/R14-KNOWN-LIMITATIONS.md`, `docs/R14-SERIAL-ABI.md`,
`docs/R14-QL-COMPAT-ABI.md`, `docs/R14-QLTERM-PORT-MAP.md`, `docs/R14-AUDIT.md`, `docs/R14-LICENSE-PROVENANCE.md`.

**Everything still runs at CPL 0: nothing here is security isolation.**

Contents
- source tree (git archive of `43a1f4481dc080bdd4604f5a53694c78e7739aa0`): the UART driver `kernel/uart.c`, `kernel/uart_s.s` (`-DQDOS_R14` only);
  libqdos serial calls `sdk/lib/qdosser.c`; the QL compatibility layer `sdk/qlcompat`; QLTerm C R1 as supplied
  `third_party/qlterm-c-r1/orig` (GPL-3.0-or-later, unmodified) and the one-function patch
  `third_party/qlterm-c-r1/r14-qdos386.patch`; TERM and XFER `sdk/examples/r14`; host scripts
  `scripts/r14-serial-args.sh`, `scripts/r14-qemu-serial.sh`; build scripts `scripts/r14-build-*.sh`,
  `tools/r14-qlterm-config.py`; tests and host peers `tests/r14`; logs and screenshots `logs/r14`
- `build/r14/pc-vga/r14-vga.img` (QEMU), `build/r14/pc/r14.img` (headless/Tiny386); the frozen R13B and R13A
  kernels and shells, unchanged; firmware
- `build/r14/qlterm/QLTERM.aout` (QLTerm for QDOS/386, configured SER1I / 115K2 / WIN1_HOME/), `QLTERM-raw.aout`
  (the original configuration block), `source-changes.diff` (the whole source change)
- `guest-programs/`; `volumes/r14-system.img` (R13B system volume + QLTERM, TERM, XFER, SERTEST in /BIN)
- `emulator/`: Tiny386 (unchanged)

Launch with COM1 on the host: `sh scripts/r14-qemu-serial.sh volumes/r14-system.img tcp` (prints the port; then
e.g. `socat - TCP:127.0.0.1:PORT`) or `... pty`; in QDOS/386: `START QLTERM /NEW` then `CON 4` (or Ctrl+Esc),
F3 for QLTerm's menu; or `TERM SER1 115200`. Tests: `sh tests/r14/run-host.sh`, `run-gate1.sh` .. `run-gate5.sh`
(gates 4 and 5 need lrzsz on the host), `run-tiny.sh`, `run-regressions.sh`, `check-frozen.sh`.

Licences: QLTerm C R1 and the QDOS/386 port of it (patch, compatibility layer as linked into QLTerm, build and test
scripts) are GPL-3.0-or-later; the original notices are in `third_party/qlterm-c-r1/orig`. MINERVA.FNT (on the
volume, optional) is Minerva 1.98's font data (GPL-2.0). SuperBASIC (`apps/superbasic`, `BASIC.AOU`) is private
research code and not freely redistributable; `apps/superbasic/docs/PROVENANCE-LICENSING.md` applies. The example
volume contains `/BIN/BASIC.AOU`: do not publish it as a public image without removing it.
