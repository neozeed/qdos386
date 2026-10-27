# QDOS/386 R9 - a 1 MiB child slot, 16 MiB i386VM, and a C89 libc subset

R9 keeps the R8 kernel and shell and adds:

- **Memory.**  The child slot (slot 1) is 1 MiB, so an application of up to
  about 1 MiB (text + data + BSS) can run while the shell stays resident in
  slot 0 (224 KiB limit).  The i386VM harness can be built with 16 MiB of guest
  RAM through an isolated patch, `patches/i386vm-r9-ram16.patch`; Tiny386 stays
  at 8 MiB and the Tiny386 core is not modified.  The R5-R8 default map is kept
  unchanged for the regression.  See `docs/R9-MEMORY-AUDIT.md`.
- **libc.a.**  A separate C89 library (`sdk/libc/`): malloc/free/calloc/realloc,
  string and memory functions, printf/puts/putchar/getchar, exit, and read-only
  fopen/fread/fseek/ftell/fclose.  It is kept apart from `libqdos.a`.  See
  `docs/R9-LIBC-PORT-MAP.md` and `docs/R9-LIBC-ABI.md`.
- **Applications run as QDOS jobs.**  `BIGAPP` (479,904 bytes, above the slot 0
  limit), `MEMSTR`, `CATFIL` (FAT16 read through the C library), `LIBHEL`
  (argc/argv), `HEAPEX` (allocator reclaim at exit).  The R9 volume holds 23
  entries.  See `docs/R9-RUNTIME-REPORT.md`.

Verification: `scripts/r9-matrix.sh` with `ROUNDS=3` passed on both targets:
R8 regression and R9 suites, scripted and PS/2 (`-term`) sessions, the EOF
session and the timer-off negative.  The archived R8 artifacts also still pass
on their own harness.  The matrix used input that was scripted through the
harness or through `session.py`; no human keyboard testing was done.

## R9 build and run

    export I386VM=/path/to/i386vm-tree           # bin/i386-gcc, as, i386-ld, stock i386vm
    # 16 MiB i386VM harness: stock source + patches/i386vm-input-status.patch
    #   + patches/i386vm-r9-ram16.patch, then:
    cc -O2 -DI386_ENABLE_FPU -fwrapv -fno-strict-aliasing -DRAM_MB=16 \
       emulator-tiny386/i386.c emulator-tiny386/fpu.c emulator-tiny386/machine.c -lm
    make r9-fat16                                # R9 volume (build/fat16/win1-r9.img)
    make r9-i386vm-shell r9-pc-shell             # R9 shell images
    sh scripts/r9-matrix.sh                      # ROUNDS=3 by default

The R9 shell image on i386VM expects the 16 MiB harness; the R8 default images
expect the 8 MiB harness.

## Known limitations

- The shell banner still reads "QDOS/386 R8 shell".  Not changed in R9.
- libc is read-only for files; there is no `SEEK_END`, no writes, and no
  `fprintf`, `fgets`, `fputs` or `scanf`.  `getchar` is linked but has no
  runtime test.  Floating point is not tested.
- Tiny386 runs 8 MiB only.  The 16 MiB RAM configuration is an i386VM patch.
- Timer preemption is observed by one check in `BIGAPP`, not by a dedicated
  test.  The timer-off negative is a weak check (budget stop, not a clean halt).
- The per-slot untouched-memory check in `tests/test_app.c` still samples only
  the first 256 KiB of slot 1.
- The i386VM harness source is not in the repository; only the patches are.

Other R9 documents: `docs/R9-MEMORY-AUDIT.md`, `docs/R9-LIBC-PORT-MAP.md`,
`docs/R9-LIBC-ABI.md`, `docs/R9-RUNTIME-REPORT.md`, `sdk/README.md`.

# QDOS/386 R8 - larger applications, argv, and an SDK foundation

R8 keeps the R7 kernel and shell and adds three things:

- **A streaming a.out loader.**  An application is validated from its file
  before any slot is touched, then copied into slot 1 in 512-byte chunks.
  Text + data + BSS may be up to 224 KiB (`LARGE.AOU` is about 175 KiB).
  See `docs/R8-LOADER-DESIGN.md`.
- **A startup and argument ABI.**  `main(int argc, char **argv)`, arguments
  from `RUN <name> "quoted words"` (15 user arguments, 432 bytes), and
  `QT_APP_ARGS` / `QT_TICKS` traps.  See `docs/R8-APP-ABI.md`.
- **An SDK foundation, not a libc.**  Headers, crt0, `libqdos.a`, slot link
  scripts, the `qdos386-gcc` wrapper and four examples.  See `sdk/README.md`
  and `docs/R8-SDK-INVENTORY.md` (which also audits the attached C68 libc).

