# R14 QL compatibility layer (`sdk/qlcompat`)

A **narrow, source-level** compatibility layer for C programs written for QDOS (C68 / QDOS GCC), sized to what
QLTerm C R1 actually uses (inventory: `docs/R14-QLTERM-PORT-MAP.md`). It is **not** the QL binary ABI: no 68000
code runs, there are no QDOS traps, system variables or QL memory map, and structures have the native i386 layout.
Everything is implemented on QDOS/386 services; what cannot be provided returns a QDOS error code — never a
pretended success.

## Files and use

| File | Content |
| --- | --- |
| `sdk/qlcompat/include/qdos.h` | the declarations (types, error codes, calls) — only those QLTerm uses |
| `sdk/qlcompat/qlc_io.c` | channels (`io_*`), files (`fs_*`), `mt_baud`, `mt_ipcom`, `mt_trans`, `ut_err`, the C68 start |
| `sdk/qlcompat/qlc_win.c` | `CON_`/`SCR_` windows on the console session, `sd_*`, key translation, `io_edlin` |
| `sdk/qlcompat/qlc_job.c` | `mt_cjob`/`mt_activ`/`mt_prior`/`mt_susjb`/`mt_frjob` as co-jobs, `mt_rclck`, `mt_inf`, `mt_dmode` |
| `sdk/qlcompat/qlc.h` | internal |

Build a program with the SDK compiler, adding the three sources and renaming its `main`:

    qdos386-gcc -O -I sdk/qlcompat/include -I sdk/qlcompat -D main=qlc_program_main -o PROG.aout prog.c ... \
        sdk/qlcompat/qlc_io.c sdk/qlcompat/qlc_win.c sdk/qlcompat/qlc_job.c

The layer's own `main` builds `_SPorig` (C68: a word with the number of inherited channels — always 0, QDOS/386 passes
none — followed by the command tail as a C string, the arguments joined by spaces) and calls `qlc_program_main`.
`_prog_name` is already the QDOS/386 title convention (R12C).

## Types and ABI audit

