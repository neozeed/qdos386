# R14 QLTerm C R1 port map

Source: the supplied `QLTERM-C-R1-FULL-SOURCE.zip` (SHA-256 `fb7e46c1…5b`), imported unchanged as
`third_party/qlterm-c-r1/orig/` (its own `SHA256SUMS`: 53/53 OK). QLTerm C R1 is a C89 translation of Jan
Bredenbeek's 68000 assembly QLTerm 3.00b2 for QDOS, built with QDOS GCC 2.95.3 (68000) and the C68 runtime; it
has never run on QDOS/386. This file is the Stage 0 inventory; it is updated with the status of each item as the
port proceeds (Stage 3/4 columns).

## 1. Modules (7 C files, 1443 lines with headers)

| Module | Lines | Content | QDOS-specific parts |
| --- | --- | --- | --- |
| `main.c` | 343 | startup (`_SPorig` command tail, inherited channels), windows, F3 command menu, Connect/Hangup/Interface/Options/Log/Transmit, child job creation, main key loop | windows via `SCR_`/`CON_` + `sd_wdef`; `mt_cjob`+`mt_activ` with a 68000 `JMP.L` stub; `mt_dmode`; `mt_baud`/`mt_ipcom` (baud, Hermes) |
| `serio.c` | 141 | transmit with TX lock, receive queue, Telnet IAC handling, serial receive job, XON/XOFF, QL key → terminal key sequences | `io_fbyte`/`io_fstrg`/`io_sstrg` with QDOS timeouts; QL key codes |
| `screen.c` | 190 | VT52/ANSI display parser, SGR colours, screen job (queue consumer), log writer | ~20 `sd_*` console operations on the QDOS console driver (pending newline, wrap, scroll in pixels, strip/ink/paper, underline, font) |
| `filexfer.c` | 319 | XMODEM (checksum/CRC), XMODEM-1K, YMODEM batch receive, YMODEM-G, upload, CRC-16, progress | `fs_headr`/`fs_heads`/`fs_save`/`fs_date` (QDOS file headers and dates), `mt_rclck`, `mt_trans` |
| `platform.c` | 85 | big-endian helpers, config accessors, `wait_frames`, **TAS locks**, file-name handling, `DATA_USE`, media wait | 68000 `tas` inline assembly; QL system-variable reads through `mt_inf` (+0x48 driver list, +0xB0 `DATA_USE`, +0xEE pending writes) |
| `config.c`/`.h` | 153 | generated `<<QCFX>>02` MenuConfig block and field offsets | none (bytes; read with `be16`) |
| `font.c` | 78 | generated IBM graphics font (chars 0x80…0xFF, QL font format) | used only through `sd_fount` |
| `crc_table.h`, `qlterm.h` | 134 | CRC table; shared state, word-ABI wrappers for `sd_setin/st/pa/wdef` | `qlterm.h` `#undef`s the libc colour calls and calls `_sd_setin` etc. with `short` arguments |

## 2. Compile inventory (measured, `tests/r14/qlterm-inventory.sh` → `logs/r14/stage0/qlterm-compile-inventory.txt`)

Host cross-compiler GCC 1.40 (C89, i386 a.out), sources unmodified:

* **Without a `qdos.h`: all 7 modules fail** (`qdos.h: No such file or directory`). QDOS/386 has no QL headers and
  the supplied archive does not include the QDOS GCC/C68 `qdos.h`.
* With `tests/r14/inventory/qdos.h` — a 12-line **declarations-only** header (types and prototypes in the shape QLTerm
  uses; no implementation; not part of the product) — **6 of 7 compile** (config, filexfer, font, main, screen,
  serio). `platform.c` fails: `try_lock: unknown register name 'memory' in 'asm'` — the 68000 `tas` statement.
* No other C89 problem was reported by GCC 1.40 (no C99 constructs, no 64-bit types; `long` = 32 bits on both).

Source-change inventory needed (measured, not assumed): **one function** — `try_lock` in `platform.c` — cannot be
compiled for i386. Everything else is a missing *interface*, to be supplied by the Stage 3 compatibility layer.

## 3. QDOS dependencies and their classification

Counts are call sites in the C sources. "Compat" = Stage 3 implements it over QDOS/386 services; "Emulated" =
implemented with a documented difference; "Unsupported" = returns a QDOS error code (never a fake success) with the
blocker stated.

### 3.1 Channels and files

