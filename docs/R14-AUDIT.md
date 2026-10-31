# R14 Stage 0 audit — baseline, interfaces, UART hardware

Everything below was read from the R13B source or measured; nothing is assumed. Logs are in `logs/r14/stage0/`.

## 1. Frozen baseline

* Branch `r14-serial-qlterm`, created from R13B commit `d9721897b4262a96a27e422e74c89cb5b428f06e`.
* R13B archive `qdos386-R13B-SHELL-USABILITY.tar.gz`, SHA-256
  `c13171d2c6894d36eab8b95d520de28a5b88ec448810f715a19353e872289263` (re-hashed).
* `logs/r14/stage0/baseline-hashes.txt`: SHA-256 of every frozen kernel and shell image (R12 … R13B, the R13B epoch
  kernels, shells A…F, `SH.AOU`). These are re-checked at the end of every stage and in the package; R14 code is
  compiled only under `QDOS_R14` (and new files), so frozen builds must stay byte-identical.
* `logs/r14/stage0/qemu-baseline.txt`: a fresh R13B system volume on QEMU (`tests/r13b/seq-fresh-smoke.txt`):
  CMOS clock, DATE, `DIR /W /BIN`, PHOON, `START SH /NEW`, focus, `EXIT`, `TASKS /L`, `LEAK`; final image
  `IMAGE PASS` (both FAT copies equal, `fsck.fat` clean, strict VFAT walker).

## 2. Kernel interfaces R14 builds on

| Area | What exists (file) | Relevance |
| --- | --- | --- |
| Channels | `kernel/channel.h`: 16 channels (`CHAN_MAX`), `struct chan_driver {name, prefix, open, close, write, read, seek, ctl, fsop}`, drivers found by name prefix; writes ≤ 256 bytes per trap (`CHAN_MAX_WRITE`); errors `CHAN_ERR_AGAIN` −19, `NAME` −7, `BADID` −12, `OWNER` −13, `RANGE` −14, `NOTSUP` −15 | `SER1`/`SER2` become one more driver with prefix `SER`; a read with nothing buffered returns `CHAN_ERR_AGAIN` (the existing non-blocking convention) |
| Traps | `kernel/jobs.c` trap switch; highest used number `QT_SURFACE` 0x3B | `QT_SER` = **0x3C** is free |
| Sleep / wake | `jobs[slot].status`: 0 runnable, *n* > 0 ticks to sleep, −1 indefinitely; `hold` flag byte; `job_wake(slot)` (R12C) and `vcon_wake_job(slot)`; a waiting trap sets `status` and returns `schedule(sp,1,0)` (`QT_SUSPEND`, `QT_CON_WAIT`) | the serial wait uses the same model; traps run with IF clear, so a waiter registered inside the trap cannot miss an interrupt that arrives before it sleeps |
| Console wait | one waiter per console (`vcon_waiter`/`vcon_set_waiter`); input is queued only for the foreground console | a terminal must sleep on *serial or keyboard*: the serial wait can also register as its console's waiter |
| IRQ entry | `kernel/kctx.s` `irq1_stub` pattern (pushal, switch to the kernel stack when outside `[kx_lo,kx_hi)`, call C, restore, `iret`); `set_gate()` in `kernel/descriptors.c` installs 0x20 (IRQ0), 0x21 (IRQ1), 0x27, 0x2F | IRQ3 → vector 0x23, IRQ4 → vector 0x24 are unused; R14 adds its stubs in a **new** `.s` file and its `set_gate` calls under `#ifdef QDOS_R14` |
| PIC | `platform/pc/platform.c` `pic_init`: master mask `0xFC` (IRQ0 timer, IRQ1 keyboard only) | the driver unmasks IRQ3/IRQ4 at run time (read-modify-write of port 0x21) only for a UART that passed the probe |
| COM1 use today | `platform.c` `diag_putc` writes port 0x402 (QEMU debugcon); it writes 0x3F8 only under `QDOS_CON_SERIAL`, which no R13B build defines | COM1/COM2 are unused by every frozen build |
| Ownership | channels are owned by the opening job (`CHAN_ERR_OWNER`); an exiting or killed job's channels are closed by the reclaim pass (`channel.h`: "Job exit: close every channel owned by 'owner'") | serial ownership follows the channel: one open per port |
| Memory | programs allocate from their own heap (`qd_alloc`); the kernel's state is static tables; all programs run at CPL 0 | ring buffers are static kernel arrays (no allocation in the IRQ path); CPL 0 means ownership is cooperative, not isolation |

