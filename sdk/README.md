# QDOS/386 SDK (R11E)

R11E makes `libqdos.a` a documented native service library and extends the
filename limit to the full 8.3 spelling.  It does not change the C89 library
except for the fixes listed in `docs/R11E-RUNTIME-REPORT.md`.

New in R11E:
- `qd_*` wrappers (declared in `include/qdos386.h`): job identity and information
  (`qd_self`, `qd_job_slot`, `qd_job_info`, `qd_job_activate`, `qd_job_chprio`,
  `qd_job_reactivate`, `qd_job_suspend`, `qd_job_exit_status`, `qd_job_reclaimed`),
  `qd_run`, `qd_con_wait`, heap with failure codes (`qd_alloc_ex`, `qd_mem_validate`),
  `qd_getc`, and the WIN1_ root directory (`qd_dir_open`, `qd_dir_next`).
- `qd_exit` and `exit` end the job by unwinding to the entry frame; a program no
  longer has to return from `main` after `exit`.
- Names: `FILENAME_MAX` is 18, so a full 8.3 name (12 characters with its dot) opens
  through `WIN1_`.  A 13-character name is refused.
- Two new kernel traps: `QT_JOB_SELF` (0x21) and `QT_JOB_SLOT` (0x22).  The lower
  bound of user pointers is the heap arena base.

Read first:
- `docs/R11E-QDOS-API-MAP.md` (C68 cross-reference and what is mapped)
- `docs/R11E-LIBQDOS-ABI.md` (wrappers, error codes, traps, exit unwind)
- `docs/R11E-KNOWN-LIMITATIONS.md` (what is not done or not verified)
- `docs/R11E-C89-COVERAGE.md` / `.csv` (per-function status and guest tests)
- `docs/R11E-RUNTIME-REPORT.md` (what was run, where, and the results)
- `docs/R11E-LICENSE-PROVENANCE.md` (new components and their notices)

Build and run: `scripts/r11e-volume.sh [LGTEST4.aout]` builds the volume;
`scripts/r11e-run-all.sh` runs the scripted Tiny386 and i386VM matrix;
`scripts/r11e-qemu-smoke.sh` runs the scripted QEMU VGA smoke test;
`sh tests/host/r11e/run.sh` builds and runs the host-side C89 unit tests.

Tests (`examples/r11e/`): `QDOSAPI` (job, heap, channel, directory and error
checks), `PS` (job listing), `C89EDGE` (edge cases of the untested libc
functions), `STREAM` and `REPEAT` (extended), `CHILD1`, `CHILD2`, `CONWAIT`,
`EXITX`, `EXITR` (programs run from the shell to test exit and status).

---

# QDOS/386 SDK (R11D, unchanged notes)

R11D extends the C89 library: string, ctype, stdlib (`qsort`, `bsearch`,
`strtol`, `rand`, `div`, `atexit`, `abort`), `setjmp`/`longjmp` (native i386),
`assert`, `locale` (C locale only), `signal` (fails: no delivery), `time`
(struct tm and formatting only; the calendar clock is absent), the `scanf`
family, `perror`/`strerror`, and `math.h` transcendental functions
(`exp log log10 sin cos tan asin acos atan atan2 sinh cosh tanh fmod`).
File-modifying APIs (`remove`, `rename`, `tmpfile`, `tmpnam`, write-mode
`fopen`) fail explicitly.  Nothing is a success-returning stub.

Read first:
- `docs/R11D-C89-COVERAGE.md` / `.csv` (per-function status and test evidence)
- `docs/R11D-BLOCKED-APIS.md` (every blocked or limited API and why)
- `docs/R11D-LIBC-PORT-MAP.md` (what came from where)
- `docs/R11D-LIBM-MAP.md` (methods and measured accuracy)
- `docs/R11D-LIBQDOS-ABI.md` (libqdos: not expanded in R11D)
- `docs/R11D-RUNTIME-REPORT.md` (what was run, where, and the results)

Build and run: `scripts/r11d-volume.sh [LGTEST4.aout]` builds the volume;
`scripts/r11d-run-all.sh` runs the scripted Tiny386 and i386VM matrix;
`scripts/r11d-qemu-smoke.sh` runs the scripted QEMU VGA smoke test.

