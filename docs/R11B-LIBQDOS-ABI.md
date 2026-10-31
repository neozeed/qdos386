# R11B libqdos ABI

`libqdos.a` holds the QDOS/386 system wrappers (`sdk/lib/qdos.c`) and the trap
entry (`sdk/lib/qtrap.s`). It is separate from `libc.a` (C89) and from the
compiler runtime (`gnulib`/libgcc). R11B adds no new wrapper: the existing
interface was enough for every R11B test. The header is `sdk/include/qdos386.h`.

## Wrappers

| Group | Functions | Trap |
|---|---|---|
| Program control | `qd_exit`, `qd_args`, `qd_ticks`, `qd_yield` | 0x18, 0x1F, 0x20, 0x08 |
| Heap | `qd_alloc`, `qd_free`, `qd_free_bytes`, `qd_heap_ok` | 0x10, 0x11, 0x12, 0x13 |
| Channels | `qd_open`, `qd_close`, `qd_write`, `qd_putc`, `qd_read`, `qd_seek` | 0x14, 0x15, 0x16, 0x17, 0x1A, 0x1B |
| Console | `qd_puts` | (open CON, write, close) |

The trap entry is `qtrap(fn, a, b, c)`: EAX = function, EBX = a, ECX = b,
EDX = c, result in EAX. Portable C sources do not contain `int 0x30`: a search
of `sdk/libc`, `sdk/include` and `sdk/examples/r11b` finds none. The only trap
instructions are in `crt0.s`, `crt0_libc.s` and `qtrap.s`, which is where they
belong.

## Errors and return values

- Negative returns are the kernel's codes (`CHAN_ERR_*`, `MEM_ERR_*`, `FAT_ERR_*`),
  passed through unchanged. `CHAN_ERR_AGAIN` (-19) means "no data yet"; stdio
  uses it to yield and retry on `stdin`.
- `qd_read` returns 0 at the end of a channel.
- `qd_alloc` returns `NULL` on failure; `qd_free` returns 0 on success.
- The wrappers do not set `errno`.

## Stale and invalid handles (tested)

`LIBQDO.AOU` checks, with the guest's own assertions:

- read, close and seek on a closed channel fail with a negative code;
- a negative channel number read fails;
- opening a missing WIN1_ file fails;
- two channels on the same file are distinct and independent;
- `qd_free` of a block already freed fails (negative), and the heap is still
  valid afterwards (`qd_heap_ok` returns 1);
- an oversized `qd_alloc` (0x7fffffff bytes) returns `NULL` and the heap stays valid;
- freeing a block returns exactly the bytes it took (`qd_free_bytes`);
- `qd_ticks` advances across `qd_yield`.

## Ownership and cleanup

- **Channels** belong to the job that opened them. `stdio` keeps one CON
  channel for the life of the job (it is opened by the first console write or
  read). `fclose` closes a file channel. The kernel closes everything a job
  holds when it exits.
- **Heap blocks** from `qd_alloc` and `malloc` belong to the job and are
  reclaimed at exit. `malloc` adds a 16-byte header with a magic word, so a
  second `free` is detected and ignored, and counted in `qd_bad_frees`.
- **The console channel's heap cost.** The first console write opens CON, and
  the kernel keeps about 80 bytes of heap for it until the job exits. A program
  that measures heap use must open the console first (`REPEAT.AOU` does).
- `REPEAT.AOU` runs 40 rounds of open, read, close and malloc/free; the heap
  returns to its starting value and the channel table keeps working. It is run
  three times in one boot.

## What is not provided

- **Writing files.** There is no write wrapper and the kernel has no write path
  to FAT16. Opening a file for writing fails in libc (`fopen` returns `NULL`).
- **Names longer than 10 characters.** A WIN1_ name must fit the kernel's
  16-byte channel name buffer (`WIN1_` + name), so names are at most 10
  characters with the dot. Lifting this is a kernel change. It was not made in
  R11B, because it is not a small change and the kernel is outside the R11B
  scope.
- **Directory listing, file size, timestamps.** Not in the trap ABI.

## Unchanged

Trap numbers, the application startup ABI (`crt0.s`, argc/argv at
`QT_APP_ARGS`), the child slot size (1 MiB, slot 1), and the kernel's error
codes are unchanged from R11A.