Other R8 documents: `docs/R8-AUDIT.md` (Phase 0), `docs/R8-RUNTIME-REPORT.md`
(results, limits, and what was not tested).

## R8 build and run

    export I386VM=/path/to/i386vm-tree           # provides bin/i386-gcc, as, i386-ld, i386vm
    make i386vm pc apps fat16                    # R2-R7 images and the R7 volume
    make r8-fat16 sdk-examples                   # R8 volume (builds r8-apps and the SDK samples)
    make i386vm-r8-shell pc-r8-shell             # R8 shell images on the R8 volume
    sh scripts/r8-matrix.sh                      # ROUNDS=3 by default

- `build/fat16/win1-r8.img` is the R8 volume (17 files; `DIR` shows 18 with
  `CHAIN.AOU`).  The R7 volume `build/fat16/win1.img` is kept for the R2-R7
  regression and is not modified by the R8 build.
- `build/i386vm-r8/qdos386-r8-shell.bin` is the i386VM R8 shell image (its
  FAT16 is `win1-r8.img`).  `build/pc-r8/r8shell.img` is the Tiny386 R8 shell
  image, with `run.ini` in the same directory.
- The SDK samples are built by `make sdk-examples` (it runs `sdk/lib` and
  `sdk/examples` with `QDOS386_TOOLS=$(I386VM)`); see `sdk/README.md`.

The matrix runs the R2-R8 kernel tests and the R8 shell session on both
emulators, scripted (i386VM pipe) or through the PS/2 path of Tiny386 driven by
`tests/interactive/session.py`.  These are not human keyboard tests.

---

# QDOS/386 R7 - a diagnostic shell on the native kernel

R7 adds a small shell. At a `> ` prompt a user types `HELP`, `DIR`, `TYPE`,
`RUN` or `EXIT`. `DIR` and `TYPE` read the WIN1_ FAT16 volume, and `RUN` loads an
a.out program from it into slot 1, waits for it, and prints its exit status.
The shell stays running across commands, and the console is read through a
blocking wait that never spins.

Input on the PC is the emulated i8042 keyboard (set-1 scan codes, IRQ1). On
i386VM it is a status register added to the harness (`patches/`). Both paths
are described in `docs/R7-CONSOLE-ABI.md`.

Documents:

- `docs/R7-CONSOLE-ABI.md`: keyboard path, input polling, EOF, CON channel,
  `QT_CON_WAIT`, `QT_IMG_LOAD`, error codes, the directory channel, name limits.
- `docs/R7-SHELL-DESIGN.md`: boot, commands, waiting, RUN, self-test, limits.
- `docs/R7-RUNTIME-REPORT.md`: what was built, how it was verified, results,
  hashes, decisions, corrections, and what has not been tested.
- `docs/R7-AUDIT.md`: the Phase 0 audit.

Verification: `scripts/r7-matrix.sh` (three rounds on both emulators, plus the
EOF and timer-off negatives). The sessions are scripted (i386VM pipe) or
driven through the PS/2 path by `tests/interactive/session.py`. They are not
human-interactive tests; the report says so.

## R7 build and run

    make i386vm pc apps fat16 i386vm-shell pc-shell I386VM=... TINY386=... SEABIOS=...
    sh scripts/r7-matrix.sh

The shell images are `build/i386vm-shell/qdos386-r7-shell.bin` (needs the
patched harness) and `build/pc-shell/r7shell.img` (`hdb` is `build/fat16/win1.img`).
The default `make i386vm pc` images are the R2–R7 regression build.

---

# QDOS/386 R6 - native kernel that loads a.out programs from a FAT16 disk

R6 adds read-only FAT16 files. A WIN1_ volume is mounted from the platform disk
(Tiny386 IDE primary slave on the PC; a preloaded RAM disk on i386VM), and a job
opens `WIN1_HELLO.AOU` through the R4 channel manager, reads it with
`QT_CH_READ`, loads it with the unchanged R5 a.out loader, and runs it as an
ordinary preemptive job. The guest prints the R6 lines only after those real
file operations. R2–R5 behaviour and the R5 transfer area are kept.

Targets: the i386VM harness and the Tiny386 PC. Both read the same FAT16 bytes.
See docs/R6-ABI.md (channel ABI), docs/R6-FAT16-FORMAT.md (volume),
docs/R6-RUNTIME-REPORT.md (results and limits) and docs/R6-AUDIT.md (the audit that
preceded the design).

## R6 build and run

    make i386vm pc apps fat16 I386VM=... TINY386=... SEABIOS=...
    scripts/run-all.sh        # three runs per target, plus the timer-off negative

