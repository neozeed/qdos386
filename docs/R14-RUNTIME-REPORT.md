# R14 runtime report — serial communications, QL compatibility, QLTerm C

Branch `r14-serial-qlterm` from frozen R13B `d9721897b4262a96a27e422e74c89cb5b428f06e`. QEMU 8.2.2 is the primary
platform; one bounded Tiny386 smoke. Results are recorded as PASS / FAIL / NA / NOT TESTED; all logs, transcripts,
peer logs and screenshots are in `logs/r14/`.

## What R14 adds

1. **UART driver and serial channels** (`kernel/uart.c`, `kernel/uart_s.s`, only in `-DQDOS_R14` kernels): bounded boot
   probe of COM1/COM2 (8250 / 16450 / 16550A / absent), interrupt-driven RX/TX rings (4 KiB / 2 KiB), FIFO, line-error
   and drop counters, RTS/CTS handshake (`H`), exclusive open, `SER1`/`SER2` channels with the QL suffix letters, and
   `QT_SER` (0x3C) for configuration, status and scheduler waits with deadlines, console-key wake and cancel.
   libqdos: `qd_ser_*` (`sdk/lib/qdosser.c`). ABI: `docs/R14-SERIAL-ABI.md`.
2. **Host connection and simple tools**: `scripts/r14-serial-args.sh` / `r14-qemu-serial.sh` (COM1 to a free TCP port or a
   host pseudo-terminal), `TERM.AOU` (minimal terminal), `XFER.AOU` (QXF1 test transfer probe — not XMODEM).
3. **QL source-compatibility layer** `sdk/qlcompat` — the QDOS/C68 calls QLTerm actually uses, on QDOS/386 services
   (`docs/R14-QL-COMPAT-ABI.md`); not the QL binary ABI.
4. **QLTerm C R1 for QDOS/386**: the supplied sources, unmodified except one function (`try_lock`: 68000 `TAS` → i386
   `XCHG`), linked with the layer and configured in the executable (`docs/R14-QLTERM-PORT-MAP.md`).

## Results

| Stage / gate | Result | Log |
| --- | --- | --- |
| 0 baseline: R13B archive and commit, 25 frozen images hashed, R13B QEMU smoke | PASS (image clean) | `logs/r14/stage0/` |
| 0 UART measurement (`UARTPRB`): QEMU COM1/COM2 16550A when configured, 0xFF when absent | measured | `logs/r14/stage0/qemu-*.txt` |
| 0 QLTerm compile inventory: no `qdos.h` 0/7; declarations only 6/7 (TAS) | measured | `logs/r14/stage0/qlterm-compile-inventory.txt` |
| 1 host unit test of `kernel/uart.c` on a simulated 16550A/8250 | 10418/10418 checks | `logs/r14/gate1/host-uart.txt` |
| 1 Gate 1 (QEMU, host peer): binary echo incl. 00/0A/1A/FF, 64 KiB bursts both ways, ring-full drop counting, timeout, cancel, key wake, break injection, close/reopen, background session + CPU load, no-port machine | **42/42 PASS** | `logs/r14/gate1/` |
| 1 frozen R12…R13B kernels and shells rebuilt from the R14 source | 25/25 byte-identical | `logs/r14/stage1-frozen.txt`, `logs/r14/final-frozen.txt` |
| 2 Gate 2: TERM background/foreground, QXF1 transfers of 100 B and 40000 B both ways (one corrupted frame), fresh boot over a host PTY | **31/31 PASS** | `logs/r14/gate2/` |
| 3 Gate 3: host C89 test of the layer (60/60), compile inventory with the layer (unmodified 6/7, patched 7/7), guest probe (co-jobs on real stacks, timeouts, windows, files, directory header, explicit ERR_NF/ERR_NI, serial), no-port run, R13B smoke on R14 | **29/29 PASS** | `logs/r14/gate3/` |
| 4 Gate 4: QLTerm against the host peer and lrzsz — terminal, keys, ANSI/VT52 display and colours, CPR, log while switching away and back through the Task Manager (300 lines exact), XMODEM CRC/checksum/1K and YMODEM/YMODEM-G both ways byte-exact, corrupted byte, lost block (timeout → retransmission), CTRL-X cancel, Transmit, Connect, Options, Interface (Telnet blocker shown), QDOS-header download refused, Quit | **37/37 PASS** | `logs/r14/gate4/` |
| 5 integrated: QLTerm, a second SH, SuperBASIC and BOUNCE in their own sessions at once; a BASIC file from the host through QLTerm, LOADed and RUN in SuperBASIC while QLTerm logs 600 lines in the background during a Task Manager tour; a binary file back; SYNC; memory back to baseline | **17/17 PASS** | `logs/r14/gate5/` (screenshots in `screenshots/`) |
| 5 regressions on the R14 kernel: R13B Gates 1–4, R13A Gates 1–4, R12B Stage 3 (SuperBASIC file I/O), R12A Stage 3 (processes) | see below | `logs/r14/regressions/` |
| Tiny386 smoke (R14 headless kernel; serial disabled in its ini): boot, ports absent, SERTEST/TERM/XFER fail with −7, SYNC, LEAK, guest shutdown | **8/8 PASS**; serial data **NA** (docs/R14-AUDIT.md 3.2) | `logs/r14/tiny/` |

