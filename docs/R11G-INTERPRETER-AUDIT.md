# R11G: interpreter and story audit

Final for R11G. Every item was checked on the commands and artefacts named here. Items not checked
are listed at the end.

## 1. Frozen base

- Branch `r11g` created from `r11f`, commit `d614fa69d070fba9acd12fca3965e8e96a4e9a4e`.
- The base FAT16 volume (`win1-r11f.img`), rebuilt from that commit with `scripts/r11f-volume.sh`,
  has SHA-256 `d319f3214efea391bc9c85d9048dac0bc5e77c42eeaf2d492fb9a1e820564375`. This matches the
  pre-run hash recorded in the R11F matrix log, so the frozen base reproduces.
- The `qdos386-R11F.tar.gz` file itself was not in the working area, so its archive hash is not
  checked here.

## 2. Story (`zork1_z3`)

- 86,838 bytes; SHA-256 `37084966477dff679282de42974b2077156b1bd68fad92a65d4ea94d8eb64d79`.
- Header (read with `od`): Z-code version 3; release 119; serial "880429"; high memory base 19,284;
  initial PC 20,693; dictionary 14,489; object table 1,254; globals 688; static base 11,282;
  abbreviations 496; file length word 43,419 (x2 = 86,838, matches the file size).
- Licence: NOT verified for this file. See `third_party/infocom/PROVENANCE.md`.

## 3. Interpreter source

- 16 modules, CRLF, copyright "(C)opyright 1987 InfoTaskforce.". No licence text in the archive.
  Archive SHA-256 `21ae323dd01b0ad4f30a6686a7cba40dc9135b1e7f1aec43c3c23f2810eae7ca`.
- The archive's `infocom.c` is a QL port (QL window, `ut_con`, `WINDOWDEF_t`, `sms.h`). It is the
  only module that does not build on QDOS/386.

## 4. Build results (before any change)

| Check | Result |
|---|---|
| unchanged CRLF sources through the SDK wrapper | fails at `file.c:4`: `#include` with trailing `\r` (CRLF boundary) |
| CRLF stripped (build-only), 15 modules | all compile, no diagnostics |
| CRLF stripped, `infocom.c` | fails on `qdos.h`, `sys/qlib.h`, `sms.h`, `fcntl.h` |
| link of the 15 modules | 45 undefined symbols, all globals or `main` defined in `infocom.c`; no libc/libm gap |

The SDK wrapper accepts only `-O`, not the makefile's `-Os`. The historical `cccp` has no
`-nostdinc`, and it falls back to glibc headers. The build therefore runs in a private mount
namespace, with a probe that aborts if a glibc-only header is visible.

## 5. Port (after changes)

See `docs/R11G-ZORK1-PORT.md` and `third_party/infocom/r11g-platform.patch` (56 lines removed,
3 added, plus the story check). Clean build with no warnings. Measured image: text 29,840, data
1,456, BSS 4,672 bytes; slot 1 headroom 226,176 bytes; game working buffer 24,576 bytes.

## 6. Filename (resolved)

- `kernel/channel.c` `name_match` is case-sensitive for the `WIN1_` prefix. The original lowercase
  `win1_zork1_z3` fails with `CHAN_ERR_NAME`. The patched name is `WIN1_ZORK1_Z3`.
- The remainder, `ZORK1_Z3`, is an eight-character name with no extension. `kernel/fat16.c`
  `fat16_name83` accepts it. `tools/make-fat16.py` previously rejected extensionless names; it now
  accepts them. The volume entry is verified by the runs (`ZORK1_Z3` appears in the root listing).
- No underscore-to-dot conversion is involved.

## 7. Memory and stack

Measured from the a.out header of the final image: text 29,840, data 1,456, BSS 4,672 bytes. The
game allocates a 24,576-byte working buffer with `malloc`. The resident area (header
`resident_bytes`, 19,284 for Zork I) and the 512-word Z-stack live inside it. The C stack is the
fixed 4 KiB job stack. C recursion occurs only in `obtree()` (the `-t` dump, not reachable in play);
C call depth in play was not instrumented.

## 8. Byte order and word widths

- `get_word` and `next_word` (`support.c`) decode big-endian explicitly (high byte shifted by 8).
- `word`/`signed_word`/`byte` widths are kept. The host LP64 build of the same patched source and the
  i386 build produce identical transcripts for the full sequence, which is evidence that the 16-bit
  and 32-bit arithmetic agree.

## 9. Behaviour found by testing

| Case | Result |
|---|---|
| truncated story (40,000 of 86,838 bytes) | original: `Internal Error #21` repeating forever; now: `story file is truncated`, shell survives |
| corrupt header (version 9, object table 0xFFFF) | original: `Internal Error #22`; now: `unsupported Z-code version 9` |
| empty file | now: `not a Z-code story file (0 bytes)` |
| valid header, random code from the initial PC (`badcode`) | original: `Internal Error #21` repeats forever; now: one message, then `exit status 1`, shell survives |
| missing story | `Failed to Open File WIN1_ZORK1_Z3`, shell survives |
| oversized story (+204,800 trailing bytes) | plays normally (declared length governs) |
| save / restore | `Filename:` then `Failed.`; game and shell continue |
| QUIT | returns to shell; process exit status is undefined (K&R `main` has no `return`) |
| internal error (any `err_` code) | now fatal: message, then the process exits with status 1 |

## 10. Not checked

- Licence of the story file for this hash against an official MIT release.
- The `qdos386-R11F.tar.gz` archive hash (the file was not in the working area).
- A human-operated session (all keys are scripted).
- Behaviour with a clock present (all Zork runs use the no-clock image).