Coverage of each function by a guest test is recorded per symbol in
`docs/R11D-C89-COVERAGE.csv`; several functions have no test yet.

Tests (`examples/r11d/`): `C89COR` (83 checks), `JMPTST` (19), `SCANTS` (33,
reads "3 4" from stdin), `MATHX` (24, with per-function ulp reporting).

---

This directory builds native applications for QDOS/386.  R11C adds a floating
point C89 runtime: `strtod` and `atof` (correctly rounded), `%f %e %g` in the
printf family, `float.h`, `errno.h`, `math.h` with `floor ceil fabs frexp ldexp
modf sqrt pow`, and the GCC 1.40 conversion helpers in a separate archive,
`lib/libgcc-qd.a`.  `long double` is 8 bytes (it is `double`).

Read first:
- `docs/R11C-FORMAT-ABI.md` (float types, conversions, printf rules, limits)
- `docs/R11C-LIBM-MAP.md` (function list, `pow` accuracy and its measured bounds)
- `docs/R11C-SUPERBASIC-DEPENDENCIES.md` (provisional: SB3 is not attached)
- `docs/R11C-LICENSE-PROVENANCE.md` (new components and their notices)
- `docs/R11C-RUNTIME-REPORT.md` (what was run, where, and the results)

## What R11C adds

- `compiler/`: builds `lib/libgcc-qd.a` from the historical GCC 1.40 gnulib
  sources plus the emx `fixdfsi.s`.  Linked after `libm.a` and `libc.a`, which may reference it.
- `libc/strtod.c`, `libc/fpbig.c`: exact decimal conversion with bignums.
- `libc/stdio.c`: `%f %e %E %g %G`, flags, width, precision, `l h L` modifiers.
- `libm/`: the functions above, and `xmath.s` (x87 kernels).
- `include/float.h`, `include/errno.h`, `include/math.h`; `stdlib.h` gains
  `strtod` and `atof`.
- Tests in `examples/r11c/`: `HELPER.aout` (compiler helpers), `FPARSE.aout`
  (strtod table), `FMATH.aout` (libm tables), `FPRINT.aout` (printf table),
  `FPCOMB.aout` (combined program), `FPMULT.aout` (two jobs with live x87 state).
  The tables are generated by `scripts/r11c-tables.py`.  Run them with
  `scripts/r11c-run-all.sh` (scripted Tiny386 and i386VM) and
  `scripts/r11c-qemu-smoke.sh` (scripted QEMU VGA).

## Limits added in R11C

- `long double` is `double`. No 80-bit type.
- `pow` is not correctly rounded (1 ulp on QEMU, several hundred ulp in the
  worst case on the emulators that model x87 in double). See the LIBM map.
- No `exp`, `log`, trigonometry, `NaN` or infinity inputs, `%a`, `%n`.
- `strtod` accepts only the C89 decimal grammar; no hexadecimal or infinity.
- `errno` is a plain global, set only by the functions documented above.
- SuperBASIC is not integrated; its dependency list is provisional.


This directory builds native applications for QDOS/386.  R11B adds buffered C89
stdio: `fprintf`, `fputs`, `fputc`, `putc`, `fwrite`, `fflush`, `setvbuf`,
`setbuf`, `vsprintf`, `rewind`, with `stdout` line buffered and `stderr`
unbuffered, flushed at exit.  The Enter key is read as a newline.  The R11A
library (`ctype`, `qsort`, `bsearch`, more string functions, `ungetc`, `fgets`,
`getc`, a bounded `snprintf`) is unchanged.  The R9 notes below still apply.

Read first:
- `docs/R11B-STDIO-ABI.md` (stream model, buffering, deviations)
- `docs/R11B-LIBQDOS-ABI.md` (wrappers, errors, ownership)
- `docs/R11B-LIBC-MAP.md` (C68 mapping, C89 coverage table, symbol inventory)
- `docs/R11B-LICENSE-PROVENANCE.md` (licence status; distribution deferred)
- `docs/R11A-LIBC-ABI.md` and `docs/R11A-C68-PORT-MAP.md` for the R11A base.

## What R11B adds

