# R11A C68 libc port map

Reference: `libc-4.24.5.tar.bz2` (C68 LIBC_A 4.24.5), extracted read-only to
`scratchpad/libc/libc-4.24.5` (not in this repository). The tree has no licence
file; see the open questions in `R11A-RUNTIME-REPORT.md`.

The C68 tree is a 68000 library: its files are C with `CSTRING` switches, `.x`
files (C with an optional assembler alternative), `.s` files (68000 assembler),
and macros such as `_LIB_F1_` and `__LIBRARY__`. For R11A it is used as the
**semantic reference** (which classes exist, EOF handling, pushback rules, search
and sort contracts). No file is copied; each function is written for this SDK.

## Inventory of the directories consulted

| Directory | .c | .x (C + asm alternative) | .s (68000 asm) |
|---|---|---|---|
| ctype | 18 | 0 | 0 |
| stdlib | 30 | 3 | 7 |
| string | 17 | 22 (21 with `CSTRING`) | 10 |
| stdio | 97 | 0 | 0 |
| libieee | floating-point support, 68000 assembler (`GY*`, `Y*`, `X*`, `frexp.s`, `ldexp.s`) | | |

## Functions in R11A

| Function | C68 reference | R11A status | R11A source | Classification |
|---|---|---|---|---|
| isalnum, isalpha, iscntrl, isdigit, isgraph, islower, isprint, ispunct, isspace, isupper, isxdigit, tolower, toupper | `ctype/*.c` (table `_ctype`) | **new** | `sdk/libc/ctype.c` | 68k table replaced by ASCII range tests (C locale); EOF and out-of-range values false |
| qsort | `stdlib/_qsort.x` (recursive quicksort), `qsort.s` (wrapper) | **new** | `sdk/libc/stdlib.c` | Replaced by **heapsort**: O(n log n), no recursion, fixed stack use; not stable (C89 does not require stability) |
| bsearch | `stdlib/bsearch.c` | **new** | `sdk/libc/stdlib.c` | Portable C, rewritten: same contract, returns NULL on miss or size 0 |
| strtol, strtoul | `stdlib/strtol.c`, `_strtoul.c`, `strtoul.s` | **kept from R9** | `sdk/libc/stdlib.c` | Not replaced; base 0/16 prefixes, clamping, and end pointer tested by STRTST |
| atoi, atol, abs, labs | `stdlib/atoi.c`, `atol.c`, `abs.c`, `labs.c` | kept from R9 | | |
| strspn, strcspn | `string/strspn.x`, `strcspn.x` (C with `CSTRING`) | **new** | `sdk/libc/string.c` | Portable C, rewritten |
| strpbrk | `string/strpbrk.x` | **new** | `sdk/libc/string.c` | Portable C, rewritten |
| strtok | `string/strtok.x` | **new** | `sdk/libc/string.c` | Static state, not reentrant (as C89 permits); empty tokens never returned |
| strchr, strrchr, strstr | `string/strchr.x`, `strrchr.x`, `strstr.x` | kept from R9 | | |
| strerror | `string/strerror.x` | **deferred** | | No `errno` in the SDK; see limits |
| fgetc, getchar, fread, fseek, ftell, feof, ferror, clearerr | `stdio/*.c` | kept from R9 | `sdk/libc/stdio.c` | Read-only WIN1_ channels; `fseek` has no `SEEK_END` (no file size query) |
| ungetc | `stdio/ungetc.c` (writes into a buffer) | **new** | `sdk/libc/stdio.c` | Unbuffered: one character of pushback in the FILE structure; `ftell` agrees with the next read; `fseek` discards it |
| getc | `stdio/getc.c` | **new** | `sdk/libc/stdio.c` | Calls `fgetc` (no macro) |
| fgets | `stdio/fgets.c` | **new** | `sdk/libc/stdio.c` | Uses `fgetc`; keeps the newline; returns NULL when nothing was read |
| sprintf | `stdio/sprintf.c`, `_f_sprintf.c` | kept from R9, **contract documented** | | Unbounded, as C89; the caller must size the buffer |
| snprintf, vsnprintf | (C99; `_f_sprintf.c` is unbounded) | **new, labelled C99 extension** | `sdk/libc/stdio.c` | Shared formatting core with a bound; writes at most size-1 characters and a NUL; returns the full length |
| printf family with floating point, scanf family | `stdio/_f_*`, `scanf.c`, `_f_scanf.c` | **not in R11A** | | Depends on `libieee` and on the 68000 float conversions; see `R11A-MATH-ROADMAP.md` |
| fopen for writing, fputs, fprintf | `stdio/fopen.c`, `fputs.c`, `fprintf.c` | **not in R11A** | | FAT16 is read-only |
| setjmp, signal, time, locale | `setjmp/`, `signal/`, `time/`, `locale/` | **not in R11A** | | Outside the selected subset |

## Classification summary

- **Portable C, rewritten for this SDK** (the C68 source is the reference): ctype,
  bsearch, strspn, strcspn, strpbrk, strtok, ungetc, getc, fgets, snprintf.
- **Adapted, with a deliberate change of algorithm**: qsort (heapsort).
- **Kept from R9, not changed**: strtol family, atoi/atol/abs/labs, string basics,
  stdio read path.
- **Blocked or deferred**: strerror (no errno), floating-point formatting and
  parsing (`libieee` is 68000 assembler), writing files (FAT16 is read-only),
  locale/time/signal/setjmp (outside the selected subset).

## Kernel services used

Only the existing R9 traps: `qd_open`, `qd_read`, `qd_seek`, `qd_close`,
`qd_write`, `qd_yield`, `qd_exit`, and the heap in `malloc.c`. **No kernel service
was added** in R11A. The C68 `lsbrk` and `_memqdos` dependencies are not used.

## Symbol coverage

Checked on `sdk/lib/libc.a` (names include the leading underscore, see
`R11A-LIBC-ABI.md`): `_isalpha`, `_qsort`, `_bsearch`, `_strspn`, `_strtok`,
`_ungetc`, `_fgets`, `_snprintf` are defined. All the other functions listed above
are in the archive from R9 or are new in R11A.

Totals for the C89 headers: `ctype.h` (14 functions, all defined);
`stdlib.h` new: qsort, bsearch; `string.h` new: strspn, strcspn, strpbrk, strtok;
`stdio.h` new: ungetc, getc, fgets, snprintf, vsnprintf.
