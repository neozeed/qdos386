# R11K boot audit — where the PC kernel-size limit comes from

Branch `r11k-integration`, from frozen R11J `f7664d7`.  Audited sources: `platform/pc/boot.s`,
`platform/pc/boot.ld`, `platform/pc/make-image.py`, `platform/pc/extract-kernel.py`,
`platform/pc/entry.s`, `platform/pc/kernel.ld`, and the Makefile rules that join them
(`$(PCD)/kernel.raw`, `$(PCD)/$(PCIMG)`, `build/pc-r9/boot.bin`).  Nothing in R11J/R11I/R11H-FIX1 was
modified.  Claim labels: **live-proven** (observed on the emulator named), **code-reviewed**, **not tested**.

## 1. The legacy boot path, step by step (`boot.s`)

| Step | What happens | Where |
|---|---|---|
| 1 | BIOS loads sector 0 at 0000:7C00, DL = boot drive. DS=ES=SS=0, SP=0x7C00, `sti`. | 16-bit, hand-encoded `.byte` (GAS 1.38 has no `.code16`) |
| 2 | `ax = [ksec] + [xsec]`; DAP count = ax; DAP = {size 16, count, offset 0, segment 0x1000, LBA 1}. | DAP at 0x7DB2 |
| 3 | **One** `INT 13h AH=42h` reads all `K+X` sectors to `1000:0000` (phys 0x10000). `jc hang` — no message; `AH` is not examined. | |
| 4 | `in/out 0x92` sets A20 (fast gate); `lgdt`; `CR0.PE`; far jump to `pm`. A20 is never verified. **IF stays 1** across the switch. | |
| 5 | PM: flat 4 GiB segments, ESP=0x90000. `rep movsb` K×512 bytes 0x10000 → 0x100000; then X×512 bytes → 0x280000 (R9 images: 0x340000, `sed` in the Makefile). `jmp 0x100000`. | |
| 6 | `kstart` (`entry.s`): `cli`, ESP=0x7FFFF0, zero `_bss_start.._bss_end`, `call _main`. | |

Disk layout: sector 0 boot, sectors 1..K kernel (`kernel.raw` = text+data of the a.out linked at 0x100000,
no BSS, padded to 512), sectors K+1..K+X transfer area. `extract-kernel.py` checks OMAGIC, no relocations,
entry == origin (0x100000) and that data follows text. `make-image.py` patches K at 0x1C2 and X at 0x1C4.

## 2. Where the 120-sector limit comes from

Three different things are involved and R11J documented them as one number.

1. **Geometry (hard limit): 128 sectors.** A single transfer into segment 0x1000 offset 0 cannot pass the end
   of that 64 KiB segment. 128 sectors = 65,536 bytes ends exactly at 0x1:0000:0000; sector 129 would wrap the
   offset. This is the *real* ceiling of the one-call design.
2. **Convention: 127 sectors.** `INT 13h` extended read is specified for at most 127 sectors per call on
   many BIOSes. `make-image.py` enforces `ksec <= 127`.
3. **Policy margin: 0xF000 bytes = 120 sectors.** `make-image.py` rejects `(K+X)*512 > 0xF000` with the message
   "kernel + transfer exceed the 64K real-mode buffer". That is 8 sectors (4 KiB) below the geometric limit.
   It is a safety margin, not a measured limit.

**Measured (live-proven).** The unchanged R11J boot sector, with `make-image.py` bypassed
(`tools/r11k-oldlimit-image.py`: forced counts, kernel zero-padded after its last byte), booted to the shell
at every size up to **128** sectors and failed at **129** and **160**:

| total sectors | 119 | 120 | 121 | 124 | 127 | 128 | 129 | 160 |
|---|---|---|---|---|---|---|---|---|
| Tiny386 (SeaBIOS rel-1.17.0 build in `bios.bin`) | boot | boot | boot | boot | boot | boot | **hang** | **hang** |
| QEMU 7.x/8.x (SeaBIOS 1.16.3) | – | boot | boot | boot | boot | boot | **hang** | **hang** |

("–": the VGA kernel alone is 116 sectors + 4, so 119 is not constructible.) Failure mode of the legacy
loader: the BIOS call returns CF=1, `jc hang` spins; the screen shows only the BIOS banner. No diagnostic.
Logs: `logs/r11k/boot/oldlimit-*.txt`.

Nothing else in the boot path bounds the kernel at 120 sectors. What does bound it:

* the real-mode **buffer** 0x10000..0x1FFFF (one call, 64 KiB);
* the **kernel's own memory map**: kernel text+data+BSS+page-table pool start at 0x100000, the heap arena is
  `bss_end .. 0x200000` (`MEM_ARENA_END`), slot 0 begins at 0x200000. The kernel image can therefore never
  exceed 1 MiB minus BSS and a usable heap, whatever the loader does;
* the **transfer area** copy target (0x280000, or 0x340000 on R9 images) and its 256 KiB (512 sector) check.

## 3. Other loader properties audited

* **Interrupts across the mode switch.** The legacy code never executes `cli` after `sti` at step 1. Between
  step 3 and the first `lidt` in the kernel the CPU runs with IF=1 and an IDTR left at the real-mode IVT. A
  timer tick in that window would triple-fault. At 60 KiB the window is microseconds; it grows with the copy.
  R11K's loader clears IF immediately after the last BIOS call.
* **A20.** Port 0x92 bit 1 ("fast A20") works on Tiny386 and QEMU. Real hardware without it, or with a
  different gate, is **not tested**. R11K verifies the result (0x500 vs 0x100500 must not alias) and refuses
  with `A` if A20 is off.
* **Real-mode buffer vs other low memory.** Stack 0x7C00 downwards, boot sector 0x7C00..0x7DFF, the BIOS data
  area (0x400..0x4FF) and IVT (0..0x3FF) are below the buffer; the PM stack is 0x90000. Conventional memory
  size is in the BDA word at 0040:0013 (639 KiB on both emulators); R11K checks it covers its larger buffer.
* **GDT/DAP/stack protection.** All are inside the boot sector (0x7DB0..0x7DFF) or below it. The buffer starts at
  0x10000, the PM copy destinations are >= 0x100000, so a transfer cannot reach the loader's own bytes. The
  check is arithmetic, not a runtime test: buffer end 0x80000 < 0x90000 (PM stack top) and < BDA memory size.
* **Entry address.** `kernel.ld` places `.text` at 0x100000 and `entry.s`'s `kstart` is the first object, so the
  jump target equals the image's first byte. `extract-kernel.py` already verifies entry == origin and no
  relocations. BSS is *not* in the image (the loader never sees it): `kstart` zeroes it from linker symbols.

## 4. R11K loader design (`platform/pc/boot2.s`, `make-image2.py`)

Same disk layout (no partition table, no second stage, no new toolchain). Differences:

* **Chunked reads.** `INT 13h AH=42h` in chunks of at most 64 sectors (32 KiB; never crosses a 64 KiB
  boundary) into consecutive low memory from 0x10000; DAP count/LBA/segment advance per chunk. Buffer
  0x10000..0x7FFFF = 448 KiB = **896 sectors** for kernel + transfer. Each call's CF and AH are checked.
* **Validation before any read:** K >= 1, 1 <= X <= 512, K+X <= 896 (no 16-bit overflow), BDA memory >= 512 KiB.
* **Integrity.** `make-image2.py` stores a 32-bit rotate-and-add checksum of all K+X sectors at 0x1C6; the
  loader verifies it in protected mode before copying. This catches truncated images whose BIOS reads succeed
  (zero-filled), and any corruption.
* **Diagnostics.** `QBOOT:<c>` on the Tiny386 debug port (0x402), COM1 and VGA text, then `cli; hlt`:
  `N` bad counts, `M` too little memory, `R` BIOS read error, `A` A20 off, `S` checksum mismatch.
* **IF is cleared** right after the last BIOS call.
* Rejected alternatives: a **second-stage loader** (needs a stage-1 read of stage 2 anyway and moves the
  layout); **unreal mode** (segment-limit retention differs between emulators); **INT 15h AH=87h** block move
  (an extra BIOS dependency); **protected-mode ATA PIO** in the loader (ignores the BIOS boot drive).
* The old loader is untouched and remains the default: the Makefile variables `PCBOOT`/`PCMKIMG` (there is no other switch)
  select the pair; defaults give byte-identical images (checked in `docs/R11K-RUNTIME-REPORT.md`).

## 5. Remaining limits of the R11K loader

* 896 sectors (448 KiB) for kernel + transfer area. The kernel's own memory map (§2) allows less in practice.
* Needs INT 13h extensions (AH=42h); the legacy loader needed them too.
* Real hardware, USB/CD boot, drives with non-512 sectors: **not tested**.
