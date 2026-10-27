# R11H-FIX1 memory map and stack audit (Phase 2)

## Initial stack (verified from generated code, not from a document)

- **PC (Tiny386):** `platform/pc/entry.s` sets `movl $0x7FFFF0,%esp` at `kstart`. Verified in source.
- **i386VM:** the kernel raw image starts with the historical toolchain's `runtime/crt0.s` prepended by
  the `--raw` link. Its first bytes are `fc bc f0 ff 7f 00 31 ed`, i.e. `cld; movl $0x7ffff0,%esp;
  xorl %ebp,%ebp`. Checked in `build/i386vm-r11h-rel/kernel-raw.bin` (offset 1: `bc f0 ff 7f 00`, the only
  such `mov esp` in the image).

Conclusion: the stack top 0x7FFFF0 is correct for both targets. For i386VM it depends on the external
`runtime/crt0.s` in the historical tree, which is not in this repository. That dependency is recorded
here and in docs/R11H-FIX1-BASELINE.md. It is not an error in the kernel source.

## Guest RAM and device regions

| target | guest RAM | evidence |
|---|---|---|
| Tiny386 PC, R9 map | 8 MiB (`run.ini mem_size = 8M`) | platform/pc, Makefile run.ini rule |
| i386VM R9 harness | 16 MiB | `patches/i386vm-r9-ram16.patch`: `RAM_MB` defaults to 8 and the R9 harness is built with `-DRAM_MB=16`. Rebuilt with `scripts/r11h-build-harness.sh` (see below). `kernel/appmap.h` `RAM_TOP = 0x1000000`. |
| i386VM R8 harness | 8 MiB | `kernel/appmap.h` `RAM_TOP = 0x800000` |

i386VM R9 device page: `0x800000..0x800FFF` (ports). RAM from `0x801000` to `0xFFFFFF`. The RAM disk
(FAT16 transport) at `0x900000..0xBFFFFF`. Free: `0xC00000..0xFFFFFF`. The image is 12,582,912 bytes
(`0xC00000`), which matches the end of the RAM disk.

## Checker output (current tree)

`scripts/r11h-memmap-check.py build/pc-r11h/kernel.aout build/r11h-maps/i386-r9.aout build/r11h-maps/i386-r8.aout`
gives `MEMMAP CHECK: PASS`. Full output: `docs/evidence/fix1-memmap-check.txt`.

Summary of the R9 i386VM map: kernel and BSS `0x0000000..0x15DC0`; heap to `0x200000`; profile 0
`0x200000..0x240000` (shell); profile 1 `0x240000..0x280000` (child); transfer area `0x340000`; RAM
disk reservation; boot stack and firmware `0x600000..0x800000`; device page `0x800000`; unassigned
`0x480000..0x500000` (512 KiB, between profiles 2 and 3, not reserved and not a profile).

## Harness build (reproducible)

`I386VM=<historical tree> sh scripts/r11h-build-harness.sh <outdir>`

- `i386vm-input-only` (input-status patch only): sha256 `2d2f8beba177592e…`, identical to the recorded
  hash in the historical `patches/HASHES.txt`. The recipe is therefore the one that produced the
  recorded harness.
- `i386vm-r9` (input-status plus R9 RAM patch, `-DRAM_MB=16`): sha256 `a4724f79b26a716e…`, deterministic
  across rebuilds.
- `patches/i386vm-synthetic-timer.patch` does not apply on top of the two patches (both hunks fail). The
  R9 harness is built without it. Its MULTI transcript on the shipped R11H i386VM image matches the
  shipped transcript line for line, excluding timer values (md5 of non-timer lines `10f5314e9bf9`, both).

## Open

- The R11H i386VM runs used an `i386vm-r9` binary that was never recorded (see the baseline). FIX1 runs
  use the rebuilt `i386vm-r9` above and record its hash in each log.
- i386VM image hashes reproduce (Phase 2.3): `build/i386vm-r11h-rel` and `build/i386vm-r11h-neg` match
  the shipped `kernel-raw.bin` and image byte for byte. Make targets: `make r11h-i386vm I386VM=<tree>`.