| Item | QDOS / C68 (68000) | Here |
| --- | --- | --- |
| `chanid_t`, `jobid_t` | 32-bit | `long` (32-bit); channel ids are `0x10000 + slot`, job ids `0x200 + co-job` |
| `timeout_t` | 16-bit, frames of 1/50 s, −1 forever | `short`; converted to 100 Hz ticks ×2 |
| `colour_t` | byte; C68 stubs fetch a word | `unsigned char`; the `_sd_setin/st/pa/wdef` entry points take `short` (QLTerm calls those) |
| `QLRECT_t` | 4 × 16-bit | 4 × `short` (8 bytes, native order) — filled and read by the program only |
| `WINDOWDEF_t` | QLTerm's own struct | native layout |
| byte order | big-endian | QLTerm reads every QDOS byte structure (file headers, its config block, Telnet sizes) through `be16`/`be32`; the layer writes those structures big-endian (`fs_headr`) |
| error codes | negative QDOS codes | the same values (`ERR_NC` −1 … `ERR_BL` −21); native QDOS/386 codes are translated (`qlc_err`) |
| inline 68000 `TAS` | test-and-set on a byte | not provided: the one-function source patch uses i386 `XCHG` (see below) |
| pointers | 32-bit | 32-bit |
| QL time | seconds since 1961 | `mt_rclck` = Unix time + 283 996 800 (QLTerm's `QL_EPOCH`); 0 when the kernel has no clock |

## Calls

### Channels and files

* `io_open(name, mode)`: `CON_…`/`SCR_…` → a window (geometry suffixes are ignored; `sd_wdef` sets it); `SER1`/`SER2`
  with the QL suffix letters → the R14 serial driver, set to the last `mt_baud` rate (default 9600); `WIN1_path` →
  the WIN1 volume (`/` separates directories, case-insensitive, VFAT long names); a name starting with `/` → the same.
  **`TCP_`, `RAM1_`… and names without a device → `ERR_NF`.** Modes: 0 read/write existing, 1 read, 2 new
  (`ERR_EX` if it exists), 3 new or overwrite, 4 directory (only `fs_headr`: the directory's name and length, which
  QLTerm uses to cut the directory from a YMODEM file name).
* `io_fbyte`, `io_fstrg`, `io_sbyte`, `io_sstrg` with QDOS timeouts: 0 = do not wait (`ERR_NC`), *n* frames,
  −1 forever. `io_fstrg` returns the bytes read (> 0) or the error (`ERR_NC` on timeout, `ERR_EF` at end of file);
  `io_sstrg` returns the bytes written. Waiting yields to the other co-jobs; the process sleeps in the kernel.
* `fs_headr`: a 64-byte QDOS header as far as QDOS/386 knows it: length (big-endian), type 0 (data; 255 for the
  mode-4 directory), no dataspace, the name; **dates 0** (QDOS/386 files have no time the layer can read).
* `fs_save`: write. **`fs_heads` and `fs_date`: `ERR_NI`** — FAT files carry no QDOS header (type, dataspace) and no
  kernel call sets a file time.
* `ut_err`: the QDOS message texts.

### Screen (`sd_*`)

The windows of one program share its console session (80 × 25 cells; `START … /NEW` gives it a private one). Pixel
geometry: 6 × 10 pixels per cell; cell (0, 0) is QL pixel (16, 3) (the 480 × 250 pixel area centred in the QL's
512 × 256 screen); a window's origin is rounded to the nearest cell and its size truncated; borders take no cells and
are not drawn. `sd_wdef` of a window that does not fit returns `ERR_OR` (QLTerm then falls back to its standard size).
Semantics follow the QDOS console as QLTerm uses it: pending newline after LF and after the last column, `sd_donl`;
`sd_pos/tab/ncol/pcol/nrow/prow` fail with `ERR_OR` at the edges; `sd_scrol`/`sd_pan` move by whole cells
(pixels / 10, pixels / 6); `sd_clear/clrtp/clrbt/clrln/clrrt` fill with paper; ink, strip (character background)
and paper. Colours: **MODE 4** (`mt_dmode` reports mode 4): QL colour & 6 → black, red, green, white (CGA
attributes). **Underline is shown as bright ink** (VGA colour text has no underline). `sd_fount` is accepted: the
console's code page 437 font has the same IBM graphics glyphs as QLTerm's font. `sd_cure`/`sd_curs` control the one
hardware cursor (the last window that enabled it).

`io_edlin`: the line before `*p` is edited in place; ENTER, ESC, ↑ and ↓ end it and are stored after the text; ←/→
move, backspace (QL CTRL+←) and Delete (QL CTRL+→) delete; text beyond the window's right edge is kept but not shown.

Keys (native → QL): ENTER → 10, Backspace → 0xC2, ← → ↑ ↓ → 0xC0 0xC8 0xD0 0xD8, Home/End → ALT+←/→ (0xC1/0xC9),
PgUp/PgDn → SHIFT+↑/↓ (0xD4/0xDC), Delete → 0xCA, F1–F5 → 0xE8 0xEC 0xF0 0xF4 0xF8, F6–F10 → SHIFT+F1–F5
(0xEA … 0xFA), ESC, TAB, CTRL+letter and text unchanged. Keys reach a program only while its session is in the
foreground.

### Jobs and system

* `mt_cjob` + `mt_activ`: QLTerm's children run C code of the parent and share its globals, so they cannot be kernel
  jobs (private address spaces since R12A). They are **co-jobs**: separate 32 KiB C stacks in the same process,
  switched (i386: `EBP EBX ESI EDI ESP`, `qlc_job.c` `co_switch`) only inside the layer's waiting calls
  (`mt_susjb`, and the waits of `io_fbyte`/`io_fstrg`/`io_sstrg`/`io_edlin`). `mt_activ` accepts **only** the exact
  6-byte stub QLTerm writes (`0x4EF9` + a 32-bit address, the QDOS `JMP.L`), otherwise `ERR_BP`. When every co-job
  waits, the process sleeps in the kernel (`qd_ser_wait` for new serial data or a new key, else `qd_yield`) until
  the earliest deadline. `mt_prior` 0 stops a co-job, any other value runs it; co-jobs run round-robin (no numeric
  priorities). `mt_susjb(-1, n)` sleeps *n* frames. `mt_frjob(-1, e)` ends the program.
* `mt_inf`: a private block standing for the three system variables QLTerm reads: +0x48 driver list **empty**, +0xB0
  `DATA_USE` = the current directory as `WIN1_DIR/`, +0xEE pending writes 0. No other system variable exists.
* `mt_baud(rate)`: the rate (must divide 115200) for SER1/SER2 now and for the next open; otherwise `ERR_BP`.
* `mt_trans`: 0 (the serial driver never translates). **`mt_ipcom`: `ERR_NI`** (no QL IPC: no BEL sound, no Hermes
  12H75). `mt_dmode`: reads mode 4; setting a mode is `ERR_NI`.

## The concurrency requirement behind QLTerm's TAS

QLTerm's three jobs share a receive queue and the transmit path, protected by `try_lock` (68000 `TAS`) and
`wait_frames` retries; a worker can hold a lock across a QDOS call that waits (`tx_raw` waits with the transmit lock
held). With co-jobs switching only at such calls, a lock is needed for exactly that case; the patch keeps QLTerm's
locks and replaces only the instruction: `XCHG` of 0xFF with the lock byte (indivisible on i386 as `TAS` is on the
68000). Gate 3 tests a lock held across switches by two co-jobs (no overlap).

## Tests

`tests/r14/host-qlc.c` (host C89 build with mocked SDK calls, co-jobs on `ucontext`), `tests/r14/qlprobe.c` (guest
probe on QEMU: real stacks, kernel clock, console, WIN1, serial), `tests/r14/qlterm-inventory.sh` (compile
inventory with this header). Results: `logs/r14/gate3/`.
