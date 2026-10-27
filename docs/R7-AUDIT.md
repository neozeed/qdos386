# QDOS/386 R7 — Phase 0 audit

Scope: the input path, the trap and channel conventions, memory slots, and the
FAT16 directory walker, as required by the R7 plan (Phase 0). Every claim below
is from source or a run on this machine. Items marked OPEN are not yet verified.

## 1. Tiny386 keyboard path (PC)

Source: `tiny386-master/i8042.c`, `term.c`, `main.c`, `pc.c` (build copy; the
source tree was not modified).

- The keyboard controller is a QEMU-derived i8042 with a PS/2 keyboard. Port
  0x60 is data, 0x64 is status and command. Status bit 0 is OBF.
- Scan codes are **set 1**. `ps2_put_keycode(kbd, is_down, linux_keycode)` queues
  make codes, break codes (bit 7) and 0xE0 prefixes.
- IRQ1 is raised from `kbd_update_irq` only when the controller command byte
  has `KBD_MODE_KBD_INT` (bit 0) set and `KBD_MODE_DISABLE_KBD` clear. The
  guest must therefore write the command byte (0x60) before IRQ1 can arrive.
  SeaBIOS may leave this state; the kernel must set it explicitly.
- **Headless input route.** `tiny386_headless -term` calls `term_init` and
  `term_step`. `term.c` puts the host terminal into raw mode and maps each ASCII
  byte to make/break scan codes (`ascii_to_scan`). Upper-case and shifted
  characters are sent with a Shift make/break around them. Backspace, Enter and
  Escape have their own codes (`KEY_BACKSPACE` 0x0E, `KEY_ENTER` 0x1C).
  The route is supported by the headless binary's own usage line.
- **Piped stdin works.** Run on the R6 PC image with `printf 'DIR\r' | ... -term
  run.ini`: the emulator starts, runs the R6 kernel to its PASS line, and exits
  with status 0 (`termpipe.log`). The R6 kernel does not read the keyboard, so
  this shows the route does not break the emulator; it does not yet show that the
  guest receives the bytes. That is a Phase 1 test.

**Decision (PC):** read keys through the i8042 (port 0x60, IRQ1) as the device
does. No BIOS INT 16h, no kernel-side key generation. Automated tests drive this
route with a piped `-term` session; a human test uses a real terminal.

## 2. i386VM input

Source: `x/i386vm/.../emulator-tiny386/machine.c` (the external harness).

- Port 0x800002 is an input byte: `rd()` calls `getchar()` and **blocks** until a
  byte arrives; at EOF it prints "input EOF" and exits the emulator.
- There is **no status register**. A guest cannot ask "is a byte waiting?"
  without blocking the whole emulator (the harness has no other thread, so the
  timer and the guest stop together).
- Port 0x800001 is output (`putchar`). Port 0x800003 is the synthetic timer.

**Decision (i386VM):** add a minimal status byte at 0x800004 (1 if a byte can be
read without blocking, checked with `poll()` on stdin plus a one-byte peek buffer)
as a documented harness patch under `patches/`. The 0x800002 read, the output
port and the timer stay as they are. The patch is isolated, and the
unpatched harness is recorded by hash.

Open (OPEN-1): confirm the 0x800004 patch preserves the existing output and timer
behaviour by re-running the R6 i386VM matrix with the patched harness.

## 3. Trap and channel conventions (R6 audit)

From `kernel/jobs.c` (`trap_dispatch`), `kernel/jobs.h`, `kernel/channel.c`:

- `int 0x30` through an interrupt gate (type 0x8E): IF is cleared for the whole
  trap. A trap cannot be preempted; this is the basis of the R6 "no
  interleaved reads" argument and of the R7 rule "never busy-wait inside a trap".
- Calling convention: EAX = function, EBX/ECX/EDX = arguments, result in EAX.
  Negative results are errors (table in `docs/R6-ABI.md`).
- Traps used: 0x08–0x0F (scheduling), 0x10–0x13 (memory), 0x14–0x17 (channels),
  0x18–0x19 (exit records), **0x1A `QT_CH_READ`, 0x1B `QT_CH_SEEK` (R6)**.
  **Free: 0x1C upward.** R7 uses 0x1C–0x1F for its additions (checked against the
  full dispatch table: nothing above 0x1B is dispatched).
