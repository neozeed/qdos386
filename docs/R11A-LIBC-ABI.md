# R11A libc ABI

Target: QDOS/386 native i386 a.out applications (OMAGIC 0407), linked by
`sdk/bin/qdos386-gcc` with the historical GCC 1.40 / GAS 1.38 / i386-ld tree.

## Calling convention and symbols

- cdecl: arguments on the stack, right to left; the caller removes them; the
  return value in `EAX`; `long long` is not used; `double` returns are not used
  in R11A (see the math roadmap).
- Every C symbol has a **leading underscore** (`_main`, `_isalpha`, `_qsort`).
  Checked on `sdk/lib/libc.a`. Assembler routines in `crt0.s` and `qtrap.s` use
  the same convention.
- `int` and pointers are 32 bits; `long` is 32 bits (checked by the STRTST
  overflow tests, `LONG_MAX` and `LONG_MIN` clamps).
- `size_t` is `unsigned int`, the historical GCC 1.40 type.
- Link order: `crt0.o`, the application objects, `libc.a`, `libqdos.a`
  (`qdos386-gcc` does this; `libc.a` is read only for the symbols it needs).

## Headers (SDK-owned)

- `<ctype.h>`: 14 functions, no macros, EOF and values outside 0..127 are false.
- `<stdio.h>`: `FILE` is opaque; `stdin`, `stdout`, `stderr` are the CON channel.
  Read-only files on `WIN1_`. The filename limit is `FILENAME_MAX` = 16 including
  the `WIN1_` prefix and the NUL. **Names must be at most 10 characters with the
  dot**, so `WIN1_` plus the name fits the 16-byte kernel channel-name buffer.
- `<stdlib.h>`, `<string.h>`: as listed in the port map.
- `<stdarg.h>`: SDK-owned, GCC 1.40 compatible.

## Contracts (what callers may rely on)

| Function | Contract |
|---|---|
| sprintf | Unbounded. The caller must provide a buffer large enough for the output. |
| snprintf, vsnprintf | **C99 extension**. Writes at most `size-1` characters and a NUL when `size > 0`; writes nothing when `size == 0`; returns the length the complete output would have had. |
| ungetc | One character. A second call before a read returns EOF. `ftell` after `ungetc` is one less. `fseek` discards the pushed character. |
| fgets | Reads at most `n-1` characters, stops after a newline (kept), NUL-terminates; NULL when nothing was read. |
| strtok | Not reentrant. A NULL first argument continues the previous string. |
| qsort | Heapsort: not stable; `nmemb` 0 or 1 and `size` 0 are no-ops. |
| bsearch | Returns a pointer into the array, or NULL. `size` 0 returns NULL. |
| fopen | Read only: `"r"`, `"rb"`. Anything else returns NULL. |
| fseek | `SEEK_SET`, `SEEK_CUR`. `SEEK_END` returns -1 (no file size query in the kernel). |
| errno | **Not provided.** |

## Resource ownership

- `malloc` blocks belong to the job and are reclaimed at exit with its open channels.
- `fopen` allocates a `FILE` on the heap and opens a channel; `fclose` closes the
  channel and frees the `FILE`. FILETEST opens and closes the same file 20 times
  and then reads it again, checking that channels and blocks do not accumulate.
- The standard streams are never closed by `fclose`.
