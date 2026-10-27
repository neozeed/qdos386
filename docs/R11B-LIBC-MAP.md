# R11B libc map

What each R11B library function is, where its C68 counterpart lives, and what
was done. C68 is the historical 68000 LIBC_A 4.24.5, read from the attached
archive (`libc-4.24.5`, 523 C and header files). None of its 68000 code is
linked. Behaviour was compared with its sources; the code was written for this
SDK against the C89 standard and the QDOS/386 trap ABI.

Classification used below:

- **Portable C**: the C68 source is plain C and could compile unchanged.
- **Adapted**: the C68 logic was reproduced, with QDOS/386 or i386 changes.
- **Written here**: no usable C68 counterpart; written for this SDK.
- **Not provided**: C89 function not implemented in R11B (reason given).

## Functions changed or added in R11B

| Function | C68 source | Class | Notes |
|---|---|---|---|
| `fputc`, `putc`, `putchar` | `stdio/fputc.c` | Written here | Buffered through `stream_putc`. |
| `fputs`, `puts` | `stdio/fputs.c`, `stdio/puts.c` | Adapted | C68 writes into a write-ahead buffer; here the line buffer is 256 bytes and `_IONBF` writes at the end of the call. |
| `fwrite` | `stdio/fwrite.c` | Written here | Returns 0 unless every byte was written. |
| `fprintf`, `vfprintf`, `printf`, `vprintf` | `stdio/fprintf.c`, `stdio/vfprintf.c`, `stdio/_vfprintf.c`, `stdio/_f_vfprintf.c` | Written here | One formatter core (`qd_vformat`) for every form; C68's float conversions are not present (see `R11A-MATH-ROADMAP.md`). |
| `sprintf`, `vsprintf` | `stdio/sprintf.c`, `stdio/vsprintf.c` | Adapted | Unbounded, as C89. |
| `snprintf`, `vsnprintf` | not in C68 | Written here | **C99 extension, not C89.** |
| `fflush` | `stdio/fflush.c` | Adapted | C68 returns 0 for `fflush(NULL)` without flushing; here it flushes `stdout` and `stderr`. |
| `setvbuf`, `setbuf` | `stdio/setvbuf.c`, `stdio/setbuf.c` | Adapted | Caller buffers are refused (C68 accepts them). |
| `ungetc` | `stdio/ungetc.c` | Adapted | One character of pushback (C68 allows more). |
| `fgetc`, `getc`, `getchar` | `stdio/fgetc.c`, `stdio/getchar.c` | Adapted | Console input yields while waiting; CR becomes LF. |
| `fgets` | `stdio/fgets.c` | Adapted | Keeps the newline, as C89. |
| `fread` | `stdio/fread.c` | Adapted | File streams only; honours pushback. |
| `fseek`, `ftell`, `rewind` | `stdio/fseek.c`, `stdio/ftell.c`, `stdio/rewind.c` | Adapted | `SEEK_END` not provided (no file size in the kernel). |
| `feof`, `ferror`, `clearerr` | `stdio/feof.c`, `stdio/ferror.c`, `stdio/clearerr.c` | Adapted | Sticky flags. |
| `fopen` | `stdio/fopen.c`, `stdio/_fopen.c` | Adapted | Read modes only; write modes return `NULL`. |
| `fclose` | `stdio/fclose.c` | Adapted | Standard streams are kept open. |
| `exit` | `stdio/exit.c`, `unistd/_exit.c` | Adapted | Flushes stdio, then the exit trap. Return from `main` also flushes (`crt0_libc.s`). |
| `qsort` | `stdlib/qsort.s` (68000), `stdlib/_qsort.x` (C variant) | Written here | Heapsort, no recursion, no allocation. Stable order is not guaranteed (as C89). |
| `bsearch` | `stdlib/bsearch.c` | Adapted | |
| `strtok`, `strspn`, `strcspn`, `strpbrk` | `string/strtok.x`, `string/strspn.x`, `string/strcspn.x`, `string/strpbrk.x` | Adapted | `strtok` is not reentrant (C89). |
| `strerror`, `strcoll`, `strxfrm` | `string/strerror.x`, `string/strcoll.x`, `string/strxfrm.x` | **Not provided** | No errno; the locale is C only. |

Functions carried from R9 and R11A (`malloc`, `calloc`, `realloc`, `free`, the
`str*` and `mem*` functions, `strtol`, `strtoul`, `atoi`, `atol`, `abs`, `labs`,
`bsearch`, the `ctype` classes) are unchanged in R11B. Their R11A coverage is in
`docs/R11A-C68-PORT-MAP.md`.

## C89 coverage

Standard headers as C89 defines them. "Yes" means implemented in libc.a; the
test column names the guest test that exercises it.

**`<stdio.h>`**

