# R14 serial ABI

Status: implemented in Stage 1 (`kernel/uart.c`, `kernel/uart_s.s`, `sdk/lib/qdosser.c`), Gate 1 42/42 on QEMU
(`logs/r14/gate1/`), host unit test of the driver 10418/10418 checks (`tests/r14/host-uart.c`).

Kernels: built with `-DQDOS_R14` (new `r14` targets). Frozen R13B and older kernels contain none of this.

## 1. Ports

| Port | Base | IRQ / vector | Probed at boot |
| --- | --- | --- | --- |
| 1 (`SER1`) | 0x3F8 | IRQ4 / 0x24 | yes |
| 2 (`SER2`) | 0x2F8 | IRQ3 / 0x23 | yes |

The boot probe (no interrupts, bounded: register readback and one internal-loopback byte, as `UARTPRB`) classifies a
port as absent, 8250, 16450 or 16550A and prints one `SERIAL:` line per port. Only a present port gets its IRQ
unmasked. COM3/COM4 are not supported.

## 2. Channel names

`SER<n>[<parity>][<handshake>][<protocol>]`, case-insensitive, n = 1 or 2, following the QL `SER` device letters:

| Letter | Meaning | R14 |
| --- | --- | --- |
| parity `O` `E` `M` `S` | odd, even, mark, space (default none) | supported (7 data bits with parity, as QDOS) |
| handshake `H` | hardware handshake | RTS raised while the receive ring has room; dropped at 3/4 full, raised again at 1/4; transmission waits while CTS is low |
| handshake `I` | ignore handshake (default) | supported |
| protocol `R` | raw (default) | supported: no translation ever |
| protocol `Z`, `C` | ^Z end-of-file / CR translation | **refused** (−15): not implemented |

Anything else after the port number is a bad name (−7). An absent port gives −7 (not found).

## 3. Open, ownership, close

* Opening sets the port to 9600 baud, 8 data bits (7 with parity), 1 stop bit, DTR and RTS on, FIFO on (16550A,
  trigger 8), enables receive and line-status interrupts, and empties both rings.
* **One open per port.** A second open of the same port by any job fails with −9 (in use). The owner is the opening
  job; other jobs' operations on the channel fail with the kernel's existing owner error (−13). All code runs at CPL 0,
  so this is cooperative ownership, not protection.
* Close (or the exit/kill of the owner) disables the UART interrupts, drops DTR/RTS, discards untransmitted bytes
  (counted in `tx_discarded`), wakes any job waiting on the port, and leaves the port free for the next open.
  Programs that need all bytes delivered drain first (`qd_ser_drain`).

## 4. Data path and buffers

* Receive ring 4096 bytes, transmit ring 2048 bytes per port, static kernel memory.
* `qd_read` on a SER channel never blocks: it returns 1…n bytes, or −19 (`CHAN_ERR_AGAIN`) when nothing is buffered.
  A serial line has no end of file: a read never returns 0 for "end".
* `qd_write` copies as many bytes as the transmit ring holds and starts the transmitter; it returns the count
  (possibly short), or −19 when the ring is full.
* Receive ring full: further bytes are **dropped and counted** (`rx_dropped`); with `H`, RTS is lowered before that.
* Line errors: overrun (`overrun`, bytes lost in the UART), parity and framing (`parity`, `framing`: the byte is kept),
  break (`breaks`: the NUL the UART delivers with a break is not stored).

## 5. Trap `QT_SER` (0x3C) and libqdos calls

