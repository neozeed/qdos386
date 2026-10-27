# R11G: Task Force '87 interpreter on QDOS/386 (Zork I)

Status: the interpreter runs the real `zork1_z3` story from the FAT16 volume on Tiny386 and i386VM,
through the authentic opening, a 20-move sequence, QUIT, a relaunch and a second game, with no
reboot. Results: `docs/R11G-RUNTIME-REPORT.md`.

## What is run

- The Task Force '87 interpreter, unchanged except for the platform adaptation below, built as an
  ordinary slot-1 a.out (`OMAGIC`, entry `0x240000`, `_start` in `crt0_libc.s`) by the historical
  GCC 1.40 / GAS / i386-ld tree and the QDOS/386 SDK libraries (`libc.a`, `libm.a`,
  `libgcc-qd.a`, `libqdos.a`).
- The story is read from the read-only FAT16 volume through the `WIN1_` channel.
- Console input and output go through libc `stdin`/`stdout`, which the SDK maps to the CON channel.

Nothing is linked into the kernel, and no host-side emulation is involved.

## Platform changes (`third_party/infocom/r11g-platform.patch`)

The patch touches `infocom.c`, `support.c` and `file.c`. Items 1 to 3 below are the `infocom.c`
changes; item 4 is the error handling in `support.c` and `file.c`.

1. **QL layer removed.** `<qdos.h>`, `<sys/qlib.h>`, `<sms.h>` and `<fcntl.h>` are not in the SDK.
   The QL `t_osal_handle`, the window constants and `init_osal()` (which opened a QL `ut_con`
   window) are deleted. The call to `init_osal()` in `main` is deleted. Nothing else in the
   interpreter used them.
2. **Filename.** `open_file("win1_zork1_z3")` becomes `open_file("WIN1_ZORK1_Z3")`. The `WIN1_`
   channel prefix is matched case-sensitively (`kernel/channel.c`, `name_match`), so the original
   lowercase name fails with `CHAN_ERR_NAME`. The failure message is changed to name the file.
3. **Story check** (`qd_story_ok`, called before `init()`). The original reads the header and then the
   story blocks without checking the file size, so a truncated file is read as garbage until an
   opcode error repeats forever. The check:
   - counts the file by reading it to end of file (the SDK's `fseek(SEEK_END)` is refused: the kernel
     has no file-size query);
   - rejects a file shorter than 32 bytes: `not a Z-code story file (N bytes)`;
   - rejects a Z-code version other than 3: `unsupported Z-code version N`;
   - rejects a file shorter than the header's length (word 26 x 2 bytes):
     `story file is truncated: N bytes, header needs M`.
   A file longer than the header declares is accepted: the game reads only the declared length.
   The original's own `read_header` version check remains, but the message is now produced first.
4. **Internal errors are fatal** (`support.c` `error()`, and the three I/O error paths in `file.c`).
   The original `error()` prints `Internal Error #NN`, calls `quit()` and returns. `quit()` only sets
   the status, so the interpreter loop carries on, runs the same bad opcode again, and prints the
   message forever (tested: a valid-header story with junk code flooded the console until the harness
   pipe broke). `error()` now terminates the process with `exit(1)` after `quit()` has closed the story
   file and released memory. The same applies to the `Failed to Seek` and `Failed to read File Header`
   paths in `file.c`. The normal `QUIT` path still returns to the shell.
5. Nothing else changed. The Z-machine logic, parser, object model, text decoding and PRNG are the
   original code.

## Filename on the volume

The interpreter opens `WIN1_ZORK1_Z3`. The `WIN1_` driver strips its prefix and the FAT16 root is
searched for `ZORK1_Z3`: eight characters, no extension. `kernel/fat16.c` `fat16_name83` accepts that
form (base up to 8 characters, extension optional), and the volume entry is `ZORK1_Z3` with a blank
extension. No underscore-to-dot conversion is involved and none was added.

`tools/make-fat16.py` previously required an extension, so it could not place `ZORK1_Z3`. Its name
check now makes the extension optional. The change affects only the host-side image tool.

## Build

`scripts/r11g-build.sh <SDK> <historical tree> <outdir>`:

