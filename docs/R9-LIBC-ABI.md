# R9 libc ABI

This describes what a program can rely on when it links `libc.a`. The kernel
side of each rule is named so it can be checked in the source.

## Start-up and exit

- `crt0.o` calls `QT_APP_ARGS` to get `argc` and the `argv` array, then calls
  `main(argc, argv)` with the cdecl convention of GCC 1.40 (see
  `docs/R8-APP-ABI.md`). `argc` counts `argv[0]`.
- `exit(status)` (libc `stdlib.c`) calls `qd_exit`, which issues `QT_JOB_EXIT`
  with the status. The shell prints `exit status N` for the job. Returning from
  `main` passes its return value to the same path.
- Exit reclaims the job's resources in `kernel/jobs.c` (`reclaim_dead`):
  1. `chan_close_owner(job)` closes every channel the job opened;
  2. `mem_reclaim_owner(job)` frees every heap block the job owns, including
     the channel objects and the `malloc` blocks.

  The R9 sessions check this by running `HEAPEX` twice (`heapex` and `whole`
  runs) and comparing the starting free-byte counts; they must match.

## Heap (`malloc`, `free`, `calloc`, `realloc`)

- Blocks are obtained with `QT_MEM_ALLOC(size)` and released with
  `QT_MEM_FREE`. The kernel tags each block with the owning job.
- Each block has a 16-byte header: `magic` (`MALL` while live, `DEAD` after
  free), `size`, and two words of padding. `free` checks the magic. A block that
  is not live, or that is freed twice, is not released; the event is counted in
  `qd_bad_frees`.
- `calloc` zeroes the block. `calloc` with a zero count or size returns `NULL`.
  `realloc(NULL, n)` is `malloc(n)`; `realloc(p, 0)` frees `p` and returns
  `NULL`.
- Out of memory returns `NULL`.

## Strings and memory

Pure C functions; no kernel calls. See `docs/R9-LIBC-PORT-MAP.md` for the list.

## Console (`putchar`, `puts`, `printf`, `getchar`)

- Output goes to the `CON` channel, opened lazily on the first output call
  (`con_channel()`). The first call therefore allocates a kernel channel object.
  Programs that measure their heap should print once before measuring. `HEAPEX`
  does this.
- `printf` formats into a 128-byte chunk and writes it through `qd_write`.
  Conversions: `%d`, `%i`, `%u`, `%x`, `%X`, `%o`, `%c`, `%s`, `%%`. Flags `-`
  (left) and `0` (zero pad), and a decimal field width. `l` and `h` are accepted
  and ignored, since int and long are both 32 bits. There is no precision, no
  `*` width, and no `%f`, `%e`, `%g`, `%p` or `%n`.
- `getchar` reads one byte from `CON`. When nothing is waiting, the read
  returns `CHAN_ERR_AGAIN` (-19); `getchar` calls `qd_yield(1)` (about one tick)
  and retries. At end of input the read returns 0 and `getchar` returns `EOF`.
- `CON` input is non-blocking. `QT_CON_WAIT` supports one waiter; a second
  waiter gets `CON_ERR_BUSY` (-50). The libc does not use `QT_CON_WAIT`.

## Files (read only)

- `fopen(name, "r" | "rb")` maps the name to a `WIN1_` channel name, so the
  caller's name must leave room for the prefix (the channel name limit is
  `CHAN_NAME_MAX` 16, including the prefix and the terminating NUL). Other modes
  return `NULL`.
- `fread(buf, size, count, fp)` reads in chunks of at most 256 bytes per
  `QT_CH_READ`. It returns the number of whole items read.
- `fseek(fp, off, SEEK_SET | SEEK_CUR)` uses `QT_CH_SEEK`. `SEEK_END` returns -1.
- `ftell` returns the position counter the libc keeps for the `FILE`.
- `feof` reports the end-of-file flag set by a short read; `ferror` reports
  errors from the channel layer; `clearerr` clears both.
- `fclose` issues `QT_CH_CLOSE`. A `FILE` that is not closed is closed at job
  exit by the kernel (the channel is owned by the job).
- Writes, `fprintf`, `fgets`, `fputs` and `scanf` are not part of R9.

## Kernel services used by the libc

| Service | Used by | Notes |
|---|---|---|
| `QT_APP_ARGS` | `crt0.o` | argc, argv |
| `QT_JOB_EXIT` | `exit` | status, then reclaim |
| `QT_MEM_ALLOC`, `QT_MEM_FREE` | `malloc`, `free` | owner-tagged heap |
| `QT_CH_OPEN`, `QT_CH_CLOSE` | `fopen`, `fclose`, `con_channel` | channel objects owned by the job |
| `QT_CH_WRITE`, `QT_CH_PUTC` | `printf`, `putchar`, `puts` | |
| `QT_CH_READ` | `fread`, `getchar` | at most 256 bytes per call |
| `QT_CH_SEEK` | `fseek` | |
| `qd_yield` (`sdk/lib/qdos.c`) | `getchar` | sleeps about the given number of ticks |

## What is not guaranteed

- No `errno`, no locale, no buffering modes (`setvbuf`), no `freopen`.
- A `FILE` is not reference counted; closing one `FILE` for a name does not
  affect another.