| Call (sites) | QLTerm use | Plan |
| --- | --- | --- |
| `io_open` (9) | `SER1I`/`SER1H`/`SER2I`/`SER2H`, custom device, `TCP_host:port`, `CON_`, `SCR_`, files mode 0/1/2, directory mode 4 | Compat: `SER*` → R14 serial driver; `CON_`/`SCR_` → window objects (3.2); files → `WIN1_`/`WIN1:` channels (`WIN1_NAME`, `WIN1_DIR/NAME`); mode 2 (new, exclusive) refuses an existing file with `ERR_EX` −8 (QLTerm's `_dup` logic depends on it); mode 4 opens the parent directory for `fs_headr`. **`TCP_`: Unsupported (−7 not found): QDOS/386 has no TCP/IP stack.** |
| `io_close` (8), `io_sbyte` (4), `io_sstrg` (6) | serial, windows, log/download files | Compat |
| `io_fbyte` (8), `io_fstrg` (3) | serial with QDOS timeouts (0, frames, −1 forever), keyboard, upload files | Compat: timeout in 50 Hz frames → 100 Hz ticks ×2; `ERR_NC` −1 on timeout, `ERR_EF` −10 at end of file; never a busy spin (the process sleeps in the kernel) |
| `io_edlin` (1) | Connect/file-name line editor | Compat: QDOS line-edit semantics as used (buffer, cursor, terminator ENTER / ESC / up / down included in the count) |
| `fs_headr` (2) | upload: length, file type (executable → 30-byte QDOS header prefix), update date; directory header for the sub-directory name length | Emulated: QDOS/386 files have no QDOS header: length = file size, type 0 (data), date 0 (`qd_stat` has no time); directory header gives the directory name length |
| `fs_save` (1) | download data | Compat (write) |
| `fs_heads` (1) | download of a file that starts with the `]!QDOS File Header` marker | **Unsupported (−19 `ERR_NI`)**: FAT files cannot carry a QDOS header (type/dataspace). Such a download stops with QLTerm's own file-error path |
| `fs_date` (1) | YMODEM date | **Unsupported (−19)**: no kernel call sets a FAT time stamp. QLTerm ignores this return value, so the file keeps the time of writing (documented) |

### 3.2 Screen (`sd_*`, 30 sites in `screen.c`/`main.c`)

QLTerm leaves cursor geometry, wrapping, pending newline and rendering to the QDOS console driver. QDOS/386 consoles
are 80×25 character cells. Plan: a window object in the compat layer (origin, size, cursor, ink/strip/paper,
underline, pending newline, cursor visibility) drawn into the program's own console session with the native calls
`qd_con_goto`, `qd_con_attr`, `qd_write`, `qd_con_clear`/region. QL pixel geometry (6×10 per character) is converted
to cells; pixel scroll/pan distances must be multiples of 10/6 (QLTerm only uses those). QL colours 0…7 map to the
CGA attribute palette. `sd_fount` with QLTerm's IBM graphics font: the PC text font already is code page 437 (the same
glyphs) — the call is accepted and the native font is used (documented; the 640×480 graphical-text presenter's own
font coverage applies). `sd_wdef` fails with `ERR_OR` −4 if the window does not fit (QLTerm then falls back to its
standard size). `mt_dmode` reports mode 4 (not 8, which QLTerm refuses).

### 3.3 Jobs, time, system

