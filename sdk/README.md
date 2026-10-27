# QDOS/386 R8 SDK foundation

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
