# R11B stdio ABI

Scope: `sdk/libc/stdio.c` and `sdk/include/stdio.h` in `libc.a`, built with the
unchanged GCC 1.40 / GAS 1.38 / i386-ld tree. Only the console is writable.
Files on the WIN1_ FAT16 volume are read-only.

## Stream model

Every `FILE` is a `struct qd_file` with:

| Field | Meaning |
|---|---|
| `kind` | `KIND_CON` (the CON channel) or `KIND_FILE` (a WIN1_ file channel) |
| `dir` | `DIR_READ` or `DIR_WRITE`. The only direction a stream accepts. |
| `eof`, `err` | sticky flags, cleared by `clearerr` (and `rewind`) |
| `pos` | logical byte position of a file stream |
| `has_unget`, `unget_c` | one pushed-back character |
| `bufmode` | `_IOFBF`, `_IOLBF` or `_IONBF` (output streams) |
| `wlen`, `wbuf[256]` | the output buffer |

The three standard streams are static objects:

| Stream | Direction | Default buffering | Channel |
|---|---|---|---|
| `stdin` | read | (input is not buffered by the library) | CON |
| `stdout` | write | line buffered (`_IOLBF`) | CON |
| `stderr` | write | unbuffered (`_IONBF`) | CON |

The kernel has one console channel. `stdout` and `stderr` both write to it, so
the two streams interleave in the order the calls are made, and `stderr` text
is never held back behind `stdout` text.

## Output buffering

- A character put into a buffered stream is held in `wbuf`.
- `stdout` (`_IOLBF`) is written when a newline is put, when the 256-byte buffer
  is full, on `fflush`, before any read from `stdin`, and at exit.
- `stderr` (`_IONBF`) is written at the end of each public call
  (`fputc`, `fputs`, `fwrite`, `printf` family).
- **Exit.** `exit()` writes pending `stdout` and `stderr` text (`qd_flush_std`)
  and then calls the exit trap. Returning from `main` also flushes: the startup
  object `sdk/lib/crt0_libc.s` calls `_qd_flush_std` before the exit trap.
  `bin/qdos386-gcc` links `crt0_libc.s` when `libc.a` is present, and the
  libc-free `crt0.s` otherwise, so R8-style programs that use only `qd_*` still
  link. A program that dies in a way the kernel reports (an exception) loses
  its unflushed `stdout` text, as it would on any C system.
- Test: `EXITFL.AOU` prints text with no newline and returns; the text is in the
  console log before `exit status 0`.

## Input

- `fgetc`, `getc`, `getchar` and `fgets` on `stdin` read the CON channel one
  character per trap. While no character is ready, the call yields one tick and
  tries again (`CHAN_ERR_AGAIN`), so a waiting program does not spin the CPU.
- Before waiting, pending `stdout` text is written, so a prompt is visible.
- **Enter key.** The keyboard driver sends CR (`0x0d`) for Enter. `stdin`
  returns it as LF (`'\n'`), so `fgets` ends a typed line and text-mode
  programs see the usual newline. Other characters are unchanged.
- `ungetc` accepts one character per stream until it is read; a second `ungetc`
  before a read fails and returns `EOF`. `fseek` discards it.
- `fgetc` on a stream opened for writing (`stdout`, `stderr`) sets the error flag
  and returns `EOF`.

## Function behaviour

| Function | Behaviour |
|---|---|
| `fopen(name, mode)` | Only `"r"`, `"rb"`. Any other mode returns `NULL`: `"w"`, `"a"`, `"r+"`, `"w+"` and so on. A name without `WIN1_` gets it added; the whole name must fit `FILENAME_MAX` (16). Missing files return `NULL`. |
| `fclose(fp)` | Files: closes the channel and frees the stream. `stdin`, `stdout`, `stderr`: flush the output and return 0; the streams stay open. `NULL` returns `EOF`. |
| `fflush(fp)` | Output stream: writes its buffer, returns 0 or `EOF`. Input stream: no effect, returns 0 (C89 leaves this undefined). `fflush(NULL)` writes `stdout` and `stderr` and returns 0 or `EOF`. |
| `setvbuf(fp, buf, mode, size)` | Output stream: `buf` must be `NULL` (the library owns the buffer; a caller buffer returns -1). Flushes, then sets the mode. Input stream: the mode is accepted and has no effect. |
| `setbuf(fp, buf)` | `buf == NULL` selects `_IONBF`. A non-NULL buffer has no effect. |
| `putc`, `fputc`, `putchar` | Put one character; return it, or `EOF` on error. |
| `fputs`, `puts` | Put a string; `puts` adds a newline. Return 0 or `EOF`. |
| `fwrite(p, size, n, fp)` | Returns `n` when every byte was written; returns 0 otherwise (conservative, see the test). |
| `printf`, `fprintf`, `vprintf`, `vfprintf` | Share one formatter with `sprintf`. Return the number of characters written, or -1 on a write error. |
| `sprintf`, `vsprintf` | Unbounded (C89): the caller must size the buffer. |
| `snprintf`, `vsnprintf` | **C99, not C89.** Write at most `size-1` characters plus NUL; return the length the full output would have had. |
| `fread`, `fgets`, `ftell`, `fseek`, `rewind` | File streams only. `fseek` supports `SEEK_SET` and `SEEK_CUR`; `SEEK_END` returns -1 (the kernel has no file size query). `ftell` and `fseek` on a console stream return -1. `rewind` is `fseek(fp, 0, SEEK_SET)` plus `clearerr`. |
| `feof`, `ferror`, `clearerr` | Sticky flags. `feof` is set by a read that reaches the end; `ferror` by a failed read or write, or a wrong-direction call. |

`fgets(buf, n, fp)` reads at most `n-1` characters, stops after a newline (keeping
it), and returns `NULL` if nothing was read. A line longer than the buffer is
returned in pieces; this is tested with a 200-character line and a 64-byte buffer.

## Deviations from C89 (deliberate)

1. `setvbuf` and `setbuf` do not accept a caller buffer (C89 requires them to).
   They fail (or have no effect) instead. The library's buffer is fixed at 256 bytes.
2. `fflush` on an input stream returns 0 and does nothing. C89 leaves it undefined.
3. `ungetc` supports one character of pushback; C89 requires at least one.
4. `SEEK_END` is not supported.
5. `errno` is not set. Failure is reported only by the return value and the
   error flag.
6. There are no write modes for files (see below).
7. `stdout` and `stderr` are one console channel; `stderr` is not a separate
   error stream.
8. Console input translates CR to LF (see Input).

C68 comparison (`libc-4.24.5/stdio`): C68 `fflush(NULL)` returns 0 without
flushing; this library flushes `stdout` and `stderr`. C68 `setvbuf` accepts a
caller buffer and `ungetc` allows more than one pushback; both are narrower here.
C68 code is 68000 and is not linked; see `docs/R11B-LIBC-MAP.md`.

## No file writes

`fopen` with any write mode returns `NULL`. There is no code path that writes a
WIN1_ file, and the test `STREAM` checks that `"w"`, `"a"` and `"r+"` fail. The
FAT16 volume hash is checked before and after every matrix run.

## Where text goes

`printf` with a newline reaches the console at once. Without a newline it waits
in `stdout`'s buffer until the next newline, a read from `stdin`, `fflush`, or
exit. Programs that write to the console through `qd_write` or `qd_puts`
directly (the kernel interface in `libqdos.a`) bypass the stdout buffer, so their
output can appear before earlier `printf` text that is still buffered. Use one
or the other within a program.