| libqdos | Meaning |
| --- | --- |
| `qd_ser_probe(port)` | 0 absent, 1 8250, 2 16450, 3 16550A |
| `qd_ser_config(ch, cfg)` / `qd_ser_getconfig(ch, cfg)` | `struct qd_ser_cfg { baud, data_bits (5..8), parity ('N','O','E','M','S'), stop_bits (1, 2), handshake ('I','H') }`; baud must divide 115200 exactly (divisor = 115200 / baud); other values −15 |
| `qd_ser_status(ch, st)` | counters and state: `rx_bytes tx_bytes rx_queued tx_queued overrun parity framing breaks rx_dropped tx_discarded msr lsr type` |
| `qd_ser_wait(ch, flags, ticks)` | sleep until a condition holds (`QD_SER_RX` data, `QD_SER_TXSPACE`, `QD_SER_TXEMPTY`, `QD_SER_KEY` input queued on the caller's console), the timeout (ticks, `QD_SER_FOREVER`), a cancel, or a close. Returns the ready mask (> 0), −1 timeout (QDOS "not complete"), −51 cancelled |
| `qd_ser_read_timeout(ch, buf, n, ticks)` | ≥ 1 bytes, −1 timeout, −51 cancelled; never spins |
| `qd_ser_write_all(ch, buf, n, ticks)` | n, or −1 / −51 with the count written so far available through `qd_ser_status` |
| `qd_ser_drain(ch, ticks)` | until the transmitter is empty |
| `qd_ser_cancel(port)` | any job: ends the current wait on that port with −51 (one pending cancel is remembered) |
| `qd_ser_flush(ch, what)` | discard RX and/or TX ring contents |

Waiting is done in the kernel scheduler; the trap registers the waiter with interrupts off, so a byte that arrives
between the check and the sleep cannot be lost.

### 5.1 Kernel call (for other language runtimes)

`QT_SER` = 0x3C, EBX = operation, ECX = b, EDX = c (`kernel/uart.h`):

| Op | b | c | Result |
| --- | --- | --- | --- |
| 0 PROBE | port 1/2 | – | type 0..3 |
| 1 CONFIG / 2 GETCFG | channel | `struct ser_cfg *` (8 bytes: `u32 baud; u8 data_bits, parity, stop_bits, handshake`) | 0 / −15 / −14 |
| 3 STATUS | channel | `struct ser_status *` (16 × u32, order as in `qdos386.h`) | 0 |
| 4 WAIT | channel | `flags << 24 \| ticks` (ticks 0xFFFFFF = forever, 0 = poll) | ready mask at once; 0 after a sleep; −1 nothing ready and ticks 0; −51 cancel pending |
| 5 POLL | channel | flags | ready mask, 0, or −51 (consumes the cancel) |
| 6 CANCEL | port 1/2 | – | 0 (any job; remembered until the owner's next WAIT/POLL) |
| 7 FLUSH | channel | 1 RX, 2 TX | 0 |
| 8 DEFAULT | port 1/2 | baud (0 = 9600) | 0 / −15: the rate used by the next open |

A sleeping WAIT sets the job's status to −1 (so a console key can also wake it through the console waiter) and keeps
its own deadline, checked by the timer interrupt; `jobs[].hold` points at a per-port byte that every wake sets, which
is how a registration made stale by another wake (a key, a kill) is recognised and dropped without a second wake.

### 5.2 Measured on QEMU 8.2.2 (Gate 1)

* 64 KiB host→guest unpaced at 115200: 77 ticks (≈ 85 KB/s), 0 dropped; 64 KiB guest→host: 46–64 ticks. QEMU does
  **not** pace the 16550A to the configured baud rate (the peer measured 96–138 KB/s at "115200"), so line timing is
  not tested by these runs.
* Receive ring full: 6000 bytes into an unread port → exactly 4095 kept, in order, and 1905 counted in `rx_dropped`.
* Timeout 50 ticks → −1 after 51 ticks; cancel from another job after 100 ticks → −51 after 104 ticks; a key typed on
  the waiting job's console ends a `QD_SER_RX | QD_SER_KEY` wait with mask `QD_SER_KEY`.
* Break: `IAC BREAK` on a `telnet=on` socket back end → `breaks` 1, nothing stored. Parity, framing and overrun
  counters are implemented from LSR but **cannot be produced on QEMU** (no parity/framing emulation; the back end
  never overruns the FIFO); the host unit test covers them against the simulated UART.
* `-serial none`: both ports reported absent at boot; probe 0; opens fail with −7.

## 6. Not provided

Modem-line change detection on QEMU socket back ends (MSR fixed), DCD-based disconnect, `Z`/`C` translation, COM3/4,
DMA, RS-485, any TCP/IP.

## 7. Host connection and Stage 2 programs

* `scripts/r14-serial-args.sh MODE` prints the QEMU options: `tcp` (a free port chosen and printed), `tcp:PORT`
  (≥ 1024), `pty` (QEMU names the pseudo-terminal on stderr: `char device redirected to /dev/pts/N (label r14com1)`;
  QEMU 8.2 ignores `path=` for pty), `tcp2:P1:P2` (COM2 in telnet mode), `none`. No fixed host path, no privileged
  port. `scripts/r14-qemu-serial.sh VOL [MODE]` boots a copy of a volume with that option for interactive use.
* `TERM.AOU` (`sdk/examples/r14/term`): `TERM [SER1|SER2] [baud]` — a minimal terminal on its own console session:
  rows 0–23 show received bytes (printable and code page 437 bytes; CR LF BS TAB obeyed; no escape-sequence
  emulation), row 24 a status line (port, settings, byte counts, drops, errors); keys typed on its console are sent;
  F10 ends it. It sleeps in `qd_ser_wait(QD_SER_RX | QD_SER_KEY)`; keys reach it only in the foreground.
* `XFER.AOU` (`sdk/examples/r14/xfer`): the **QXF1 test transfer probe — not XMODEM/YMODEM and not a QLTerm
  feature**: STX-framed 1 KiB blocks with a CRC-32 each, stop-and-wait ACK/NAK lines, an end frame with the file's
  length and CRC-32; `XFER GET name path`, `XFER PUT path name`, `XFER SUM path`. The host side is
  `tests/r14/serpeer.py`.
* Gate 2 (`tests/r14/run-gate2.sh`, `logs/r14/gate2/`): 31/31 — TERM in a background session (no text on the shell's
  screen, shell keys never sent, PING→PONG once in the foreground, 23 consecutive intact lines, 0 drops, F10 back to
  the shell); 100-byte control-byte and 40000-byte binary fixtures host→guest (one run with a corrupted frame NAKed
  and repeated) and guest→host, equal by SHA-256 on the final image and at the peer; a fresh boot of that image
  with COM1 on a host pseudo-terminal finds both files intact and sends one back byte-exact; all images clean.
