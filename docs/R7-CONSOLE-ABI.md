# QDOS/386 R7 — console input and RUN ABI

This document defines what R7 adds to the R6 contracts: the CON input channel,
the blocking wait, the RUN service, the directory channel, and the error codes.
R2–R6 services are unchanged except where noted.

## 1. Keyboard input (PC)

Source: `platform/pc/i8042.c`, `kernel/kbd.c`, `kernel/cin.c`.

- `input_init` (called before the scheduler starts) disables both i8042 ports,
  flushes the output buffer, writes the command byte with bit 0 (IRQ1 on),
  bit 1 clear (aux IRQ off), bit 4 clear (keyboard enabled), and bit 6 set
  (set-1 translation), then enables the keyboard port. The IRQ1 gate is
  vector 0x21 (`kernel/context.s`, `kernel/descriptors.c`). The PIC is
  programmed with OCW1 0xfc, so IRQ0 and IRQ1 are unmasked.
- `input_irq1` reads port 0x60 only when status bits 0 and 5 read `01`
  (output full, from the keyboard). Each byte goes through the set-1 decoder,
  then `cin_put`, then `con_input_arrived`, and the handler writes EOI to port
  0x20.
- Decoder (`kbd_feed`): make codes 0x00–0x39 map to plain and upper tables.
  Shift (0x2A, 0x36) is tracked, and releases (bit 7) are consumed. Enter 0x1C
  gives CR, Backspace 0x0E gives 0x08, Tab 0x0F gives HT. Other keys without a
  table entry produce no byte. The prefix 0xE0 marks an extended key: only the
  keypad Enter (E0 1C) produces CR, and other extended keys produce nothing.
  E1 (Pause) drops the five bytes that follow it.
- The ring is 128 bytes (127 usable). A byte that arrives when the ring is full
  is dropped and counted in `cin_overflows`. Stored bytes are never overwritten.
- Aux (mouse) output is drained by `input_tick` on each timer tick, counted in
  `kbd_aux_drained`. The drain is defensive: a stale aux byte in the shared
  output buffer hides the keyboard's IRQ1. The R7 keyboard probe (the runtime report, section on the probe)
  found that keys pressed before the controller is initialised are lost, not
  that aux output blocked the keyboard. The drain stays in the code.
- Counters: `kbd_scancodes`, `kbd_timeouts`, `kbd_flushed`, `kbd_aux_drained`.

On i386VM there is no PIC and no i8042. `input_irq1` and `input_tick` are empty.

## 2. Input polling (i386VM)

The i386VM harness is patched (`patches/i386vm-input-status.patch`, recorded
in `patches/HASHES.txt`). The patch adds one status register and leaves the
existing ports alone:

| address    | access | meaning |
|------------|--------|---------|
| 0x800001   | write  | putchar (unchanged) |
| 0x800002   | read   | next input byte (unchanged, blocking in the harness) |
| 0x800003   | write  | timer period (unchanged) |
| 0x800004   | read   | status: bit 0 = a byte is ready; bit 1 = end of input |

`input_poll` reads 0x800004 and never waits. A byte is read through 0x800002
only when bit 0 is set. Bit 1 means the harness has seen end of input on
stdin. The status read never blocks, so an idle pipe returns "nothing yet".

`cin_poll_tick` runs on each timer tick, but only after `cin_enable_poll`
(the shell boot calls it). It moves up to `CIN_TICK_MAX` (16) bytes per tick
into the ring and stops early when the ring is nearly full.

## 3. End of input (EOF)

- EOF is **sticky**. Once `cin_set_eof` runs, every later read returns 0.
- i386VM: the harness reports EOF through status bit 1 when stdin closes.
  The `eof` session mode (`tests/interactive/session.py --mode eof`) checks
  this. The shell then reports `end of input` and exits with status 3.
- PC: Tiny386 gives the guest no end of input. A PC session only ends when the
  user types `EXIT`. The EOF path is not reachable on the PC, and no PC test
  claims it. The PC's `input_poll` always returns `INPUT_NONE`.

## 4. CON channel

`CON` is opened with `QT_CH_OPEN` as before. Reads follow the R4 rule of
returning what is available, with these changes:

| condition | result |
|-----------|--------|
| bytes waiting | up to `n` bytes, in arrival order (`n` may be 1) |
| nothing waiting, not at end | `CHAN_ERR_AGAIN` (-19) — the read never blocks in the kernel |
| end of input | 0 |

A caller that gets -19 waits with `QT_CON_WAIT` and then retries the read.

## 5. QT_CON_WAIT (0x1C)

    qtrap(QT_CON_WAIT, 0, (unsigned int) &flag, 0)

- `b` points to a flag byte in the caller's memory. The caller clears it before
  the call. It is set to 0xff when input or end of input arrives while the
  caller sleeps.
- If a byte or end of input is already pending, the trap returns 0 at once.
- Otherwise the caller sleeps indefinitely (status -1). It does not spin.
  The arrival path (`con_input_arrived`) clears the status, sets the flag, and
  does not schedule. The job runs again on the next scheduler pass and gets 0
  from the trap.