- Blocking read contract (R7, planned): a `CON` read that finds no input returns
  immediately with `CHAN_ERR_AGAIN` (-19) in nonblocking mode. In blocking mode the
  trap does **not** spin. It records the waiting job, returns a "retry" status to
  the job's wrapper, and the wrapper suspends with `QT_SUSPEND` until the ISR marks
  the job runnable. Where the wait happens is decided in Phase 1 (OPEN-2).
- EOF: a `CON` input channel never reaches EOF; reads return 0 only after
  an explicit close. File reads return 0 at end of file (R6).

## 4. Memory map and loader staging

From `kernel/appmap.h`, `kernel/aout.c`, `tests/test_fs.c`:

| Region | Address | Use |
|--------|---------|-----|
| Kernel | 0x000000 (i386VM) / 0x100000 (PC) | kernel, BSS to 0x00E310 / 0x10E0F0 |
| Heap | to 0x200000 | jobs, channels, staging is not here |
| Slot 0 | 0x200000–0x23FFFF | first a.out |
| Slot 1 | 0x240000–0x27FFFF | second a.out |
| Transfer | 0x280000–0x2BFFFF | R5 images |
| RAM disk (i386VM) | 0x2C0000–0x5BFFFF | FAT16 volume, read only |
| Stack floor | 0x600000 | boot stack top 0x7FFFF0 |

- The R6 loader staging buffer (`stage[8192]`) is a static kernel buffer, below
  the heap end, and therefore a fixed 8 KiB limit on file size for `RUN`.
- **Shell in slot 0, child in slot 1:** `app_load_bytes` takes the slot from the
  image's entry address, so the shell (slot 0) and a child (slot 1) can coexist.
  R5 already runs prep in slot 1 alongside hello in slot 0. OPEN-3: prove a
  shell-in-slot-0 with repeated slot-1 `RUN`s, with a stale-id check after each.
- **Staging limit (R7 decision).** R7 caps `RUN` at the 8 KiB staging buffer and
  tests the rejection of a larger file. No larger loader region is added in R7,
  so arbitrary program loading is not claimed.

## 5. FAT16 directory walker

From `kernel/fat16.c`, `kernel/win1.c`:

- `fat16_open_root()` scans the root directory for one name. It skips deleted
  (0xE5), volume-label/directory/long-name (attribute bits 0x18) entries and stops
  at the end marker (0x00). There is **no enumeration function**.
- The root sectors, the entry layout and the end-marker rule are already
  validated bounds in `fat16_open_root`, so a listing can reuse them.
- **Chosen interface (Phase 2):** a single common function in `fat16.c`,
  `fat16_list_root(vol, index, out)`, that returns the N-th valid root entry
  (name as 8.3 text, size, first cluster). It walks the same bounded sectors and
  refuses entries past `root_sectors`. The shell never reads FAT or directory
  bytes itself. Exposed to applications as channel reads on a directory channel
  (`WIN1_` with an empty name), so no new listing trap is needed.

## 6. Summary of the decisions

1. PC keyboard: i8042 port 0x60 / IRQ1 with set-1 scan codes. Headless input by
   `-term` (verified accepting piped stdin).
2. i386VM input: existing 0x800002 byte port plus a new status byte at 0x800004
   (harness patch, documented, output and timer unchanged).
3. New traps from 0x1C; no existing number changes.
4. Blocking reads wait by suspending, never by spinning inside a trap.
5. Shell in slot 0, children in slot 1, slot-1 links only, 8 KiB executable cap
   tested.
6. Directory listing in `fat16.c`; applications get it through a directory
   channel.

## 7. Open items before Phase 1 code

- OPEN-1: harness status byte keeps the R6 matrix unchanged.
- OPEN-2: the blocking wait mechanism (ISR wake-up vs the scheduler's
  suspend), confirmed against `jobs.c` before writing it.
- OPEN-3: the slot-0/slot-1 coexistence run.
