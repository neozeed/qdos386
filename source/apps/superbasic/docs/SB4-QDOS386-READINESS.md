# SB4 readiness for a future native i386 build (QDOS/386)

This is an audit, not a port. SB4 does not integrate with QDOS/386, does not
change its kernel, and has no 386 build, run or size measurement. It lists
what the SB4 interpreter depends on, so a later port knows what to replace.
It updates `docs/SB3-QDOS386-READINESS.md`; SB3's channel contract
(`docs/SB3-CHANNEL-ABI.md`) is unchanged.

Private research. See `docs/PROVENANCE-LICENSING.md`.

## 1. Result in brief

- The core uses a small set of C library calls, all in `sb_mem`, `sb_format`,
  `sb_lex`, `sb_expr`, `sb_store`, `sb_value` and `sb_interp`. Host stdio is
  used only by the hosted driver (`sb_main.c`) and the hosted console adapter
  (`sb_console.c`).
- SB4 adds procedure and function call frames. They live on the C stack, one
  nested call per level, so recursion depth costs C stack. This is the main
  new risk for a 386 target.
- A build-only compile with `-m32` succeeds for 3 of 11 core units in this
  container; the other 8 stop at missing 32-bit glibc headers, which is a
  container limit, not a source error. No link or run was attempted.

## 2. Direct use of the C library

Found by `grep` over `src/` and by the undefined symbols of the hosted binary
(`nm -u build/gcc/sb`). Symbols marked *driver* are in `sb_main.c` or
`sb_console.c` only.

| Symbol | Used in | Purpose | Replacement needed on QDOS/386 |
|---|---|---|---|
| `malloc`, `free`, `realloc` | `sb_mem.c` (malloc, free); `sb_main.c` (realloc, driver); `sb_console.c`, `sb_store.c`, `sb_interp.c` (free) | the heap for lines, values, variables, frames | the QDOS heap or a fixed arena; `sb_mem.c` is the only allocator entry |
| `memcpy`, `memset`, `memcmp` | `sb_expr.c`, `sb_format.c`, `sb_interp.c`, `sb_lex.c`, `sb_mem.c` (memcpy); `sb_store.c` (memset); `sb_expr.c`, `sb_interp.c` (memcmp) | string copy, zeroing LOCal arrays, name compare | any libc or a local copy |
| `strlen`, `strchr` | `sb_expr.c`, `sb_interp.c`, `sb_lex.c`, `sb_value.c` (strlen); `sb_format.c` (strchr) | lengths and `E` search in number text | local helpers |
| `strtod` | `sb_format.c`, `sb_lex.c` | number literals and number parsing | the number parser must match the QL float rules; this is the riskiest replacement (see section 5) |
| `strtol` | linked, not called from `src/` (appears in the hosted link; a libc helper, to be confirmed with `objdump -r` before a port) | none in the core | none, if confirmed unused |
| `frexp`, `ldexp` | `sb_format.c` | float formatting to the QL display rules | local versions, or the same float rules as the ROM |
| `floor` | `sb_expr.c`, `sb_format.c`, `sb_interp.c` | `DIV`, `MOD`, number display | local version |
| `pow` | `sb_expr.c` | `^` | local version, with the ROM's overflow rules (KNOWN-LIMITATIONS section 3) |
| `sprintf` (`__sprintf_chk` under fortify) | `sb_format.c`, `sb_interp.c` | exponent text (`E%d`) and `%.16e` | a local integer-to-text routine |
| `toupper`, `isdigit`, `isalpha`, `isalnum`, `isspace` (`__ctype_*_loc`) | `sb_lex.c`, `sb_store.c`, `sb_interp.c`, `sb_expr.c`, `sb_format.c` | name and keyword tests | a table or local functions; the `__ctype_*_loc` calls come from the macros and go away with them |
| `fopen`, `getc`, `fclose`, `ferror`, `fread`, `fwrite`, `fflush`, `fprintf` | `sb_main.c` (driver); `sb_console.c` (`fread`, `fwrite`, `ferror`, `fflush`) | the hosted file and console | the QDOS channel adapter (the SB3 contract, section 6 of `SB3-CHANNEL-ABI.md`); the core never calls these |
| `stdin`, `stdout`, `stderr` | `sb_main.c`, `sb_console.c` | driver only | as above |
| `__stack_chk_fail` | compiler-inserted when stack protection is on | none | build flag |

