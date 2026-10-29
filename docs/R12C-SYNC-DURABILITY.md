# QDOS/386 R12C — SYNC and durability

R12C adds a real `SYNC`: a versioned kernel service (`kernel/sync.c`, `-DQDOS_SYNC`), the shell command, a C call,
an optional periodic sync, and a runtime flush in the Tiny386 emulator. This document says exactly what reaches
stable storage when, and what does not.

## 1. Every write stage

| stage | where the bytes are | when they move on | SYNC reaches it? |
|---|---|---|---|
| a. C stdio buffer (`FILE *`, libc) | the writing process | `fflush`, a full buffer, `fclose`, `exit` (libc `fflush` also calls `qd_flush`, which writes the directory entry) | **no**: another process's buffer is invisible to the kernel. Each program flushes its own `FILE`s first (`SYNCT`, EDIT and BASIC SAVE close their files) |
| b. FAT16 data, FAT copies, directories | the device: the kernel has no cache; every sector write is an ATA PIO command that has completed when the call returns | — | already there |
| c. directory entry of a file open for writing (size, first cluster) | kernel memory (`fat16_file.dirty`) | a new cluster (written at once, `kernel/fat16.c`), `qd_flush`/libc `fflush`, close, **SYNC** | **yes** (step 1) |
| d. the drive | QEMU: host page cache of the image file (written with `pwrite` as each ATA command completes). Tiny386: the emulator's `FILE *` buffer for the image (`fwrite`), then the host page cache | QEMU: `fdatasync` on FLUSH CACHE. Tiny386 (R12C patch): `fflush` + `fsync` on FLUSH CACHE; at the 0x8900 shutdown `ide_flush_all(1)` | **yes** (step 2) |
| e. host storage | the host's disk and its cache | the host OS and the disk | as far as the host's `fsync`/`fdatasync` reaches |

## 2. The service

`QT_SYNC` = 0x39, op 0 (`qd_sync()`, shell `SYNC`):

1. For every open file channel with a pending directory entry: `fat16_flush` (the directory sector is rewritten).
2. ATA `FLUSH CACHE` (0xE7) to the volume's drive (`disk_flush` in `platform/pc/disk.c`): the status is polled
   until BSY clears (bounded; no answer = timeout), then ERR/DF are checked and the error register is read.
   Step 2 runs even when step 1 failed, so what can be kept is kept; the first error is returned.

