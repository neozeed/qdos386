# QDOS/386 R8 - SDK foundation and libc inventory

Status: the SDK foundation is built and runs on both targets (see
`R8-RUNTIME-REPORT.md`).  The libc inventory is read-only and reports what the
attached archive contains and what it does not.

## 1. What R8 ships as the SDK

| Path | What |
|---|---|
| `sdk/include/qdos386.h` | the application interface: `qtrap`, trap numbers, `qd_*` wrappers |
| `sdk/lib/crt0.s` | startup: QT_APP_ARGS, `main(argc, argv)`, QT_EXIT (copy of `apps/common/app_start.s`) |
| `sdk/lib/qtrap.s`, `sdk/lib/qdos.c`, `sdk/lib/Makefile` | `libqdos.a` (the trap entry and the `qd_*` wrappers) |
| `sdk/ld/slot0.ld`, `sdk/ld/slot1.ld` | application link scripts, origins 0x200000 and 0x240000 |
| `sdk/bin/qdos386-gcc` | the build wrapper (GCC 1.40, GAS 1.38, i386-ld 1.94) |
| `sdk/examples/` | `hello`, `args`, `stream`, `stress`, built only with the wrapper |

Nothing in the SDK changes the kernel, the slot layout or the historical tools.

## 2. The wrapper

    QDOS386_TOOLS=/path/to/tree qdos386-gcc [-slot 0|1] [-O] [-I dir] [-D name] -o out.aout src.c ...

- `QDOS386_TOOLS` must name the historical tree (`bin/i386-gcc`, `bin/as`,
  `bin/i386-ld`).  There is no default path.
- Default slot is 1.  `-slot 0` links for slot 0.
- Refused: `-m*`, `-std=c99` and later, `-fpic`/`-fPIC`, `-shared`, `-pie`,
  `-static`, `-nostdlib`, `-nostartfiles`, `-g`, unknown options, and non-`.c` inputs.
  These are refused because GCC 1.40 cannot honour them and a silent difference
  would produce a wrong image.
- Compiles with `-ansi` (C89).  Only freestanding C89 is supported.

## 3. What the SDK does not promise

- No `stdio.h`, `printf`, `FILE`, `malloc`, `calloc`, `realloc`, `getenv`, or
  any hosted-library behaviour.
- `qd_alloc` / `qd_free` are QDOS heap calls: not zeroed, no realloc, NULL on
  failure, shared with the kernel.  They are not described as `malloc`.
- No C99, no C++, no 64-bit `long long` assumptions (not tested).
- No libm.  The historical compiler's own helpers are used as they are.

## 4. libqdos.a and the archiver

The historical tree has no `ar`.  `sdk/lib/Makefile` uses the host `ar` to build
`libqdos.a` from the two historical objects.  That is an assumption the runtime
tests check: the examples link against `libqdos.a` and run.  If a future host
`ar` changes the archive format, the SDK build must be revisited.  Recorded as a
limitation in the runtime report.

## 5. Inventory of the attached libc: `libc-4.24.5.tar.bz2`

Read-only.  The archive was extracted to a scratch directory and not modified.

What it is: the **C68 `LIBC_A` standard C library** for the **68000 QL** (QDOS)
platform.  Its Makefile header says it builds "the C68 LIBC_A Standard C
library using the qdos-gcc cross-compiler", with a change history entry of
September 2000 "Adaptation to qdos-gcc cross-compiler" and a math change of
1 October 2000.  It is therefore a 68k library, not the GCC 2.95.3 bundled C
library that the R8 plan asked to inventory.  That archive was not supplied.

Directory summary (C sources, headers, assembly files):