The core never calls `printf`, `fprintf`, `fopen` or `getc`. Channel output
goes through `sb_chan.h` handles (SB3 contract). POSIX calls are absent from
the core; the build uses only ISO C89 plus the libc above.

## 3. Memory and state

- All interpreter state is in one `SbState` (`docs/INTERPRETER-STATE.md`).
  SB4 adds the frame list (`SbFrame`), the DEFine table (`SbDefEntry`), and
  local flags on `SbVar`, `SbArray` and `SbStrArray`.
- Program lines, variables, arrays and DEFine names are heap allocations
  through `sb_mem`. A port that replaces `sb_mem` controls the whole heap.
- Limits in the hosted build (`src/sb_state.h`): `SB_MAX_GOSUB` 4096,
  `SB_MAX_DIMS` 8, `SB_MAX_ARRAY_ELEMENTS` 1000000, `SB_MAX_CALL_DEPTH` 5500.
  The ROM's own recursion ceiling is between 5000 and 6000 (`p11`).

## 4. Stack

This is the largest new item for SB4.

- Each procedure or function call nests C calls: `stmt_ident` or
  `sb_call_function` to `call_named` to `run_def` to `run_body` to `exec_text`
  and back into the expression parser for arguments.
- Measured on the hosted build with `ulimit -s` (gcc, the SB4 build flags):
  - 8 MB: depth 5499 runs.
  - 6 MB: depth 5499 segfaults.
  - 1 MB: depth 1000 segfaults; 500 runs (earlier measurement).
  So one call level costs roughly 1.1 to 1.5 KB of C stack in this build.
  These are whole-stack measurements, not per-frame sizes.
- A 386 port with a small stack must either raise the stack for calls, or
  lower `SB_MAX_CALL_DEPTH` to what the stack allows, and return -3 at that
  depth. The second option is simpler and is what the hosted core already
  does at its own limit.
- Local variable storage is heap (`sb_mem`), not stack, so the stack holds
  only the call chain and the expression temporaries.

## 5. Floating point

- Numbers are C `double` in the core. The QL float has its own range and
  precision rules; the hosted core reproduces the observed display rules, and
  the known float-range differences are in `KNOWN-LIMITATIONS.md` section 3.
- A 386 port with x87 or without FPU changes rounding and range details, so
  the sweep (`tools/sweep.py`) must be rerun on that target before any claim.
- `strtod`, `frexp`, `ldexp`, `pow` and `floor` are the functions to check.

## 6. Channels

- Opaque channel handles and status codes are defined in `sb_chan.h`
  (SB3 contract). SB4 does not change the channel layer; `PRINT`, `INPUT`
  and `LIST` with `#n` for 0 to 3 are unchanged from SB3.
- A QDOS/386 backend implements the handle operations listed in
  `SB3-CHANNEL-ABI.md` section 6. It is a design only; nothing here calls
  QDOS.

## 7. Build-only i386 probe

Command: `gcc -m32` with the SB4 C89 flags, one object file per core unit.

| Unit | Result |
|---|---|
| `sb_chan`, `sb_error`, `sb_check` | compiled |
| `sb_mem`, `sb_console`, `sb_format`, `sb_value`, `sb_lex`, `sb_store`, `sb_expr`, `sb_interp` | stopped: missing 32-bit glibc headers in this container (`bits/wordsize.h`, `bits/libc-header-start.h`) |

So the source is not yet shown to build for 32 bits. The headers are a
container limit. A port needs a 32-bit C library or a freestanding header
set first.

## 8. What this audit does not show

- No 386 link, no run, no size. No QDOS/386 kernel or R8 code was used.
- No timing on a 386 or a 68000. The sweep and the corpus run on the host.
- No statement about QDOS window or file-channel compatibility. SB4 does not
  claim it.
- The float and string rules still differ from the ROM (`KNOWN-LIMITATIONS.md`).