- Only one job may wait on the console. A second job that calls the trap while
  the first is asleep gets `CON_ERR_BUSY` (-50) and the first waiter keeps its
  wait. (The first version of this trap replaced the waiter, which left the
  first job asleep forever. This was corrected in Phase 5; see the runtime report.)
- The return value is 0 in both cases. Any negative value means the wait was
  refused, and the caller should back off and retry.
- `b` is **not range-checked**, the same as the hold byte of `QT_SUSPEND`. The
  kernel writes one byte to that address on arrival. A caller must pass the
  address of a live byte. This is a known limit, and it is listed in the report.

The shell calls the trap through `APP_QT_CON_WAIT` (0x1C) in `apps/common/appcalls.h`.

## 6. RUN service: QT_IMG_LOAD (0x1D) and QT_JOB_RECLAIMED (0x1E)

    id = qtrap(QT_IMG_LOAD, (unsigned int) name, 0, 0);
    done = qtrap(QT_JOB_RECLAIMED, id, 0, 0);

`QT_IMG_LOAD` takes a user pointer to a NUL-terminated name of at most 15
characters (16 bytes with the NUL). The name is a file name in the WIN1_ root
directory, for example `HELLO.AOU`; the `WIN1_` prefix is added by the kernel.

Order of checks:

1. Copy the name from user memory (`FAT_ERR_NAME` on a bad name, `FAT_ERR_RANGE`
   on a bad pointer).
2. The volume must be mounted (`CHAN_ERR_NOVOL`, -18).
3. Open the root entry (`FAT_ERR_NOFILE`, `FAT_ERR_CHAIN`, `FAT_ERR_NAME`).
4. Size 0 gives `APP_ERR_SIZE`. Size above `IMG_MAX` (8192) gives
   `APP_ERR_TOOBIG` **before anything is read**.
5. Read the file into a kernel heap block (`APP_ERR_NOMEM` if the heap is full).
6. Check the image with the R5 loader (`APP_ERR_MAGIC` and the other `APP_ERR_*`
   checks).
7. The image must be linked for slot 1 (`APP_ERR_NOTSLOT1`, -32).
8. Create the job (`app_load_bytes`), which may return `APP_ERR_SLOTBUSY` or
   `APP_ERR_NOJOB`.
9. Free the staging block and return the job id.

`QT_JOB_RECLAIMED` returns 1 once the job slot has been freed, and 0 while it is
still in use. The shell waits for `QT_JOB_EXIT` and then for reclaim, so the
slot is free before the next RUN.

The boot path shares this loader through `img_load_kernel(name, slot)` (kernel
only). It loads `SHELL.AOU` into slot 0 and returns `APP_ERR_NOTSLOT0` (-33) if
the image is linked for slot 1.

## 7. Errors

| value | name | meaning |
|-------|------|---------|
| -18 | CHAN_ERR_NOVOL | WIN1_ volume not mounted |
| -19 | CHAN_ERR_AGAIN | CON empty: wait with QT_CON_WAIT and retry |
| -20 | APP_ERR_SIZE | file empty or truncated |
| -21 | APP_ERR_MAGIC | not OMAGIC 0407 |
| -22 | APP_ERR_RELOC | relocation records present |
| -23 | APP_ERR_FORMAT | symbol or string table invalid |
| -24 | APP_ERR_LAYOUT | alignment, or larger than a slot |
| -25 | APP_ERR_ENTRY | entry is not a slot base |
| -28 | APP_ERR_SLOTBUSY | the slot is in use |
| -29 | APP_ERR_NOJOB | no free job slot |
| -30 | APP_ERR_NOMEM | no heap memory |
| -31 | APP_ERR_TOOBIG | RUN: larger than 8192 bytes |
| -32 | APP_ERR_NOTSLOT1 | RUN: not linked for slot 1 |
| -33 | APP_ERR_NOTSLOT0 | boot: not linked for slot 0 |
| -40 .. -47 | FAT_ERR_* | I/O, BPB, type, chain, no file, size, name, range (R6) |
| -50 | CON_ERR_BUSY | QT_CON_WAIT: another job is already waiting |

## 8. Directory channel (WIN1_)

Opening `WIN1_` with an empty name returns a directory channel. Each read
returns whole 16-byte records: `name[12]` (BASE.EXT, zero-padded; a full
12-character name has no terminator), then the size as a little-endian u32.
A read buffer smaller than 16 bytes gives `CHAN_ERR_RANGE`. Seek 0 rewinds.
Any other seek is refused. End of listing reads 0.

Entries that are deleted (0xE5), long-name fragments, labels, subdirectories,
and invalid 8.3 names are skipped, and the invalid names are counted in
`fat16_dir_skipped`. The listing never checks cluster chains. `fat16_open_root`
does, so a listed file with a bad chain shows in DIR and then fails to open.

## 9. Name limits

Channel names are limited to 16 bytes including the NUL (`CHAN_NAME_MAX`). The
`WIN1_` prefix uses 5, so a file name passed through a channel is at most 10
characters. This covers the 8.3 names used in R7 (`HELLO.AOU` is 9). A 12-character
8.3 name such as `ABCDEFGH.TXT` cannot be opened through `WIN1_`. The shell
refuses such names with `invalid name` (`SH_NAME_MAX` is 10). Lifting the limit
needs a larger channel name field, which is a change to the R4 channel table.