Result: 0 only if every pending entry was written **and** the drive reported the flush complete without error.
Otherwise: a FAT error code (a directory write failed), -96 the drive answered with ERR (error register in the
status: 4 = aborted, which is how an unsupported command and QEMU's flush errors are reported; 64 = the R12C
Tiny386's host flush failure), -97 timeout or no drive, -18 no volume. **A drive or emulator without FLUSH CACHE
makes SYNC fail; there is no success-only path.**

Serialisation: the whole operation runs inside one trap (or in SYSTASK, in the SYSTEM job, with interrupts off, for the periodic
sync), as every other file system operation does, so it never interleaves with another file call and includes
every write that completed before it. Cost: interrupts stay off while the drive flushes; under Tiny386 that
is the host `fsync` time (timer ticks during it are lost, not queued).

Status (op 1, `qd_sync_status`, shell `SYNC /STATUS`): 16 words — 0 version (1), 1 syncs, 2 ok, 3 failed, 4 last
result, 5 its tick, 6 entries written by the last sync, 7 error register of the last device error, 8 periodic
syncs, 9 periodic failures, 10 interval (s), 11 entries written in all, 12 device flushes completed, 13 device
errors, 14 timeouts, 15 reserved.

Periodic sync (op 2, `qd_sync_interval(s)`, shell `SYNC /INTERVAL:s` or `SET SYNC_INTERVAL=s`, `UNSET
SYNC_INTERVAL` = 0): 0 = off (the default), 1..3600 seconds; other values are refused (-4) and the setting is
unchanged. SYSTASK (`kernel/taskmgr.c`, run by the SYSTEM job's loop every tick) checks the due tick and then runs exactly the
same `sync_now` with interrupts off — never from the timer interrupt, never two at once (one job, one call at a
time). A change of interval restarts the count from the moment it was set. The setting is a kernel setting:
`SET SYNC_INTERVAL` in any shell, or the `REM SET SYNC_INTERVAL=30` line in `/ETC/STARTUP.CMD`, changes it for
the whole system. It is not kept across a reboot.

## 3. Emulators

**QEMU** (8.2, `-drive ... format=raw,if=ide`, default `cache=writeback`): an ATA write completes after QEMU has
handed it to the host (`pwrite`); a SIGKILL of QEMU loses nothing that the guest saw complete. FLUSH CACHE
becomes `bdrv_flush` → `fdatasync` of the image file; a flush with nothing written since the previous one
completes without a host call. Durability level: after `SYNC` succeeds, the image file's data is on the host's
stable storage as far as the host's `fdatasync` guarantees. A host flush failure (tested with `blkdebug`,
`flush_to_disk` EIO) is reported to the guest as ERR/ABRT: `SYNC: FAILED (code -96) - FLUSH CACHE error (error
register 4: aborted)`.

**Tiny386** (R11M-FIX1 + `patches/tiny386-r12c-flush.patch`, built in `t386/r12c`): R11M-FIX1 aborted FLUSH CACHE
(unknown command), so `SYNC` fails there with error register 4 (tested). The R12C patch adds a `flush` method to
the block device and handles 0xE7 / 0xEA (FLUSH CACHE EXT): for the read/write image it checks an earlier write
error, `fflush`, `ferror`, `fsync` (`_commit` on Windows), all without closing the file; success completes the
command, any failure completes it with ERR and error register 0x40 (UNC), with a message on stderr. Read-only
images have nothing to flush (success); a snapshot image never writes the file, so its flush fails. The
0x8900 shutdown path (`ide_flush_all(1)`: flush, fsync, fclose, exit status) is unchanged.

Fault injection (tests only): `TINY386_FLUSH_FAIL=N` makes the Nth flush (counted over all images from 1) fail
as if `fsync` had failed; `N-` makes flush N and every later one fail; `TINY386_FLUSH_FAIL_STAGE=fflush` fakes
the `fflush` step instead. Both are reported on stderr as `(injected)`.

## 4. What the tests show (Gate 3, `tests/r12c/run-s3.sh`, `logs/r12c/stage3/`)

* QEMU: create, overwrite (shorter), rename, delete, a long VFAT name, and a file still open for writing with a
  pending size; `SYNC` reports "1 open file entries written, drive cache flushed"; the emulator keeps running
  (later commands answered); SIGKILL; on the host the image passes FAT-copy comparison, `fsck.fat -n` and the
  VFAT walker, and every file has exactly the bytes written (the open file all 300 lines); a restart reads them
  back (`DIR`, `TYPE`, long and case-insensitive names).
* QEMU negative (same writes, no SYNC, SIGKILL): the open file's last size never reached the disk — 2565 of its
  2592 bytes are visible (the size written at its last cluster allocation, line 297 of 300); the closed files are
  complete (write-through FAT16); the image itself is consistent.
* QEMU injection: `SYNC` reports FAILED -96, the counters record it, the next `SYNC` succeeds.
* Tiny386 (R12C emulator): the same writes + `SYNC` + SIGKILL → image clean and complete. Without `SYNC` the same
  SIGKILL lost exactly what QEMU lost (the open file's pending size: 2565 of 2592 bytes) and no sector data: glibc
  writes a `FILE *` buffer out on every `fseeko`, and Tiny386 seeks before every sector read and write, so only the
  last write command before the kill can still be in the emulator's buffer (the 0x8900 shutdown and SYNC flush
  it; a SIGKILL right after a write would lose it). Injected `fsync` and
  `fflush` failures give `SYNC: FAILED (code -96) - FLUSH CACHE error (error register 64)`, the next `SYNC`
  succeeds, and the 0x8900 shutdown still flushes and closes the image; the unpatched R11M-FIX1 emulator makes
  `SYNC` fail (error register 4) instead of pretending.

## 5. Not provided

* **No crash-atomic FAT.** A file system operation writes several sectors (FAT copy 1, FAT copy 2, directory);
  a power loss or emulator kill between them leaves a state `fsck.fat` must repair. SYNC only bounds *when*
  completed operations are durable.
* **No flushing of other processes' stdio buffers** (stage a). `SYNC` from the shell cannot see them.
* **No guarantee against power loss** beyond what the host's `fsync`/`fdatasync` and disk give; an emulator
  configured with `cache=unsafe` or a snapshot image ignores flushes by design.
* No write barriers or ordering guarantees between ordinary writes beyond their completion order.
* The periodic sync is not a journal: data written in the last interval can be lost.