- `libc/stdio.c`: a per-stream state model (direction, buffer mode, output
  buffer, pushback, sticky flags).  Output to the console is buffered; file
  streams are read only.  One formatter core serves `printf`, `fprintf` and the
  string forms.
- `libc/stdio.c`, `libc/stdlib.c`, `lib/crt0_libc.s`: `exit()` and the return
  from `main` flush stdio.  `bin/qdos386-gcc` links `crt0_libc.s` when `libc.a`
  is present.
- `lib/Makefile` and `examples/Makefile`: each example depends on `libc.a`,
  `libqdos.a` and the startup sources, so changing a library relinks the examples.
- Tests in `examples/r11b/`: `STREAM.aout`, `FORMAT.aout`, `CONSOL.aout`,
  `LIBQDO.aout`, `REPEAT.aout`, `EXITFL.aout`.  Run them with
  `scripts/r11b-run-all.sh`.  Names are at most 10 characters with the dot.

## Limits added in R11B

- Console input: the Enter key sends CR; `stdin` returns it as LF.
- Caller-supplied stdio buffers are refused by `setvbuf` and `setbuf`.
- `fflush` on an input stream returns 0 and does nothing.
- `ungetc` supports one character.  `SEEK_END` is not supported.
- No file writes.  `fopen` with a write mode returns `NULL`.
- `stdout` and `stderr` share the one console channel.
- Not provided: `scanf`, `strtod`, `atof`, `strerror`, `errno`, `getenv`,
  `rand`, `system`, `atexit`, `div`, `freopen`, `tmpfile`.  See
  `docs/R11B-LIBC-MAP.md` for the full C89 coverage table.

## R11A (unchanged notes)

R11A extends the R9 C89 subset with `ctype.h`, `qsort`, `bsearch`, more string functions, `ungetc`,
`fgets`, `getc` and a bounded `snprintf`.
Read `docs/R11A-LIBC-ABI.md` and `docs/R11A-C68-PORT-MAP.md`.

## What R11A adds

- `include/ctype.h`, `libc/ctype.c`: classification and case mapping, C locale.
- `stdlib.h`: `qsort` (heapsort; not stable), `bsearch`.
- `string.h`: `strspn`, `strcspn`, `strpbrk`, `strtok` (not reentrant).
- `stdio.h`: `ungetc` (one character), `fgets`, `getc`, and `snprintf` and
  `vsnprintf` (C99, labelled).  `sprintf` keeps its unbounded C89 contract.
- Tests: `examples/r11a/` builds `CTYPE.aout`, `STRTST.aout` and `FTEST.aout`.
  Run them with `scripts/r11a-run-all.sh` and `scripts/r11a-volume.sh`.

## Limits added in R11A

- **File names on the volume are at most 10 characters with the dot.**  The
  channel name buffer is 16 bytes including `WIN1_`, so `STRINGS.AOU`,
  `FILETEST.AOU` and `LIBHELLO.AOU` cannot be opened or run.  The R11A tests are
  `STRTST.AOU`, `FTEST.AOU` and `LIBHEL.AOU`.  Changing this is a kernel change.
- No `errno`, so no `strerror`.
- No floating-point printf, scanf, or libm (see `docs/R11A-MATH-ROADMAP.md`).
- `FILENAME_MAX` is 16, with `WIN1_` included.

---

## R9 SDK (unchanged notes)

R9 added the C89 library subset `libc.a` to the R8 SDK.  Read
`docs/R9-LIBC-ABI.md` and `docs/R9-LIBC-PORT-MAP.md` for the R9 contract.

## What R9 adds

- `lib/` builds `libc.a` from `libc/` (`string`, `malloc`, `stdlib`, `stdio`).
  It is archived with the host `ar`; the GCC 1.40 compiler and i386-ld are the
  historical ones.
- `include/`: `stdio.h`, `stdlib.h`, `string.h`, `stddef.h`, `stdarg.h`,
  `limits.h`, alongside `qdos386.h`.  `stdarg.h` is SDK-owned.
