# R11I — i386VM host-backed block device protocol

Optional device added by `patches/i386vm-block.patch` plus `harness/i386vm-block/blockdev.{c,h}`.
Read-only, one 512-byte sector per command, synchronous, no interrupts, no DMA.

## Launch

    i386vm image.bin               # unchanged: legacy RAM-disk image, device reports NODISK
    i386vm image.bin disk.img      # disk.img opened read-only and served by the device

`disk.img` is refused before the guest starts (stderr `disk <path>: refused (<code>)`, exit 2) if it
cannot be opened (`-2`), is empty (`-3`), its size is not a multiple of 512 (`-4`), or its size cannot
be determined (`-5`).  The host file is opened with `fopen(path, "rb")` (`O_RDONLY`), unbuffered.

## Registers

All registers are **one byte**.  The harness refuses 16- and 32-bit accesses in the device page,
so the guest must use byte loads and stores.  Multi-byte values are **little-endian**, byte 0 at the
lowest address.

| Address | Name | Access | Meaning |
|---|---|---|---|
| `0x800010` | CMD | W (reads 0) | `0x01` = read sector LBA into the buffer; any other value → BADCMD |
| `0x800011…0x800014` | LBA0…LBA3 | R/W | 32-bit LBA; read back as written |
| `0x800015` | STATUS | R | result of the last command (table below) |
| `0x800016` | DATA | R | next buffer byte; auto-increments; after 512 bytes returns 0 and stays |
| `0x800017…0x80001A` | COUNT0…COUNT3 | R | 32-bit sector count of the open image (0 if none) |

Writes to STATUS, DATA and COUNT are ignored.  Reads of unassigned addresses in the range return 0.

## Status codes

| Code | Name | When |
|---|---|---|
| 0 | OK | sector in buffer (also the state after a successful open) |
| 1 | NODISK | no image (harness started with one argument) |
| 2 | RANGE | `LBA >= count` |
| 3 | SHORT | host returned fewer than 512 bytes (file shrank after open) |
| 4 | BADCMD | CMD value other than `0x01` |
| 5 | IOERR | host `fread` error (code-reviewed; not reproducible portably in C89 tests) |

On any status other than OK the buffer is all zeros.

## Sequencing

1. Write LBA0…LBA3 (any order).
2. Write `0x01` to CMD.  The command completes **before the write instruction retires**; there is
   no busy state and no need to poll.  CMD resets the DATA index to 0.
3. Read STATUS.  If 0, read DATA 512 times.

The guest (`platform/i386vm/disk.c`) performs steps 1–3 with interrupts disabled
(`irq_save`/`irq_restore`), because the device has one LBA, one buffer and one DATA index shared by
every job.  The sector count is read once per mount from COUNT0…COUNT3.

## Limits and overflow

* LBA and count are 32-bit: at most 2^32−1 sectors (≈2 TiB) are addressable by the protocol.
* The host uses `fseek`/`ftell` with `long`, so on a host whose `long` is 32 bits an image is limited
  to 2 GiB; larger images are refused at open (`-5`).  On LP64 hosts this limit does not arise.
* The guest FAT16 code itself accepts at most a FAT16 volume (the R11H volume is 3 MiB).
* LBA `0xFFFFFFFF` and `0x01000000` return RANGE (unit-tested); the byte offset is computed only after
  the range check, so it cannot overflow.

## Unit tests (`tests/host/r11i/blockdev_test.c`, strict C89, 88 checks)

Port map, NODISK before open, every open refusal, last valid sector, first invalid sector, large LBAs,
short read after truncation under the open device, repeated reads, DATA auto-increment and end
behaviour, read-only register writes, all 255 invalid commands, reopen, and the host file's FNV-1a
checksum unchanged after every phase.