`build/fat16/win1.img` is the 3 MiB volume (tools/make-fat16.py). It is attached
as `hdb` for the PC and embedded at 0x2C0000 in the i386VM image. Its SHA-256
is in SHA256SUMS and must not change during a run.

## Earlier releases

# QDOS/386 R5 - native kernel that loads and runs independently linked a.out programs
## Status

Verified in the guest on both targets, three runs each (`docs/R5-RUNTIME-REPORT.md`):

- i386VM and Tiny386 PC: 103 checks ok, 1 n/a, 0 FAIL; `QDOS/386 R4 CHANNEL PASS`
  and `QDOS/386 R5 A.OUT EXEC PASS`.
- i386VM with the timer disabled: no PASS (instruction limit).

Documents:

- `docs/R5-AOUT-FORMAT.md`: the a.out dialect, from bytes (Phase 0).
- `docs/R5-APP-ABI.md`: memory map, linking, startup, exit, traps, transfer area,
  loader and application contracts.
- `docs/R5-RUNTIME-REPORT.md`: results, checks, limits, external dependency hashes.
- `docs/R4-CHANNEL-ABI.md`, `docs/R3-MEMORY-ABI.md`: earlier ABIs, unchanged.

## Layout

```
kernel/
  appmap.h               R5 memory map (slots, transfer area, heap end)
  aout.h, aout.c         transfer-area reader and OMAGIC loader
  main.c                 boot, R2-R4 phases, R5 application phase, report
  jobs.h, jobs.c         job table, scheduler, traps (incl. QT_EXIT 0x18, QT_JOB_EXIT 0x19)
  memory.h, memory.c     native heap
  channel.h, channel.c   channel manager and CON driver (R4)
  irq.h, irq.c           IRQ7 and IRQ15 classification (R4)
  context.s              IRQ stubs, yield, resume, software int helpers
  descriptors.c          GDT, IDT
tests/
  test_jobs.c/h, test_memory.c/h, test_channels.c/h   R2-R4 checks
  test_app.c/h           R5 application phase and malformed-image tests
apps/
  hello/hello.c          channel output, exit status 7
  prep/prep.c            data, BSS, stack, preemption, exit status 42
  common/app_start.s     _start (calls main, QT_EXIT, returns)
  common/qtrap.s         C wrapper for int 0x30
  common/slot0.ld        link script, origin 0x200000
  common/slot1.ld        link script, origin 0x240000
  probe-layout/          Phase 0 probe (format audit evidence)
platform/
  pc/                    boot sector (INT 13h), kernel link, image builder, console, PIC/PIT
  i386vm/                console, synthetic timer, halt
tools/
  make-xfer.py           transfer area: directory + exact a.out bytes
  make-vm-image.py       i386VM image: kernel, zero padding to 0x280000, transfer area
scripts/
  run-all.sh             build both targets and run the matrix
logs/                    R5 matrix logs (logs/*.log, logs/pc-run*.txt), fresh-extraction
                         logs (logs/fresh-extraction/), R4 and R3 logs for comparison
docs/                    see Status
```

## Build and run

External inputs are not included. Set their locations:

- `I386VM`: the i386VM harness tree (`bin/i386-gcc`, `bin/as`, `bin/i386-ld`, `bin/i386vm`).
- `TINY386`: the `tiny386_headless` binary built from the unmodified Tiny386 tree.
- `SEABIOS`: a directory with `bios.bin` and `vgabios.bin` (the known-good pair in
  `docs/R5-RUNTIME-REPORT.md` section 7).

```
I386VM=... TINY386=... SEABIOS=... scripts/run-all.sh
```

Outputs:

- `build/apps/hello.aout`, `build/apps/prep.aout`: the two applications (OMAGIC).
- `build/xfer.bin`: the transfer area.
- `build/i386vm/qdos386-r5.bin`: flat image for the i386VM harness (2.6 MB).
- `build/i386vm/qdos386-r5-notimer.bin`: negative-test image.
- `build/pc/r5pc.img`: Tiny386 hard-disk image.

## Services

Trap `int 0x30`, EAX = function, EBX/ECX/EDX = arguments, EAX = result.

| Code | Service |
|---|---|
| `0x08`–`0x0F` | R2 job and scheduler services |
| `0x10`–`0x13` | R3 memory |
| `0x14`–`0x17` | R4 channel: open, close, write, putc |
| `0x18` | R5 `QT_EXIT`: record the status of the calling job |
| `0x19` | R5 `QT_JOB_EXIT`: read a job's recorded status |

## Not in R5

- Keyboard input, files, directories, serial, VGA, SuperBASIC, ELF loading.
- Relocation, dynamic linking, shared libraries, ZMAGIC/QMAGIC.
- Protection between applications; everything runs in ring 0.
- More than two application slots, or applications larger than 256 KiB.
