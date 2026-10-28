# R11K boot ABI — the PC multi-transfer boot sector

Scope: `platform/pc/boot2.s` + `platform/pc/make-image2.py`. The original `boot.s` + `make-image.py` are
unchanged and remain the default. Evidence is in `docs/R11K-BOOT-AUDIT.md` (why the old limit existed) and
`docs/R11K-RUNTIME-REPORT.md` (what was run). Claim labels: **live-proven**, **build-only**, **code-reviewed**,
**not tested**.

## 1. Selecting a loader

The Makefile's `pc` target takes two variables (nothing else changed):

| | boot sector | image tool | max kernel+transfer |
|---|---|---|---|
| default (R11H / R11I / R11J) | `build/pc-r9/boot.bin` (`PCBOOT`) | `platform/pc/make-image.py` (`PCMKIMG`) | 120 sectors (0xF000 bytes) |
| R11K | `build/pc-r9/boot2.bin` | `platform/pc/make-image2.py` | 896 sectors (448 KiB) |

    make pc ... PCBOOT=build/pc-r9/boot2.bin PCMKIMG=platform/pc/make-image2.py

The choice is independent of `QDOS_PAGING`: the R11K loader boots the ordinary non-paging kernel
(`build/r11k/pc-default*/`) as well as the paging kernel. Default images are byte-identical to R11I
(`logs/r11k/default/identity.txt`; **live-proven** by hash). `boot2.bin` for R9 images is produced from the
same source by the same `sed` (transfer area 0x340000) as the original.

## 2. Disk layout (unchanged)

    LBA 0           boot sector (this file)
    LBA 1 .. K      kernel image: text+data of the a.out linked at 0x100000, zero-padded to a sector
    LBA K+1 .. K+X  transfer area (X sectors; 4 on the current images)

No partition table, no second stage, no ELF. The kernel's BSS is not in the image; `kstart` clears it.

## 3. Fields patched into the boot sector by `make-image2.py`

| offset in sector | size | meaning |
|---|---|---|
| 0x1B0 | 1 | boot drive (written by the loader from DL) |
| 0x1B2 | 16 | disk address packet (count/LBA/segment advance per chunk) |
| 0x1C2 | 2 | `ksec` K, 1 ≤ K |
| 0x1C4 | 2 | `xsec` X, 1 ≤ X ≤ 512 |
| 0x1C6 | 4 | checksum (new) of all K+X sectors: `s = rol32(s + dword, 1)`, little-endian dwords, s₀ = 0 |
| 0x1D0 / 0x1E0 | 6 / 24 | GDT descriptor / flat 4 GiB null+code+data GDT |

`make-image2.py` refuses (before writing a disk) K < 1, X < 1, X > 512, K+X > 896, a boot sector whose
count/checksum fields are not zero, and a kernel that is not an exact whole of sectors after padding.
Host tests: `tests/host/r11k/test_boot_images.py` (just below, at and above 120 sectors, up to 896).

## 4. Algorithm

1. `cli`, segments 0, `sp=0x7C00`, `sti` (BIOS disk services need interrupts). Save DL.
2. Validate K, X, K+X (`N` on failure) and the BIOS conventional-memory word at 0040:0013 ≥ 512 KiB (`M`).
3. Read K+X sectors with `INT 13h AH=42h` in chunks of at most **64 sectors (32 KiB)** into consecutive
   memory from 0x10000 (so no transfer crosses a 64 KiB boundary and the buffer 0x10000..0x7FFFF cannot touch
   the loader at 0x7C00, the BIOS data area, or the protected-mode stack at 0x90000). After every call CF and AH
   are checked (`R`); the DAP is reloaded from fixed addresses because a BIOS need not preserve SI.
4. `cli`; port 0x92 bit 1 (fast A20); `lgdt`; `CR0.PE`; far jump.
5. Protected mode: flat segments, `esp=0x90000`. A20 is **verified** (0x500 versus 0x100500 must not alias;
   `A`). The checksum over the K+X sectors is compared (`S`). Only then are the kernel (K×512 bytes) copied to
   0x100000 and the transfer area (X×512 bytes) to 0x280000 (R9: 0x340000), and the loader jumps to 0x100000.

Nothing is copied unless every check passed: a refused image leaves 0x100000 and the transfer address
untouched (**live-proven** on QEMU by reading guest memory after each refusal).

## 5. Diagnostics

On any failure the loader writes `QBOOT:<c>\n` to port 0x402 (Tiny386 debug port), COM1 (0x3F8) and the
top-left of the VGA text buffer, then `cli; hlt`.

| c | meaning |
|---|---|
| N | invalid sector counts |
| M | too little conventional memory for the buffer |
| R | the BIOS reported a read error (CF set or AH ≠ 0) |
| A | A20 did not come on |
| S | checksum mismatch: truncated or corrupt image |

Emulator note (**live-proven**): Tiny386's disk returns zeros past the end of the image file, so a truncated
image reads "successfully" and is caught by `S`; QEMU/SeaBIOS fails the read and reports `R`. Both are refusals.
On the Tiny386 `-term` console the debug port and the VGA are both mirrored, so a diagnostic may appear twice.

## 6. Limits that remain

* 896 sectors (448 KiB) for kernel + transfer area. The kernel's own memory map allows less in practice: text,
  data, BSS and the heap arena must all lie below 0x200000 (slot 0).
* Needs INT 13h extensions (AH=42h), as the old loader did. Hard disks only: floppy, CD and USB boot are **not
  tested**; real hardware, other BIOSes, non-512-byte sectors and boot from a partition: **not tested**.
* A20 other than port 0x92 (keyboard controller gate, BIOS INT 15h): not implemented; such a machine is
  refused with `A` rather than booted with a wrapped address space.

## 7. Evidence summary

| claim | label |
|---|---|
| old loader works to 128 sectors, hangs silently at 129 and 160 (Tiny386, QEMU) | live-proven (`logs/r11k/boot/oldlimit-*.txt`) |
| new loader boots 119 … 896 sectors, default and paging kernels, with the in-kernel pad check | live-proven (`logs/r11k/boot/matrix/`) |
| N / R / S refusals, no hang, nothing copied | live-proven (Tiny386 + QEMU, 3 rounds) |
| memory after boot equals disk bytes | live-proven (QEMU) |
| disassembly of the hand-encoded real-mode part equals the reviewed listing | build-only (`tests/host/r11k/boot2-disasm.sh`, needs objdump) |
| real hardware | not tested |