- `bin/qdos386-gcc` links `libc.a` (when built) before `libqdos.a`.
- Examples: `libhello/` (LIBHEL.AOU, argc/argv), `catfil/` (CATFIL.AOU, reads a
  FAT16 file through `fopen`/`fread`), `memstr/` (MEMSTR.AOU), `heapex/`
  (HEAPEX.AOU, allocator reclaim), `bigapp/` (BIGAPP.AOU, 479,904 bytes, runs in
  slot 1 only; `bigapp/table.c` is generated by `bigapp/gen_table.py`).

## Build

    export QDOS386_TOOLS=/path/to/the/historical/tree
    make -C sdk/libc                             # libc.a
    make -C sdk/lib                              # libqdos.a
    make -C sdk/examples                         # builds the examples (*.aout are not tracked)

Keep `libc.a` and `libqdos.a` separate.  A program that uses `printf` links
both; the wrapper handles the order.

## Limits

- Read-only `fopen` (`"r"` and `"rb"`); `SEEK_SET` and `SEEK_CUR` only.
- `printf`: `%d %i %u %x %X %o %c %s %%`, flags `-` and `0`, decimal width.
  No precision, no floating point.
- `getchar` polls `CON` with `qd_yield(1)`; it is not yet tested at run time.
- Heap blocks are owned by the job and reclaimed at exit, together with the
  job's open channels.
- Slot 1 holds images up to about 1 MiB; slot 0 holds up to 224 KiB.

## R8 SDK foundation (unchanged)

This directory is the minimal, stable way to build a native application for
QDOS/386 R8.  It is **not** a C library.  Read `docs/R8-APP-ABI.md` and
`docs/R8-SDK-INVENTORY.md` first.

## Build a program

    export QDOS386_TOOLS=/path/to/the/historical/tree   # bin/i386-gcc, as, i386-ld
    make -C sdk/lib                                      # libqdos.a (needs host ar)
    sdk/bin/qdos386-gcc -o MYPROG.AOU myprog.c           # slot 1 by default
    sdk/bin/qdos386-gcc -slot 0 -o MYPROG0.AOU myprog.c  # slot 0

Put the `.AOU` file on the WIN1_ volume and run it from the shell:
`RUN MYPROG.AOU one "two words"`.

File names on the volume are 8.3 names, and the whole name, dot included, is at
most 10 characters (for example `SDKHEL.AOU`).  The limit comes from the channel
name buffer: `WIN1_` plus the name must fit in 16 bytes (`CHAN_NAME_MAX`,
including the terminating NUL).  Changing it is a kernel change, not an SDK one.

## What is in the box

- `include/qdos386.h` - the interface: `qd_exit`, `qd_args`, `qd_ticks`,
  `qd_yield`, `qd_alloc` / `qd_free` / `qd_free_bytes` / `qd_heap_ok`,
  `qd_open` / `qd_close` / `qd_write` / `qd_putc` / `qd_read` / `qd_seek`,
  `qd_puts`.  Plus `qtrap` and the trap numbers.
- `lib/crt0.s` - startup: `QT_APP_ARGS`, `main(argc, argv)`, `QT_EXIT`.
- `lib/qtrap.s`, `lib/qdos.c` - the trap entry and the wrappers, in `libqdos.a`.
- `ld/slot0.ld`, `ld/slot1.ld` - link scripts for the two application slots.
- `bin/qdos386-gcc` - the build wrapper.
- `examples/` - `hello` (SDKHEL), `args` (SDKARG), `stream` (SDKSTM, reads
  `WIN1_README.TXT`), `stress` (SDKSTS, heap and yield).  Built only with the
  wrapper.  Their 8.3 names are what they carry on the R8 volume.

## What it does not do

No `stdio.h`, no `printf`, no `malloc` (the heap calls are QDOS-specific and
are not zeroed), no `FILE`, no C99, no libm.  See `docs/R8-SDK-INVENTORY.md`
for the libc that was supplied and why it is not used in R8.

## Limits worth knowing

- `qdos386-gcc` refuses options it cannot honour (`-m*`, `-std=c99` and later,
  `-fpic`, `-shared`, `-pie`, `-static`, `-nostdlib`, `-g`, and others).  See
  the header of `bin/qdos386-gcc`.
- Applications have 224 KiB of text + data + BSS, a 16 KiB stack, and read
  arguments only through `argv`.
- The kernel heap is shared and is not inside the application's slot.
