# R11H-FIX1 build reproducibility (Phase 1)

## Result

One command rebuilds the four shipped PC artifacts byte-identically from a clean build directory:

    make r11h-pc I386VM=<historical tree>

| artifact | shipped sha256 (first 16) | rebuilt |
|---|---|---|
| build/pc-r11h/kernel.aout | 5fed2619e0a0e8d1 | identical |
| build/pc-r11h/kernel.raw | 017891a9e885e0b0 | identical |
| build/pc-r11h/r11h-shell.img | 8f22b0b2613d0554 | identical |
| build/pc-r11h-vga/r9shell-vga.img | 0560ff320177cc33 | identical |

All 32 objects of the serial shell kernel and all 32 of the VGA kernel are identical to the shipped
objects.

## Why the previous rebuild differed

The R11H notes did not record the flags the shipped images used. `make pc` with no flags builds a
different kernel. Three inputs were missing:

1. `PCFLAGS` for the R9 memory map: `-DQDOS_LAYOUT_R9`. Without it, `aout.o` and `main.o` differ.
2. `PCFLAGS` for the R7 shell mode: `-DR7_SHELL_MODE`. `main.o` changes with it.
3. The boot sector. The R9 images use `build/pc-r9/boot.bin` (transfer address 0x340000), not
   `platform/pc/boot.bin` (0x280000). Only byte 0x79 differs between them, but that byte is the
   transfer address and it changes the whole image.

The VGA image also used the pinned clock: `-DQDOS_CON_VGA -DQDOS_CLOCK_EPOCH=1793102400
-DQDOS_CLOCK_PINNED`. Without the epoch, `clock_boot.o` differs.

## Canonical recipes

From `Makefile` target `r11h-pc`:

- `build/pc-r9/boot.bin` (R9 boot sector, from `platform/pc/boot.s` with the transfer address changed)
- `pc PCD=build/pc-r11h PCFLAGS="-DQDOS_LAYOUT_R9 -DR7_SHELL_MODE" PCIMG=r11h-shell.img PCBOOT=build/pc-r9/boot.bin`
- `pc PCD=build/pc-r11h-vga PCFLAGS="-DQDOS_CON_VGA -DQDOS_LAYOUT_R9 -DR7_SHELL_MODE -DQDOS_CLOCK_EPOCH=1793102400 -DQDOS_CLOCK_PINNED" PCIMG=r9shell-vga.img PCBOOT=build/pc-r9/boot.bin`

Tools: the historical i386-gcc, as and i386-ld tree (hashes in docs/R11H-FIX1-BASELINE.md). The Makefile's
default `I386VM` is `\$(HOME)/i386vm`, which does not exist here; pass `I386VM=<tree>`.

## Stale-artifact fix

Before this change, `PCFLAGS` was not a dependency. Building an existing `PCD` with different flags
reused objects compiled with the old flags, so the output depended on build history.

Fix: `\$(PCD)/.pcflags` records `PCFLAGS`. It is rewritten only when its content changes, and every
compiled object depends on it. Changing the flags for a directory rebuilds all 30 compiled objects.
Tested: `make pc PCD=build/pc-r11h PCFLAGS="-DQDOS_LAYOUT_R9"` after the canonical build recompiled 30
objects, and the canonical rebuild then restored identical artifacts.

## Dependency tests (real builds, not dry runs)

| change | rebuilt |
|---|---|
| `kernel/jobs.c` | `jobs.o` in each PC directory, then relink, raw image and shell image |
| `platform/pc/kernel.ld` | `kernel.aout` relink in each directory, then raw and image |
| `build/libqdos.a` | nothing. The PC kernel does not link libqdos.a (it is an application library). |
| `PCFLAGS` | all 30 compiled objects in that directory |
| second run, no change | nothing compiled |

Note: the first `jobs.c` test also rebuilt `test_shell.o`. The cause was the state before the stamp was
written; repeat tests rebuilt only `jobs.o`.

## Application and volume reproduction

Rebuilt in this worktree from clean sources, with the historical tree `x/i386vm/i386vm` (i386-gcc
3567f3beab83e17d, identical to the baseline):

| artifact | rebuilt sha256 (first 16) | shipped (SHA256SUMS) | result |
|---|---|---|---|
| `build/r11h/MULTIA.AOU` | 1d8118c1261241dc | 1d8118c1261241dc | identical |
| `build/pc-r11h/r11h-shell.img` | 8f22b0b2613d0554 | 8f22b0b2613d0554 | identical |
| `build/pc-r11h-vga/r9shell-vga.img` | 0560ff320177cc33 | 0560ff320177cc33 | identical (not rebuilt; checked) |
| `build/fat16/win1-r11h.img` | 672806ad2c2ca337 | 672806ad2c2ca337 | identical (not rebuilt; checked) |

The release volume was not rebuilt over the shipped one. A full rebuild of `scripts/r11f-volume.sh`
then `scripts/r11h-volume.sh` (into `win1-r11h-rebuilt.img`, 890d96746ce7c5c7) is **not** byte-identical:
every R11F application differs from the shipped copy by the same 38 bytes. Example, CTYPE.AOU: the text
size grows 0x3dc0 to 0x3dd0 and the entry moves 0x96c to 0x978. The R11H applications MULTI, MULTIA,
MULTIB, COLLIDE and NEG match the shipped volume byte for byte (FAULT is new in FIX1 and has no shipped
copy). The rebuilt R11F set was produced after this
worktree's SDK libraries were rebuilt (`sdk/lib`, `sdk/libc`, `sdk/libm`, `sdk/compiler`), and the
historical archive that produced the shipped R11F objects was not captured, so the cause is not
established. Resolution: the matrix runs against the shipped bytes; the rebuilt volume is not a release
candidate. The exact cause is an open item for FIX2, with this as the first reproducible failure.

## Not covered

- The historical tools themselves are not rebuilt; they are identified by hash only.
- Rebuilt R11F application bytes (above).
- Zork I and the private story volume (not in this worktree).
