# R9 libc port map

`libc.a` is a separate archive from `libqdos.a`. `libqdos.a` remains the kernel
service veneer from R5–R8; `libc.a` (built from `sdk/libc/`) provides the C89
subset that programs link against. `qdos386-gcc` links `crt0.o`, the program
objects, `libc.a` (when present) and then `libqdos.a`.

## Reference and provenance

- The reference is the C68/QDOS libc 4.24.5 tree (`init`, `stdlib`, `string`,
  `stdio` with 97 sources, `unistd`, `fcntl`, `stat`, `dirent`, `lib68k`,
  `libieee`, `misc`, `qdos`). It was read as a behavioural reference for
  function names, argument conventions and return values.
- No 68k assembly was translated. Each function in `sdk/libc/` is written for
  the i386 cdecl ABI and the R9 kernel services, and was checked by the tests
  in `sdk/examples/` and by `tests/`.
- The reference tree's licence terms and its authorship were not reviewed in
  this round. Nothing from it that is not listed below is shipped. Any future
  family ported from it needs a licence review first.

## Implemented in R9

| File | Functions | Notes |
|---|---|---|
| `sdk/libc/malloc.c` | `malloc`, `free`, `calloc`, `realloc` | Blocks come from `QT_MEM_ALLOC`/`QT_MEM_FREE`; each block carries a 16-byte header (`MALL`/`DEAD` magic). Bad frees are counted in `qd_bad_frees`. |
| `sdk/libc/string.c` | `strlen`, `strcpy`, `strncpy`, `strcat`, `strncat`, `strcmp`, `strncmp`, `strchr`, `strrchr`, `strstr`, `strdup`, `memcpy`, `memmove`, `memset`, `memchr`, `memcmp` | Plain C; no alignment tricks. |
| `sdk/libc/stdlib.c` | `atoi`, `atol`, `strtol`, `strtoul`, `abs`, `labs`, `exit` | `exit` calls `qd_exit`, which uses `QT_JOB_EXIT`. |
| `sdk/libc/stdio.c` | `putchar`, `puts`, `printf`, `vprintf`, `sprintf`, `getchar`, `fgetc`, `fopen`, `fclose`, `fread`, `fseek`, `ftell`, `feof`, `ferror`, `clearerr` | `printf` is a single engine (`qd_vformat`) with a 128-byte output chunk. Input files are read in chunks of at most 256 bytes. |

Headers: `sdk/include/stdio.h`, `stdlib.h`, `string.h`, `stddef.h`, `stdarg.h`,
`limits.h`. `stdarg.h` is SDK-owned (the historical GCC 1.40 tree does not
supply it on the default path).

## Partly implemented

- `fopen` accepts only `"r"` and `"rb"`. Any other mode returns `NULL`.
- `fseek` supports `SEEK_SET` and `SEEK_CUR`. `SEEK_END` returns -1: the
  kernel has no file size query.
- `stdin`, `stdout` and `stderr` are not `FILE` objects; `putchar`, `puts` and
  `printf` write to the `CON` channel, and `getchar` reads from it.
- `getchar` polls the `CON` channel with `qd_yield(1)` between attempts. It does
  not use the single-waiter `QT_CON_WAIT` service (a second waiter gets
  `CON_ERR_BUSY`, so one polling reader is the safe pattern). It is compiled and linked but has no runtime test yet (see
  the runtime report).

## Deferred, by family

These families are present in the C68 reference and are not ported in R9. They
are listed at family level; no per-file classification was made, as the plan
does not require one.

- `unistd`, `fcntl`, `stat`, `dirent`: depend on file-system services that the
  kernel does not provide (directories, metadata, writes).
- `stdio` beyond the subset above: write modes, `fprintf`, `fgets`, `fputs`,
  `scanf` and its family, `setvbuf`/buffering modes, `freopen`, `tmpfile`.
- `stdlib` beyond the subset: `qsort`, `bsearch`, `rand`/`srand`, `div`,
  `ldiv`, `strtod`, `getenv`, `system`.
- `string` beyond the subset: `strtok`, `strspn`, `strcspn`, `strpbrk`,
  `strerror`, `strcoll`.
- `init`, `misc`, `qdos`: start-up code is provided by `crt0.o` (argc/argv per
  `docs/R8-APP-ABI.md`); the reference `init` routines are not used.
- `lib68k`, `libieee`: 68k and IEEE floating-point support. The i386 build
  uses the historical GCC floating-point code, not these routines. Floating
  point is not tested in R9.
- `errno`, locale, `time`, `signal`, `setjmp`: not provided.

## Status of the audit

- R9 classified the functions the plan named as priorities, and the families
  above at family level.
- No deferred family is needed by any R9 application.