| Function | Status | Test |
|---|---|---|
| clearerr | yes | STREAM |
| fclose | yes | STREAM |
| feof | yes | STREAM |
| ferror | yes | STREAM |
| fflush | yes | FORMAT |
| fgetc | yes | CONSOL, STREAM |
| fgetpos | **no** | (fpos_t; not needed by any R11B test) |
| fgets | yes | STREAM, CONSOL, REPEAT |
| fopen | yes | STREAM, REPEAT |
| fprintf | yes | FORMAT |
| fputc | yes | FORMAT |
| fputs | yes | FORMAT |
| fread | yes | STREAM |
| freopen | **no** | |
| fscanf | **no** | scanf family not provided (no input conversions) |
| fseek | yes | STREAM |
| fsetpos | **no** | |
| ftell | yes | STREAM |
| fwrite | yes | FORMAT |
| getc | yes | STREAM |
| getchar | yes | CONSOL |
| gets | **no** | unsafe; not provided |
| perror | **no** | needs errno |
| printf | yes | FORMAT, all programs |
| putc | yes | FORMAT |
| putchar | yes | (uses fputc) |
| puts | yes | FORMAT |
| remove | **no** | no write path |
| rename | **no** | no write path |
| rewind | yes | STREAM |
| scanf | **no** | |
| setbuf | yes (NULL only) | FORMAT |
| setvbuf | yes (NULL buffer only) | FORMAT |
| sprintf | yes | FORMAT |
| sscanf | **no** | |
| tmpfile | **no** | no write path |
| tmpnam | **no** | no write path |
| ungetc | yes (one character) | STREAM, CONSOL |
| vfprintf | yes | (library) |
| vprintf | yes | (library) |
| vsprintf | yes | FORMAT |

**`<stdlib.h>`**

| Function | Status | Note |
|---|---|---|
| abort | **no** | |
| abs | yes | R9 |
| atexit | **no** | |
| atof | **no** | floating point: R11C |
| atoi, atol | yes | R9/R11A |
| bsearch | yes | R11A |
| calloc, free, malloc, realloc | yes | R9 |
| div, ldiv | **no** | |
| exit | yes | flushes stdio (R11B) |
| getenv | **no** | no environment |
| labs | yes | R9 |
| mblen, mbstowcs, mbtowc, wcstombs, wctomb | **no** | no multibyte or wide characters |
| qsort | yes | R11A |
| rand, srand | **no** | |
| strtod | **no** | floating point: R11C |
| strtol, strtoul | yes | R11A |
| system | **no** | |

**`<string.h>`**

| Function | Status | Test |
|---|---|---|
| memchr, memcmp, memcpy, memmove, memset | yes | R9 |
| strcat, strchr, strcmp, strcpy, strcspn, strlen, strncat, strncmp, strncpy, strpbrk, strrchr, strspn, strstr, strtok | yes | R11A STRINGS |
| strcoll, strerror, strxfrm | **no** | see above |

**`<ctype.h>`**

isalnum, isalpha, iscntrl, isdigit, isgraph, islower, isprint, ispunct,
isspace, isupper, isxdigit, tolower, toupper: all yes (C locale). R11A CTYPE.

**Not in R11B:** `<math.h>` (libm), `<time.h>`, `<locale.h>`, `<setjmp.h>`,
`<signal.h>`, `<errno.h>`, `<limits.h>` beyond the constants already present,
`<assert.h>`, `<float.h>`, `<wchar.h>`, `<wctype.h>`. None is a C89 requirement
this SDK meets yet, and none is claimed.

## Gaps against the plan

- `getchar` was linked in R11A but not run; R11B runs it (CONSOL).
- `strtod`/`atof`: not implemented. The C68 floating-point code is 68000
  assembler (`libieee`), and the decision belongs to R11C (`R11A-MATH-ROADMAP.md`).
- `strerror` and `errno`: not implemented; no error codes are mapped.
- File streams are allocated with `malloc`; the three standard streams are static.

## Exported symbols

Generated by `scripts/r11b-symbols.py` from the C sources (non-static
function definitions). Regenerate after any change to `sdk/libc` or `sdk/lib/qdos.c`.