| Call (sites) | QLTerm use | Plan |
| --- | --- | --- |
| `mt_cjob` (1) + `mt_activ` (1) | two child jobs (screen job, receive job) **sharing the parent's globals**, created by writing a 68000 `JMP.L entry` stub into the new job's code area | Emulated: QDOS/386 processes have private address spaces, so a kernel job cannot share QLTerm's globals. The compat layer runs the children as cooperative co-jobs (separate C stacks) inside the QLTerm process; `mt_cjob` returns a co-job id and a code area; `mt_activ` accepts **only** the exact 6-byte stub QLTerm writes (0x4EF9 + 32-bit address) and refuses anything else with `ERR_BP` −15 |
| `mt_prior` (9) | 0 stops a co-job outside a critical section; 32/127 restart it | Compat (co-job scheduler) |
| `mt_susjb` (1, via `wait_frames`) | all waits and polling | Compat: the co-job sleeps; when every co-job sleeps, the process sleeps in the kernel until the earliest deadline, serial data or a key |
| `mt_frjob` (2) | exit with status | Compat (`exit`) |
| `try_lock` (TAS) | locks shared by the three jobs | i386 `xchg` (atomic); the requirement is mutual exclusion across the co-jobs' yield points (a co-job may yield while holding a lock). The only source change |
| `mt_rclck` (3) | transfer throughput, progress | Compat: QL seconds = Unix time + 283996800 (the source's own `QL_EPOCH`); clockless kernel → 0 |
| `mt_inf` (3) | system variables: driver list (+0x48), `DATA_USE` (+0xB0), pending writes (+0xEE) | Emulated: a private, documented block: no driver list (0), `DATA_USE` = a configured default directory string, pending writes 0 (QDOS/386 writes are synchronous to its cache; `SYNC` flushes) |
| `mt_trans` (2) | optionally disable serial translation during transfers | Compat: the R14 serial driver never translates; returns 0 |
| `mt_baud` (1) | set the serial rate | Compat → serial driver configuration |
| `mt_ipcom` (3) | Hermes 12H75 test/command; BEL beep | **Unsupported (−19)**: no QL IPC/Hermes. Baud 12H75 is refused; BEL does not sound |
| `ut_err` (1) | print an error message | Compat (QDOS message texts) |
| `mt_dmode` (1) | screen mode | Compat (reports mode 4) |

### 3.4 C68 runtime conventions

`_SPorig` (inherited channel count + converted command tail): built by a compat `main` wrapper from `argv` (count 0;
tail = the joined arguments) — QLTerm's own `main` is compiled as `qlterm_main`. `_prog_name` ("QLTerm C") is already
the QDOS/386 R12C title convention. `_stack`, `_mneed`, `_cmdparams`, `_conchan`, `_stdchans` and similar are C68
startup hooks: they remain as ordinary definitions with no effect. libc used: `sprintf strtol strtoul strncpy strchr
strlen strcpy strcat memcpy memmove memset memcmp malloc free` — all in the QDOS/386 libc.

### 3.5 Big-endian, layout, error codes

* QLTerm reads every QDOS byte structure (file headers, config block, inherited channels, Telnet window size)
  through `be16`/`be32`/`put16`/`put32` — byte-order independent. It never overlays a struct on QDOS memory except
  the `QLRECT_t`/`WINDOWDEF_t` it fills itself; their layout is the compat header's own (native i386). **The QL binary
  ABI (68000 code, trap numbers, QDOS memory layout) is not provided and is not claimed.**
* Error codes are the QDOS values QLTerm compares with: −1 NC, −4 OR, −7 NF, −8 EX, −10 EF, −12 BN, −15 BP, −19 NI.
* Timeouts are `short` frames; −1 = forever.

## 4. Features and expected port status (Stage 0 view)

| Feature | Depends on | Expected |
| --- | --- | --- |
| Terminal over `SER1`/`SER2` (I and H variants), echo modes, ASCII/VT52/ANSI keys and display | serial driver, compat | port |
| Telnet / raw TCP (`TCP_`, default `PORT=6`) | a `TCP_` device | **unsupported** — no TCP/IP on QDOS/386; the build's configuration selects SER1I, as the original's own tests reconfigured it |
| Custom device port (`PORT=4`) | any QDOS/386 channel name | port (a file or `SER2` name) |
| Baud selection | serial driver | port, except 12H75 (Hermes IPC) |
| Log (raw/cooked), Transmit text file | files | port |
| XMODEM/1K/YMODEM/YMODEM-G down and up | serial, files | port; QDOS-header files and YMODEM dates as in 3.1 |
| Download buffer (B), receive buffer, XON/XOFF | memory, serial | port |
| BEL | `mt_ipcom` | unsupported (silent) |
| EX/EW inherited channels (1–3) | QDOS `EX` | not available: QDOS/386 passes no channels to a new program; count 0 always |
| MenuConfig configuration | the `<<QCFX>>02` block in the executable | a host tool patches the block in the built program (the role of MenuConfig) |

## 5. Port result (Stages 3–4)

**Build** (`scripts/r14-build-qlterm.sh`): the seven modules copied from `third_party/qlterm-c-r1/orig/src`, the
patch `third_party/qlterm-c-r1/r14-qdos386.patch` applied (one function, `try_lock`: 68000 `TAS` → i386 `XCHG`;
`build/r14/qlterm/source-changes.diff` is the measured inventory), compiled with GCC 1.40 through `qdos386-gcc`
together with `sdk/qlcompat` (`-D main=qlc_program_main`), then configured in the executable by
`tools/r14-qlterm-config.py` (the MenuConfig role: it locates the `config_blob` bytes and changes only those):
`PORT=0` (SER1I), `BAUD=10` (115K2), `DLDIR=WIN1_HOME/`. No other source line differs. Executable 72722 bytes.

**Run**: `START QLTERM /NEW` (or `QLTERM`): its own session, title "QLTerm C" (`_prog_name`), keys only in the
foreground, the receive and screen jobs as co-jobs that keep running while the session is in the background; the
Task Manager switches to and from it.

### Feature parity with QLTerm C R1 (Gate 4, `logs/r14/gate4/`; Stage 5, `logs/r14/gate5/`)

| Feature (README of the supplied source) | Status | Evidence / blocker |
| --- | --- | --- |
| Terminal on SER1I: send keys, show received text | PASS | typed text at the peer; 300 / 600 streamed lines |
| SER1H, SER2I, SER2H | NOT TESTED in QLTerm | the driver's H handshake and SER2 are tested in Gate 1 / host test |
| Custom QDOS device port (PORT 4) | NOT TESTED | |
| Raw TCP and Telnet (PORT 5/6; supplied default 6) | NOT SUPPORTED | no `TCP_` device: QDOS/386 has no TCP/IP stack; QLTerm shows "Could not open comms device TCP_example.org:23" (tested) |
| Telnet BINARY/ECHO/SUPGA/terminal type/window size | NOT SUPPORTED | needs Telnet (above) |
| Echo Off (default) / Echo On / Local only | PARTIAL | Off PASS; On and Local NOT TESTED |
| Keyboard modes: ANSI, VT52, ASCII | PARTIAL | ANSI cursor keys → ESC[A/B/D/C PASS; VT52 (Options) → ESC A PASS; ASCII NOT TESTED |
| Display parser: CSI H/K, TAB, BS, CR LF, VT52 ESC Y, SGR colours, inverse, cursor position report | PASS | screen text and attributes (`%cells`) |
| Underline | PARTIAL | shown as bright ink: VGA colour text has no underline |
| Scrolling | PASS | streamed lines; panning (CSI space @/A) NOT TESTED in QLTerm (host-tested `sd_pan`) |
| Cursor save/restore (CSI s/u) | NOT TESTED | |
| Colour modes (auto = 4-colour in MODE 4, 8-colour, none) | PARTIAL | 4-colour PASS; others NOT TESTED |
| IBM graphics font (`sd_fount`) | PARTIAL | accepted; the console's code page 437 font (same glyph set) is used, not QLTerm's bitmap data |
| Baud selection | PARTIAL | 115K2 PASS (configured and re-selected in Interface); other rates NOT TESTED; 12H75 (Hermes IPC) NOT SUPPORTED (`mt_ipcom` → ERR_NI) |
| Modem strings: prefix/number/postfix, hang-up with `~` waits | PASS | ATD5551234 CR; +++ATH0 on Quit |
| Connect remembers the last entry; ↑/↓ clears it | PASS | the remembered number was appended once, ↑ cleared it |
| Receive buffering (64 KiB queue) | PASS | no line lost while the session was in the background |
| XON/XOFF backpressure | NOT TESTED | needs the queue 3/4 full |
| Log (cooked, CR/LF conversion) | PASS | 300 and 600 lines in the log files, exact |
| Log raw | NOT TESTED | |
| Transmit text file (LF → CR LF, unterminated last line kept) | PASS | |
| XMODEM checksum | PARTIAL | upload to `rx` (8-bit checksum) PASS; download NOT TESTED (QLTerm asks CRC first) |
| XMODEM CRC download / upload | PASS / PASS | lrzsz `sz --xmodem`, `rx -c`; byte-exact |
| XMODEM-1K download / upload | PASS / PASS | with a lost block: "Timeout #1", NAK, retransmission, exact |
| YMODEM batch receive (128 and 1K blocks), exact length | PASS | 50001 and 70001 bytes; a corrupted byte → CRC error → repeated block |
| YMODEM upload | PASS | the name without device/directory (`UP.BIN`), exact length |
| YMODEM-G receive | PASS | lrzsz streamed without ACKs |
| YMODEM-G upload | NOT TESTED | |
| Multi-file YMODEM batch, duplicate names (`_dup1`) | NOT TESTED | |
| Cancel (CTRL-X) | PASS | CAN CAN sent, partial file kept, QLTerm continues |
| Remote cancel | NOT TESTED | |
| QDOS executable header in a download | NOT SUPPORTED | `fs_heads` ERR_NI (FAT has no QDOS header): QLTerm reports "FILE ERROR: not implemented" and cancels (tested) |
| QDOS header in an upload | PARTIAL | QDOS/386 files are type 0: no header is ever sent |
| YMODEM file dates | NOT SUPPORTED | `fs_date` ERR_NI (QLTerm ignores the result; the file keeps its write time) |
| Progress / throughput on the status line | PASS | |
| Separate receive and screen jobs | PASS | co-jobs (`sdk/qlcompat/qlc_job.c`) |
| Menus (F3; ← → Enter Esc), Options, Interface, Quit | PASS | |
| Buffer size command (B) | NOT TESTED | |
| MenuConfig block | PASS | changed in the built executable by the host tool, read by QLTerm |
| EX/EW inherited channels | NOT SUPPORTED | QDOS/386 starts programs without channels (count 0) |
| BEL | NOT SUPPORTED | `mt_ipcom` ERR_NI: no sound |
| MODE 8 refusal | NA | `mt_dmode` always reports MODE 4 |