| Directory | C | H | asm | Content |
|---|---|---|---|---|
| `include/` | 0 | 79 | 0 | headers: stdio, stdlib, string, unistd, signal, time, qdos, sms, rll, ... |
| `stdio/` | 97 | 9 | 0 | stdio implementation (FILE, printf family) |
| `stdlib/` | 30 | 0 | 7 | malloc, calloc, environment, conversions |
| `string/` | 17 | 0 | 10 | string functions |
| `ctype/`, `locale/`, `time/` | 18, 1, 12 | 0 | 0, 0, 2 | ctype, locale, time |
| `unistd/`, `init/`, `defaults/`, `misc/` | 35, 10, 30, 7 | 0 | 8, 6, 10, 2 | `sbrk` (via `lsbrk`), `_exit`, startup and defaults |
| `signal/`, `setjmp/` | 39, 0 | 0 | 0 | signals (kill, raise) and setjmp (empty here) |
| `fcntl/`, `stat/`, `dirent/`, `pwd/`, `grp/`, `libgen/` | 18, 11, 6, 3, 3, 22 | 0 | 3, 1, 0, 0, 0, 0 | POSIX-style file and directory calls over QDOS |
| `qdos/` | 49 | 0 | 131 | QDOS-specific: trap1/trap2/trap3 wrappers, pointers, misc |
| `lattice/`, `things/` | 21, 2 | 0 | 0, 11 | memory-pool and sort routines in the Lattice style (`allmem`, `bldmem`, `dqsort`); SMS and hotkey assembly (`smsfthg.s`, `hotkey.s`) |
| `lib68k/`, `libieee/`, `precomp/` | 2, 0, - | 1, 1 | 17, 59 | 68000 helpers and IEEE floating point (assembly) |

Notable facts:

- **Architecture: 68000 throughout.**  Assembly files are 68k; the QDOS traps
  are `trap #1`, `trap #2`, `trap #3` on the 68000 (the `qdos/trap*` directories).
  Nothing in the archive can run on the i386 without a port.
- **Heap:** `malloc` is built on `lsbrk`, which is the QDOS break routine.  Its
  implementation is not the R8 heap; it would need to be reimplemented over
  `QT_MEM_ALLOC` (or a sbrk-style region in the slot) for R9.
- **Exit and startup:** `_exit` and `_RLL_exit` assume the QDOS job model and its
  return conventions; the R8 model (`main(argc, argv)`, `QT_EXIT`) is similar in
  intent but not in calling convention.
- **File I/O:** the POSIX-style layer goes through QDOS channels (`qdos/`), with
  68k register conventions.  The R8 channel API (`QT_CH_*`) is the replacement.
- **stdio:** large (97 files).  It is the most useful piece to port, but it
  depends on malloc, the file layer and the 68k conventions above.
- **Licence:** there is no licence text at the top of the tree.  Individual
  files carry their own notices (for example `lib68k/Xalloca16.s` "Copyright
  1991, Christoph van Wuellen"; `qdos/qptr/ioppick.s` "Copyright 1992, HaPeSu").
  Any port must check each file's notice; the archive as a whole is not
  licensed in a single statement.
- **Math:** `libieee/` is IEEE floating-point assembly for 68k.  The
  "qdos-gcc specific math functions" are selected by `-DGCC_MATH` in the Makefile.

Reusable with the least work: the C-only string and conversion functions
(`string/`, parts of `stdlib/`, `ctype/`), after a check of their dependencies.
Not reusable as they stand: anything in `qdos/`, `lib68k/`, `libieee/`, the
startup code, `malloc`/`sbrk`, `signal/` and the channel layer.

## 6. Staged libc strategy (proposal for R9)

1. Obtain the actual GCC 2.95.3 QDOS distribution and its bundled library, if it
   exists, and inventory it the same way.  Open question: which C library it
   ships, and whether it is compatible with the R8 startup and trap ABI.
2. Define the R9 interface first: `malloc` over a dedicated heap region (not
   `QT_MEM_ALLOC` directly), `_exit` over `QT_EXIT`, channels for stdio, `argv`
   from R8.  Write it in C89, freestanding, with tests in the R8 style.
3. Port in layers: `string` and `ctype` (pure C), then `stdlib` conversions, then
   `malloc`, then stdio over channels, then math last.
4. Keep SuperBASIC out of the critical path.  Its C89 build is a size probe until
   the library is settled.

## 7. TODO (missing inputs)

- The GCC 2.95.3 QDOS distribution, with its bundled C library: **not supplied**.
  The inventory above is of the 68k C68 library only.
- The SuperBASIC/C89 archive, for dependency sizing: **not supplied** in R8.
  No size probe was built.