| Symbol | Library | Source |
|---|---|---|
| `isupper` | libc.a | sdk/libc/ctype.c |
| `islower` | libc.a | sdk/libc/ctype.c |
| `isdigit` | libc.a | sdk/libc/ctype.c |
| `isalpha` | libc.a | sdk/libc/ctype.c |
| `isalnum` | libc.a | sdk/libc/ctype.c |
| `iscntrl` | libc.a | sdk/libc/ctype.c |
| `isprint` | libc.a | sdk/libc/ctype.c |
| `isgraph` | libc.a | sdk/libc/ctype.c |
| `ispunct` | libc.a | sdk/libc/ctype.c |
| `isspace` | libc.a | sdk/libc/ctype.c |
| `isxdigit` | libc.a | sdk/libc/ctype.c |
| `tolower` | libc.a | sdk/libc/ctype.c |
| `toupper` | libc.a | sdk/libc/ctype.c |
| `malloc` | libc.a | sdk/libc/malloc.c |
| `calloc` | libc.a | sdk/libc/malloc.c |
| `free` | libc.a | sdk/libc/malloc.c |
| `realloc` | libc.a | sdk/libc/malloc.c |
| `qd_flush_std` | libc.a | sdk/libc/stdio.c |
| `vfprintf` | libc.a | sdk/libc/stdio.c |
| `fprintf` | libc.a | sdk/libc/stdio.c |
| `vprintf` | libc.a | sdk/libc/stdio.c |
| `printf` | libc.a | sdk/libc/stdio.c |
| `sprintf` | libc.a | sdk/libc/stdio.c |
| `vsnprintf` | libc.a | sdk/libc/stdio.c |
| `snprintf` | libc.a | sdk/libc/stdio.c |
| `fputc` | libc.a | sdk/libc/stdio.c |
| `putc` | libc.a | sdk/libc/stdio.c |
| `putchar` | libc.a | sdk/libc/stdio.c |
| `fputs` | libc.a | sdk/libc/stdio.c |
| `puts` | libc.a | sdk/libc/stdio.c |
| `fwrite` | libc.a | sdk/libc/stdio.c |
| `fflush` | libc.a | sdk/libc/stdio.c |
| `setvbuf` | libc.a | sdk/libc/stdio.c |
| `setbuf` | libc.a | sdk/libc/stdio.c |
| `fgetc` | libc.a | sdk/libc/stdio.c |
| `getc` | libc.a | sdk/libc/stdio.c |
| `getchar` | libc.a | sdk/libc/stdio.c |
| `ungetc` | libc.a | sdk/libc/stdio.c |
| `fgets` | libc.a | sdk/libc/stdio.c |
| `fopen` | libc.a | sdk/libc/stdio.c |
| `fclose` | libc.a | sdk/libc/stdio.c |
| `fread` | libc.a | sdk/libc/stdio.c |
| `fseek` | libc.a | sdk/libc/stdio.c |
| `ftell` | libc.a | sdk/libc/stdio.c |
| `feof` | libc.a | sdk/libc/stdio.c |
| `ferror` | libc.a | sdk/libc/stdio.c |
| `clearerr` | libc.a | sdk/libc/stdio.c |
| `strtol` | libc.a | sdk/libc/stdlib.c |
| `strtoul` | libc.a | sdk/libc/stdlib.c |
| `atoi` | libc.a | sdk/libc/stdlib.c |
| `atol` | libc.a | sdk/libc/stdlib.c |
| `abs` | libc.a | sdk/libc/stdlib.c |
| `labs` | libc.a | sdk/libc/stdlib.c |
| `exit` | libc.a | sdk/libc/stdlib.c |
| `qsort` | libc.a | sdk/libc/stdlib.c |
| `bsearch` | libc.a | sdk/libc/stdlib.c |
| `memcpy` | libc.a | sdk/libc/string.c |
| `memmove` | libc.a | sdk/libc/string.c |
| `memset` | libc.a | sdk/libc/string.c |
| `memcmp` | libc.a | sdk/libc/string.c |
| `memchr` | libc.a | sdk/libc/string.c |
| `strlen` | libc.a | sdk/libc/string.c |
| `strcmp` | libc.a | sdk/libc/string.c |
| `strncmp` | libc.a | sdk/libc/string.c |
| `strcpy` | libc.a | sdk/libc/string.c |
| `strncpy` | libc.a | sdk/libc/string.c |
| `strcat` | libc.a | sdk/libc/string.c |
| `strncat` | libc.a | sdk/libc/string.c |
| `strchr` | libc.a | sdk/libc/string.c |
| `strrchr` | libc.a | sdk/libc/string.c |
| `strstr` | libc.a | sdk/libc/string.c |
| `strdup` | libc.a | sdk/libc/string.c |
| `strspn` | libc.a | sdk/libc/string.c |
| `strcspn` | libc.a | sdk/libc/string.c |
| `strpbrk` | libc.a | sdk/libc/string.c |
| `strtok` | libc.a | sdk/libc/string.c |
| `qd_exit` | libqdos.a | sdk/lib/qdos.c |
| `qd_args` | libqdos.a | sdk/lib/qdos.c |
| `qd_ticks` | libqdos.a | sdk/lib/qdos.c |
| `qd_yield` | libqdos.a | sdk/lib/qdos.c |
| `qd_alloc` | libqdos.a | sdk/lib/qdos.c |
| `qd_free` | libqdos.a | sdk/lib/qdos.c |
| `qd_free_bytes` | libqdos.a | sdk/lib/qdos.c |
| `qd_heap_ok` | libqdos.a | sdk/lib/qdos.c |
| `qd_open` | libqdos.a | sdk/lib/qdos.c |
| `qd_close` | libqdos.a | sdk/lib/qdos.c |
| `qd_write` | libqdos.a | sdk/lib/qdos.c |
| `qd_putc` | libqdos.a | sdk/lib/qdos.c |
| `qd_read` | libqdos.a | sdk/lib/qdos.c |
| `qd_seek` | libqdos.a | sdk/lib/qdos.c |
| `qd_puts` | libqdos.a | sdk/lib/qdos.c |

Total: 91 exported functions (76 in libc.a, 15 in libqdos.a).
