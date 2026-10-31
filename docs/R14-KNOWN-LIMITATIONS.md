# R14 known limitations

* **No security isolation.** Everything still runs at CPL 0. Serial port ownership (one open per port) and the
  separate sessions are correctness properties, not protection.
* **Serial hardware.** COM1 and COM2 only (0x3F8/IRQ4, 0x2F8/IRQ3); COM3/COM4, DMA, RS-485 and USB serial are not
  supported. Tested only on QEMU 8.2.2's emulated 16550A; **real UARTs: NOT TESTED**. QEMU does not pace the UART to
  the configured baud rate, so line timing, divisors other than the 115200 one in use, and overruns are not exercised
  by the runs (the driver's handling of them is covered by the host unit test against a simulated UART only).
  Parity and framing errors cannot be produced on QEMU (counters implemented, NOT TESTED in an emulator run).
* **Modem lines.** With QEMU socket/PTY back ends MSR is fixed (CTS, DSR, DCD on): a disconnect is not visible as a
  DCD drop, and the H (RTS/CTS) handshake's CTS gating is tested only in the host unit test. A serial channel never
  reports end of file.
* **Buffers.** 4 KiB receive / 2 KiB transmit ring per port; when the receive ring is full further bytes are dropped
  and counted (`rx_dropped`), with H the RTS line is lowered first. Closing a port discards untransmitted bytes
  (counted); programs drain first (`qd_ser_drain`).
* **`Z` and `C` channel suffixes** (^Z end of file, CR translation) are refused (−15): not implemented.
* **Tiny386.** Its 8250 model has no loopback, scratch register, FIFO, error bits or interrupt identification and
  shares the emulator's stdin/stdout with the harness's keyboard and screen; the R14 smoke runs with serial disabled
  (ports absent). Serial data transfer on Tiny386: **NA**.
* **TERM.AOU** has no terminal emulation (no escape-sequence interpreter). **XFER.AOU** is a test probe (QXF1), not a
  file-transfer protocol for users and not XMODEM/YMODEM.
* **QL compatibility layer** (`sdk/qlcompat`): source level only, sized to QLTerm. Not the QL binary ABI: no 68000 code,
  QDOS traps, QL memory map or system variables (`mt_inf` returns a private block with three fields). QL windows are
  emulated on 80 × 25 cells (borders not drawn, pixel sizes rounded to cells, MODE 4 colours only, underline shown as
  bright ink, the console font instead of program-supplied fonts). `mt_cjob` children are cooperative co-jobs inside
  the process: they switch only in the layer's waiting calls, priorities other than 0 are not graded, and a child that
  loops without a QDOS call would starve the others. `fs_heads`, `fs_date`, `mt_ipcom`, mode setting: `ERR_NI`.
  QDOS directory separators are `/` (not `_`); QL devices other than `CON_`, `SCR_`, `SER`, `WIN1_` are not found.
* **QLTerm** (see the parity table in `docs/R14-QLTERM-PORT-MAP.md`): no Telnet / raw TCP (QDOS/386 has no TCP/IP:
  `TCP_` is not found); no QDOS executable headers in transfers (a download carrying one stops with "not implemented");
  YMODEM dates are not applied; no BEL sound and no Hermes 12H75; no EX/EW inherited channels. Not tested: SER2 and the
  H ports from QLTerm, echo On/Local, ASCII keys, raw logging, XON/XOFF, multi-file YMODEM batches, `_dup` names,
  remote cancel, YMODEM-G upload, XMODEM checksum download, other baud rates, the B command. The test build is
  configured for SER1I, 115K2 and `WIN1_HOME/` (the original defaults - Telnet, 2400, `RAM2_` - do not exist here).
* **lrzsz** is the independent X/YMODEM implementation of the tests (host tool, not shipped). After QLTerm cancels a
  transfer lrzsz's sender may keep retrying (for example while repeating EOT); the test peer ends it 5 s after the
  guest's CAN CAN and logs that it did.
* **Package:** SuperBASIC (`/BIN/BASIC.AOU` on the example volume) is private research code: do not publish the volume
  publicly. QLTerm C R1 and the port are GPL-3.0-or-later (`docs/R14-LICENSE-PROVENANCE.md`).
* **Not done (out of scope by the handoff):** Elite, GW-BASIC, DOS V86, SMP, DOOM, a graphics redesign, TCP/IP.