- sources: `third_party/infocom/orig` (copied and CRLF-stripped, a build-only transform), then the
  platform patch applied to the LF copies (it fails the build if it does not apply);
- `-O` for every module. The makefile's `-Os` is refused by the SDK wrapper, which accepts only `-O`;
- host headers hidden: the build runs in a private mount namespace (`unshare -m`) where
  `/usr/include`, `/usr/local/include` and the GCC include directory are empty tmpfs. The historical
  `cccp` has no `-nostdinc`, and it otherwise falls back to glibc headers. The script first checks
  that a glibc-only header (`sys/cdefs.h`) is NOT found, and aborts if it is;
- link: `crt0_libc.s`, the 16 objects, `libm.a`, `libc.a`, `libgcc-qd.a`, `libqdos.a`, with
  `ld/slot1.ld` (`-N`, origin `0x240000`).

The makefile is not used: its `clean` target is DOS-only (`del`), and its link line (`ld ... -lgcc -lc -lgcc -lm`)
is not the QDOS/386 link. The QDOS/386 link is in `scripts/r11g-build.sh`.

## Memory and stack (measured on the final image)

| Item | Value |
|---|---|
| `text` / `data` / `bss` | 29,840 / 1,456 / 4,672 bytes |
| image total (text+data+bss) | 35,968 bytes (35.1 KiB) |
| slot 1 (linker script `0x240000`..`0x27FFFF`) | 256 KiB; headroom 226,176 bytes |
| game working buffer (`malloc`, `io.c` `alloc_mem`) | 24,576 bytes; holds the resident area (header `resident_bytes`, 19,284 for Zork I), the 1 KiB Z-stack and the print/status buffers |
| story image | 86,838 bytes, read through a 512-byte page cache |

The SDK README says slot 1 holds "about 1 MiB"; `docs/R5-APP-ABI.md` and the linker script say
256 KiB. This build fits under the stricter figure, and the discrepancy is recorded for the owner.

C stack: the job stack is fixed at 4 KiB (`docs/R1-RUNTIME-REPORT.md`) with no overflow detection.
The Z-machine stack is inside the 24 KiB buffer, not on the C stack. C recursion occurs only in
`obtree()` (the `-t` tree dump, not reachable in play). The C call depth in play is shallow;
this was not instrumented.

## Integer widths and byte order

- Story words are decoded explicitly, high byte first: `get_word` and `next_word` in `support.c`
  shift the first byte left by 8. The result does not depend on host endianness.
- `word`, `signed_word` and `byte` keep their unsigned/signed widths. `int` is 32-bit on i386.
- The host reference (LP64, native `stdio`) is built from the same patched source. It produces the
  same transcript as the i386 port, which is evidence that the 16-bit and 32-bit arithmetic agree.

## Behaviour that differs from a desktop interpreter

- **SAVE / RESTORE**: out of scope. `save` asks for a filename, and `restore` asks for one too;
  both then print `Failed.`. The game keeps running and the shell is unaffected. Nothing is written
  to the volume.
- **QUIT**: the interpreter returns from `main`. Its `quit()` stores the status in a global and does
  not call `exit`. `main` has no `return`, so the process exit status is whatever `main` leaves in
  the return register: the shell shows `exit status 0` after a QUIT, `34` after the missing-story
  message, and other values after a failed story. This is a property of the original source and is
  left unchanged. It does not affect the game, and the shell returns to its prompt either way.
- **Random seed**: `seed_random` calls `time()`. Without a clock `time()` returns `(time_t) -1` and
  the seed is `0xFFFF`, as in the original's fallback. The no-clock image therefore plays
  deterministically. A clocked image would seed differently and change random events.
- **Output**: the status line and text wrapping are the interpreter's own, written through the
  libc stream. The console is 80 columns.
- **Input**: `getchar()` in the SDK waits about one tick per poll, so the scheduler is not held.

## Known limits

- SAVE, RESTORE, transcripts and writable game files are not implemented.
- The story is not included in the repository or package (licence not established for this asset;
  see `third_party/infocom/PROVENANCE.md`).
- The interpreter source is included only as the build input; its licence is not established.
