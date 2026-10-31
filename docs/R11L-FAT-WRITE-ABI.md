# R11L: writable FAT16 (opt-in) - ABI, ordering and crash policy

Status: **implemented and tested** on i386VM (block device), Tiny386 PC (IDE) and QEMU PC (IDE).
Build option: `-DQDOS_FS_WRITE`. Without it, every kernel, SDK and image of R11K is **byte-identical**
(`scripts/r11l-default-check.sh` rebuilds the R11I block, R11K paging and R11K default PC images and
compares hashes). The read-only device and mount stay; a build with the option on a read-only device
behaves as before for reads and returns an error for every write.

## 1. What is and is not supported

| | |
|---|---|
| Volume | FAT16 only, 512-byte sectors, two FAT copies, power-of-two sectors per cluster, 4085..65524 clusters (the R6 mount rules, unchanged) |
| Names | root directory only, uppercase 8.3 (`A-Z 0-9 _`) at the kernel; libc folds to upper case |
| Operations | create, open for read / write / append / update, write, seek (never past the end), truncate (shrink frees clusters, grow appends zeros), flush, close, remove, rename, free-space query |
| Not supported | subdirectories, long names (VFAT), FAT32, FAT12, partitions, file times other than a fixed stamp, `tmpfile`, delete-on-close, multi-sector atomicity, journaling |
| Sharing | any number of readers **or** one writer per file; `remove` and `rename` need the file closed (`-84` EBUSY) |

## 2. Device layer (`platform/platform.h`, `-DQDOS_FS_WRITE` only)

`int disk_write_sector(unsigned int lba, const unsigned char *buf)` -> 0, `-2` write-protected, other negative on error.

* **Tiny386 PC / QEMU PC** (`platform/pc/disk.c`): IDE `WRITE SECTORS` (0x30), LBA28, polled PIO, 256 `outw`. A drive that answers `ERR` is write-protected (`-2`).
* **i386VM block** (`platform/i386vm/disk.c`): the R11I byte-register device plus the R11L commands in `harness/i386vm-block/blockdev.h`: `0x03 WRBEGIN` (reset write index), 512 writes to `DATA`, LBA registers, `0x02 WRITE`. Status `6` write-protected, `7` incomplete buffer, `8` host write error. These exist only when the harness is built with `-DBLK_RW` (`scripts/r11l-build-harness.sh` -> `i386vm-r9-blockrw`) **and** was started with the third argument `rw`; the R11I harnesses are untouched and answer the new commands as unknown commands.
* The preloaded RAM disk (R6-R11H images) is read-only and stays so (`disk_write_sector` returns `-2`).
* Each transfer runs with interrupts off on i386VM (one command/LBA/data pointer).

The kernel cannot learn that a device is write-protected before writing, so the first operation that needs to write **rewrites sector 0 with its own contents** (`probe_write`), latching only a definite `-2`. On a read-only disk this is how `fopen("x","r+")` fails with EROFS instead of succeeding and failing later; on a writable disk it is a no-op (the image bytes do not change).

## 3. Kernel interface

New traps (kernels with the option only; others answer `-1`):

| Trap | a | b | c | Result |
|---|---|---|---|---|
| `0x2A QT_CH_OPENX` | name | flags | | channel id or error |
| `0x2B QT_CH_CTL` | channel | op | arg | op 1 flush, 2 truncate(arg=size), 3 size, 4 tell |
| `0x2C QT_FS_OP` | op | name | name2 | op 1 remove, 2 rename, 3 free clusters, 4 bytes per cluster, 5 total clusters |

**Trap numbers (renumbered during Stage 3/4).** The first R11L build used traps 0x25-0x27; the R11K paging launcher already owns
0x25-0x29 (`QT_PG_*`), so a kernel with both options failed to build (duplicate `case` values in `trap_dispatch`). The write traps
were moved to 0x2A-0x2C; no released R11K kernel or program uses them, so no ABI of a frozen round changed. The SDK reads the
numbers from `kernel/jobs.h`; programs built against an earlier R11L library must be relinked.

Flags: `READ 1, WRITE 2, CREATE 4, TRUNC 8, APPEND 16, EXCL 32` (`CHAN_O_*` = `FAT_O_*` = `QD_O_*`). `QT_CH_OPEN` (0x14) is unchanged and means READ. `QT_CH_WRITE` already existed (max 256 bytes per call, `QD_WRITE_MAX`); a write (or `putc`) on a channel that was not opened for writing still answers `-15` NOTSUP, exactly as the read-only R11K driver did (the R11E `QDOSAPI` regression test checks this; it failed when the new driver returned `-80` and the driver was changed, not the test); `WIN1_` channels now have a write routine. `QT_CH_CLOSE` writes the directory entry and returns that error if it fails (the channel is closed regardless); a job's channels are flushed (best effort) when it exits. `CHAN_MAX` is 16 with the option (8 without).

Error codes (negative; `FAT_ERR_*`): `-80` write-protected or read-only open, `-81` disk full, `-82` root directory full, `-83` exists, `-84` busy (open elsewhere), `-85` sector write failed, `-86` not open for this operation. Existing codes keep their meaning (`-44` no file, `-46` bad name, `-43` chain, `-45` size, `-47` range).

