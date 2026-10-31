# R11I Phase 0 — Block device evidence and interface audit

Branch `r11i-block`, from frozen R11H-FIX1 commit `4affc37` (archive SHA256
`43fe3a125355991df151057715a510d736cbf2349f921ad1fdef2971d6dcbf22`).  Nothing on `r11h-fix1`
or in the FIX1 archive was changed.

## 1. Frozen inputs and baseline

| Item | Value |
|---|---|
| FIX1 archive | `43fe3a12…dcbf22`, manifest verified on fresh extraction |
| Historical tree (`bin/i386-gcc`) | `3567f3beab83e17d` |
| Tiny386 emulator of record | `tiny386-shipped-fb63b405` |
| PC firmware | `bios.bin` `7bd968419c81fcf8`, `vgabios.bin` `f5aedb52cde5c88d` |
| R9 16 MiB i386VM harness (stock + input-status + r9-ram16) | `a4724f79b26a716e` (rebuilt, unchanged) |
| R11H release FAT16 volume `win1-r11h.img` | `672806ad2c2ca337…` |
| R11H negative volume `win1-r11h-neg.img` | `5af618370d810bb4…` |
| R11H i386VM release image (RAM disk) | `c5462138ea630fa7` |

Bounded baseline, kept in `logs/r11i/baseline/`: legacy RAM-disk i386VM `RUN MULTI.AOU` →
`MULTI: PASS`; Tiny386 PC `RUN MULTI.AOU` → `MULTI: PASS`.

## 2. Harness trace (source-verified)

* **CPU decode.** `i386.c` (`in_iomem`, ~line 753): every physical address `>= 0x800000` is routed
  to the harness `iomem_*` callbacks, whatever the configured RAM size.  The core never touches
  RAM for those addresses itself.
* **`machine.c` callbacks.** `rd`/`wr` serve bytes: `0x800001` console out (write),
  `0x800002` input (read), `0x800003` timer (read ticks / write period), `0x800004` input status.
  The R9 patch (`i386vm-r9-ram16.patch`) serves guest RAM from `0x801000` up to RAM top through
  `g_ram`, so `0x801000…0xFFFFFF` is real RAM.  Any other address in the device page is fatal
  (`read/write outside RAM: …`, exit 1).
* **Wide accesses.** `rd16/rd32/wr16/wr32` refuse device ports; only byte access is possible there.
  The protocol is therefore byte-wide.
* **Arguments and files.** `main` took exactly one argument (the image), read it with
  `fopen(argv[1],"rb")` into RAM at 0, and refused images that are empty or larger than RAM.
  There was no other host file access.
* **Chosen register range.** `0x800010…0x80001A`: inside the device page, above every existing
  port (`0x800001…0x800004`), below `0x801000`, and unused by the stock harness or any of the
  three historical patches (verified by reading all four sources).  The illustrative `0x8100`
  ports from earlier discussion are not used and are not an ABI.

## 3. FAT16 and disk interface

* `platform.h`: `int disk_read_sector(unsigned long lba, void *buf)` (0 or negative) and
  `unsigned long disk_sector_count(void)`.  Sector size is fixed at 512 throughout `fat16.c`.
* `fat16.c` `rd_sector` turns any negative return into `FAT_ERR_IO` and any `lba >= count` into
  `FAT_ERR_RANGE` before calling the backend; mount checks the BPB total against the count.
* `kernel/win1.c` mounts with `fat16_mount(&vol, disk_thunk, 0, disk_sector_count())`.
* `platform/pc/disk.c` (ATA PIO) is not touched by R11I; the PC kernel and image hashes are
  unchanged (§5).

## 4. Actual R9 address map (i386VM, 16 MiB)

Verified from `kernel/appmap.h`, `crt0.s`/`entry.s` stack setup, `tools/make-vm-image.py` and the
harness decode above.  See `R11I-MEMORY-MAP.md` for the old/new table.

`0x000000` kernel + heap → `0x200000` slot 0 (shell) → `0x240000` slot 1 (1 MiB) →
`0x340000` transfer area (256 KiB) → `0x380000` profile 2 → `0x480000…0x4FFFFF` unassigned → `0x500000` profile 3 →
`0x600000…0x7FFFFF` boot stack (top `0x7FFFF0`) and firmware data → `0x800000` device page →
`0x801000…0x8FFFFF` RAM, unused → `0x900000…0xBFFFFF` RAM disk (3 MiB) → `0xC00000…0xFFFFFF`
RAM, unused.

## 5. Risk analysis and decisions

| Risk | Decision |
|---|---|
| A timer switch mid-transfer interleaves two jobs' commands on the single device | Each sector transfer runs between `irq_save`/`irq_restore` |
| Guest writes the host file | Device has no write command; file opened `"rb"`; fd mode checked at runtime (`O_RDONLY`) |
| Host pointers / DMA addresses from the guest | None: 512-byte internal buffer, read one byte at a time |
| Stale data after the host file changes | Stream is unbuffered (`setvbuf _IONBF`); found by the short-read unit test |
| Legacy builds change | All changes are under `QDOS_I386VM_BLOCK`; legacy and PC hashes re-verified identical |
| Block kernel on an unpatched harness | Stops at once with `read outside RAM: 00800017` (verified), not a hang |
| Guest exploitation of the reclaimed span | The loader accepts only profile origins; the span is FREE but no origin lies in it |

No architectural blocker was found.