Console/display: R13 private sessions (`START … /NEW`), the R13A presenter (VGA text and 640×480×16 graphical text),
the R13B Task Manager (Ctrl+Esc, F2) and focus return. A program's title comes from `_prog_name`/`QT_JOB_TITLE`.

## 3. UART audit

PC convention: COM1 0x3F8 IRQ4, COM2 0x2F8 IRQ3 (COM3 0x3E8, COM4 0x2E8 share IRQ4/IRQ3 and are not used).

### 3.1 QEMU 8.2.2 (measured)

`tests/r14/uartprb.c` (`UARTPRB.AOU`) probes each base with bounded register tests, interrupts disabled in the UART
and the PIC untouched: scratch register write/readback, IER write/readback, FIFO enable (IIR bits 7..6), and an
internal-loopback byte (MCR bit 4, THR 0xA5 → RBR, ≤ 100000 polls). Runs (logs `qemu-none.txt`, `qemu-dflt.txt`,
`qemu-two.txt`):

| QEMU options | COM1 0x3F8 | COM2 0x2F8 | COM3/COM4 |
| --- | --- | --- | --- |
| `-serial none` | all registers 0xFF → ABSENT | ABSENT | ABSENT |
| none given (the R12/R13 capture harness) | PRESENT, **16550A** (FIFO IIR 0xC1), scratch 55/AA, MSR 0xB0, loopback A5 | ABSENT | ABSENT |
| `-serial null -serial null` | PRESENT, 16550A | PRESENT, 16550A | ABSENT |

So a register probe (an absent ISA port floats to 0xFF; IER readback 0x0F; loopback byte) reliably distinguishes
present from absent ports, in bounded time. MSR reads 0xB0 (CTS, DSR, DCD set) on socket/null back ends: QEMU only
reports real modem lines for host tty back ends, so **disconnect is not visible as a DCD drop** on these back ends.
QEMU's chardev only delivers a byte when the UART can accept it, so receive **overrun cannot be produced** on QEMU;
by QEMU's character-device source, a `telnet=on` socket back end turns a received `IAC BREAK` into a UART break
(LSR BI) — the one line error a host peer could inject (measured in Stage 1, not assumed). Parity and framing errors are not emulated.

### 3.2 Tiny386 (source audit, `t386/r12c/misc.c`, `pc.c`)

* One 8250-like model at 0x3F8, IRQ4, enabled only by `enable_serial=1` in the ini; otherwise reads of 0x3F8–0x3FF
  return 0xFF (absent). 0x2F8/0x3E8/0x2E8 read **0** (not 0xFF) and ignore writes.
* No scratch register, no FIFO, no loopback (MCR bit 4 is stored but ignored), no divisor effect, LSR always
  `0x60 | DR` (no error bits), MSR fixed 0xB0, IIR only 0 or 1 (no interrupt identification). The THRE interrupt
  source (`ioready` bit 1) is set whenever IER bit 1 is set and never cleared by a THR write.
* TX writes to the emulator's **stdout**, RX reads the emulator's **stdin** (`IsKBHit`/`ReadKBByte`), which the
  `-term` front end used by the harness also reads for the keyboard and writes for the screen.
* Consequence: with `enable_serial=1` the probe's loopback test fails, and serial data cannot be separated from the
  harness keyboard/screen streams. The Tiny386 smoke therefore covers the **missing-port** path (default ini) plus an
  R14 kernel boot/regression; serial data transfer on Tiny386 is **NA** (evidence above). Tiny386 is not extended.

### 3.3 Driver consequences

* Probe at boot (no interrupts, bounded), register a port only if present; a missing port gives an explicit error at
  open time.
* Use the 16550A FIFO when IIR reports it (trigger level 8 bytes), plain 16450 operation otherwise.
* The interrupt handler drains IIR in a bounded loop (at most 16 identifications per interrupt), reads LSR errors
  (OE, PE, FE, BI) into counters, moves RX bytes into a ring, refills the transmitter from a TX ring and turns THRE
  interrupts off when the ring is empty. No blocking work, no allocation, one `job_wake` at most per port.
* PIC: unmask the port's IRQ only when a port is present; EOI to the master PIC.