SDK (`sdk/include/qdos386.h`, library member `qdosfs.o`): `qd_open_ex`, `qd_flush`, `qd_truncate`, `qd_fsize`, `qd_tell`, `qd_remove`, `qd_rename`, `qd_fs_free_clusters`, `qd_fs_cluster_bytes`, `qd_fs_total_clusters`, `qd_fs_writable`, `qd_write_all`.

libc (`sdk/libc/stdio.c`, `misc.c`, `errno.c`): `fopen` with `r w a r+ w+ a+` (and `b`), buffered file streams (one 512-byte buffer, shared by reads and writes), `fwrite`, `fputs`, `fputc`, `fprintf` to files, `fflush` (buffer + directory entry), `fclose` (reports the flush error), `fseek` (`SEEK_END` works; seeking past the end of a writable file extends it with zeros), `ftell`, `remove`, `rename`, `tmpnam` (`TMPnnnnn.TMP`, checked against the directory). `errno` is set from the kernel code (`ENOENT EIO EBADF ENOMEM EBUSY EEXIST EINVAL EMFILE ENOSPC ESPIPE EROFS ENAMETOOLONG ENOSYS`). Not implemented and returning failure: `tmpfile`, `freopen`.

## 4. Write ordering and what a crash can leave

All writes are write-through: when `qd_write` returns, the data sector is on the device. Order for a **growing file**:

1. data sector (into a cluster that is still free in the FAT - a cut here loses nothing);
2. FAT: new cluster = end-of-chain, **first copy then second copy**;
3. FAT: predecessor's link to the new cluster (or the first-cluster field), first copy then second copy;
4. directory entry (size, first cluster, fixed stamp) - written **immediately** when a cluster was added, and again at flush/close.

Order for **truncate to a shorter size**: directory size, then the new end marker in the FAT, then the freed tail (head first). **Truncate to 0 / `fopen "w"`**: directory entry (size 0, first 0), then the chain is freed. **Remove**: directory entry marked deleted (0xE5), then the chain is freed. **Rename**: the 11 name bytes in the entry, one sector write. **Create**: one directory sector write. An overwrite inside the file touches data sectors only. FAT writes always go to both copies, copy 1 first.

There is **no journal and no atomicity beyond one sector**. What a power cut between two sector writes can leave (all observed in `scripts/r11l-interrupt.sh`, 107 cuts of 7 operations):

| State | Cause | Seen by the guest | Seen by `r11l-fatcheck.py` / `fsck.fat` |
|---|---|---|---|
| consistent | cut between independent steps; size may lag the data by less than a cluster | file opens, data is a prefix of what was written | clean |
| leaked clusters | cut after a chain was cut or an entry deleted but before the tail was freed; or after the end marker of a new cluster but before the link | files open | warning: orphan clusters (fsck.fat: "unused clusters") |
| chain longer than size | cut after the FAT link, before the directory update (two sector writes wide); or after shrinking the size | **that file refuses to open** (`FAT_ERR_CHAIN`, "fail closed") - never misread | error |
| FAT copies differ | cut between copy 1 and copy 2 | the guest reads copy 1 only | error (copy 2 is rewritten from copy 1 by repair) |

The host checker's `--repair` makes every one of the 107 cut images clean (no `ERROR`, no `WARN`, `fsck.fat -n` rc 0) and no file ever ends up with content that is neither the old nor the new data nor a documented prefix. The guest has no repair tool in R11L.

Other policies:
* **Disk full**: the cluster allocator scans from a hint; when none is free, `fwrite` returns a short count and `errno` is `ENOSPC`; bytes already in the stream buffer are lost as with any stdio. Nothing is left half-linked (data first, FAT after).
* **Root full**: `fopen(...,"w")` fails with `ENOSPC` (`-82`).
* **Short writes**: `fat16_write` returns the bytes it did write when that is non-zero and reports the error on the next call.
* **Bad clusters** (`0xFFF7`) and any nonzero FAT entry count as used by the allocator and are never allocated. A chain that reaches a free or bad entry, leaves the range, or cycles makes `open`/`remove` fail with `-43` (`remove` refuses rather than guessing) - the cluster walk is bounded by the cluster count, so a cycle cannot hang the kernel.
* **Malformed volumes** are handled by the existing mount checks (unchanged); R11L adds none and relaxes none.
* **I/O errors**: `-85` for a failed sector write, `-40` read; surfaced as `EIO`.

## 5. Evidence

`docs/R11L-RUNTIME-REPORT` section Stage 1 and `logs/r11l/stage1/`: `summary.txt`, per-run transcripts, initial and final images with hashes, checker output, `fsck.fat` output, the interrupted-operation CSV (`interrupt/sweep.csv`) and the Gate 1 persistence run (`persist/`).
Host unit tests: `tests/host/test_fat16w.c` (kernel code on the host, 290 checks, sector-write counter), `tests/host/test_blockrw.c` (device, 20 checks).