Regressions on the R14 kernel (each suite's own unchanged script, volume and judge; `logs/r14/regressions/`):

| Suite | Result |
| --- | --- |
| R13B Gate 1 (dynamic shells) | 23/23 PASS |
| R13B Gate 2 (program lookup, PATH, START, SuperBASIC EXEC) | 33/33 PASS |
| R13B Gate 3 (Task Manager F2, CLS) | 33/33 PASS |
| R13B Gate 4 (CMOS clock, DATE/TIME, Phoon oracle) | 24/24 PASS |
| R13A Gate 1 (fonts) | 44/44 PASS |
| R13A Gate 2 (rendered text consoles) | 26/26 PASS |
| R13A Gate 3 (logical surfaces) | 22/22 PASS |
| R13A Gate 4 (native key events) | first run 16/17 FAIL; rerun 17/17 PASS (see note) |
| R12B Stage 3 (SuperBASIC LOAD/SAVE/MERGE file I/O) | 39/39 PASS |
| R12A Stage 3 (dynamic processes: spawn, wait, kill) | 35/35 PASS |

Note on R13A Gate 4: in the first run the release of `q` was logged after its press (`... D10 U10`), while the
check requires the press to be the last event GFXA logs before it stops; GFXA polls every 2 ticks and QEMU releases a
`sendkey` after 100 ms, so the release lands in the same poll only when the guest is not scheduled for that long. That
run shared the host with two other emulator runs; run alone it passed 17/17 with the same kernel. Both logs are kept
(`r13a-g4.log`, `r13a-g4-rerun.log`). The R14 kernel changes nothing in the key path; the only R14 work in the timer
interrupt is `ser_tick` (two ports, returns at once when no job waits).

QLTerm feature parity (PASS / PARTIAL / NOT TESTED / NOT SUPPORTED, with the blocker for each): the table in
`docs/R14-QLTERM-PORT-MAP.md` §5.

## Measurements worth knowing

* QEMU's 16550A is not paced to the baud rate: 64 KiB moved in 0.5–0.8 s at "115200" (≈ 85–138 KB/s).
* Receive ring full: exactly 4095 bytes kept in order, the rest counted in `rx_dropped`.
* Timeouts: 50 ticks → 51; a cancel from another job after 100 ticks → returned after 104; QL frames → ticks ×2
  (QLTerm's 50-frame keyboard timeout measured 100 ticks).
* QLTerm transfers against lrzsz: 2–8 KB/s (stop-and-wait with 10 ms co-job polling granularity); YMODEM-G streams.

## How to reproduce

    . env (QDOS386_TOOLS=...); make r14-pc; make -C sdk/lib; sh scripts/r14-build-guest.sh
    sh tests/r14/run-host.sh OUT
    sh tests/r14/run-gate1.sh OUT1   ...   sh tests/r14/run-gate5.sh OUT5      (gates 4/5: lrzsz on the host)
    TINY386=<tiny386_headless> sh tests/r14/run-tiny.sh OUTT
    sh tests/r14/run-regressions.sh OUTR; sh tests/r14/check-frozen.sh LOG
