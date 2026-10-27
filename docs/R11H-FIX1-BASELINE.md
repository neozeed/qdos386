# R11H-FIX1 baseline (Phase 0)

Frozen starting point: R11H commit `a15bd80` (branch `r11h`). Phase 2 checkpoint `a3c432a` is
preserved and unchanged. The fix work is on branch `r11h-fix1`, created from `a15bd80` in the
worktree `fix1`. Frozen R11F, R11G and R11H sources are not altered.

## Delivered R11H package (verified before FIX1 started)

- `qdos386-R11H.tar.gz` sha256 `b95009dfbe008a8a0d45ee689b245dc72730398abca259fc13edb4fb5f58cdec`
- `SHA256SUMS` verified by `sha256sum -c` on a clean extraction: 907 entries.

## Toolchain (historical tree used for the R11H build)

Tree: `scratchpad/x/i386vm/i386vm/bin` (the tree used for the R11H programs and PC images).

| file | sha256 (first 16) |
|---|---|
| i386-gcc | 3567f3beab83e17d |
| as | 47369dd548514480 |
| i386-ld | dfdb396514559958 |
| cc1 | 1dd52eeb51191b0f |
| cccp | a5d10c5c176cad35 |
| i386vm (standalone harness) | baa683711ecfe674 |

## Firmware and emulators

| item | sha256 (first 16) | note |
|---|---|---|
| SeaBIOS used by the harness, `build/pc-r11f-none-run/bios.bin` | 7bd968419c81fcf8 | the file the R11H harness copies |
| VGA BIOS, `build/pc-r11f-none-run/vgabios.bin` | f5aedb52cde5c88d | |
| `/usr/share/seabios/bios.bin` (system package, not used by harness) | 4d597f68e06a0e28 | recorded for comparison only |
| QEMU | 8.2.2 (Debian) | VGA smoke only |
| Tiny386 candidates (see below) | | |

The harness reads `bios.bin` and `vgabios.bin` from `build/pc-r11f-none-run/`. That directory is not
in the R11H package. FIX1 will ship these two files with the package.

### Tiny386 binary: not recorded in R11H

The R11H harness expects `$HOME/tiny386/tiny386_headless`, which does not exist on this machine. Three
candidate binaries exist under `scratchpad/bootcompat/tiny/`:

| candidate | sha256 (first 16) |
|---|---|
| tiny386-shipped-fb63b405 | fb63b4057e3a62f5 |
| tiny386-dev-base-36e29bb1 | 36e29bb17a0844b3 |
| tiny386-dev-patched-d5da4535 | d5da453542fe10d5 |

The R11H transcripts do not record which binary ran. The three candidates produce identical
deterministic output on the MULTI sequence (all non-timer lines match, md5 31a7d14b59df). Their timer
lines differ, so MULTI cannot identify the binary. FIX1 uses `tiny386-shipped-fb63b405` as the emulator of
record and records its hash in every log.

## What was verified before FIX1

- Runtime results in `docs/R11H-RUNTIME-REPORT.md` (NEG 848/0 on both emulators, MULTI, Zork, Phoon,
  regression, QEMU VGA with the corrected smoke volume).
- The shipped kernel, linked image and boot sector reproduce from documented commands; see
  `docs/R11H-FIX1-BUILD-REPRO.md` (written during Phase 1).

## Unresolved at the start of FIX1

1. PC kernel and VGA image reproducibility (resolved in Phase 1, see the build-repro doc).
2. Phoon `7 Feb 2106 06:28:14` discrepancy against R11F (Phase 3).
3. i386VM initial `ESP` = 0x7FFFF0 not verified in source (Phase 2).
4. Unreached loader and region negative paths (Phase 4).
