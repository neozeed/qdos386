# R11M-FIX1 durability report: the Tiny386 FAT-copy mismatch

Question left open by R11M (`docs/R11M-KNOWN-LIMITATIONS.md` section 5): after a Tiny386 full-machine headless run the two FAT copies of the scratch
volume differed in one entry (cluster 3168: FAT 1 = 0x0000, FAT 2 = 0xFFFF), and an extra disk read before the end of the run made it disappear.
This report establishes where the write was lost, shows that the guest was not at fault, and shows what the emulator change does and does not guarantee.

**Verdict.** The guest kernel issued both FAT writes, in order, and the emulator received both. The second one stayed in the emulator's stdio buffer and was
discarded when the harness ended the process with SIGKILL. A guest-requested exit (and SIGTERM) already flushed it, because `exit()` flushes stdio; the
emulator now also does the flush explicitly, synchronises, checks every step and reports a failure through its exit status. SIGKILL and power loss remain
unprotected by design. **No crash-safe-write claim is made.**

## 1. Where the persistence boundary is (code read, baseline emulator)

Tiny386 tree used for R11 (`emulator/` and `docs/R11M-FIX1-EMULATOR-BUILD.md` give the hashes). Disk images are ordinary stdio `FILE`s:

| Point | Code | Persistence consequence |
|---|---|---|
| open | `block_device_init`: `fopen(name, "r+b")` for every hard disk (`ide_attach`) | fully buffered stdio stream |
| guest write | `bf_write_async` (`BF_MODE_RW`): `fseeko` + `fwrite`, result ignored | data sits in the stdio buffer |
| guest read | `bf_read_async`: `fseeko` + `fread` | the `fseeko` flushes the pending buffer as a side effect: this is why one more read hid the problem |
| flush / sync | none: no `fflush`, no `fsync`/`_commit` anywhere for hard disks | nothing forces the data out |
| ATA FLUSH CACHE (0xE7) | `#define WIN_FLUSH_CACHE 0xE7` only; no `case`, the command is aborted | a guest cannot ask for a flush |
| end of the headless / rawdraw loop | `return 0` from `main` | `exit()` flushes all open streams (unchecked, no sync) |
| `SDL_QUIT` (SDL frontend) | `exit(0)` | same |
| SIGTERM / SIGINT (headless, rawdraw) | handler sets `g_running = 0`, loop ends, `return 0` | same |
| guest "Shutdown" string on port 0x8900 | sets `shutdown_state = 8`, loop ends, `return 0` | same |
| SIGKILL, crash, power loss | nothing runs | the unflushed stdio buffer is lost |

## 2. Reproduction on a disposable copy (Phase 1.2)

| Item | Value |
|---|---|
| Emulator | the baseline emulator **with write tracing only** (a `fprintf` per sector write to a log, flushed at once); `t386/trace` build, SHA-256 `d38ca78e...ee76e` |
| Kernel | `build/r11m/pc-pg/r11m-pg.img` (R11M paging kernel, unchanged) |
| Volume | `scripts/r11m-demovol.sh` volume, SHA-256 `52542b2d417d525a...` (the original R11M scenario); a second run used `scripts/fix1-vol.sh` (same plus `SHUTDOWN.AOU`), `29e8d7b8a068fc35...` |
| Sequence | `tests/r11m/seq-s3a.txt`: `PS`, `RUN JOBT.AOU`, `PS` (JOBT: 69 checks, 0 failed) |
| Run | `scripts/r11l-run-pc.sh` (**no** extra read) |
| Guest state at the end | shell prompt, `PS` answered, JOBT finished |
| Host termination | `proc.kill()` = SIGKILL |
| Result, demo volume | `fsck.fat -n`: "FATs differ but appear to be intact"; `r11m-fatwalk.py --strict --vfat`: "FAT copies differ in 1 entries"; cluster 3168: FAT 1 = 0x0000, FAT 2 = 0xFFFF |
| Result, fix1 volume | the same, cluster 3229 (the volume holds one more file), FAT 1 = 0x0000, FAT 2 = 0xFFFF |

**Reproduced in every forced-kill run of this milestone** (the original R11M matrix run, the traced run, the baseline comparison, and the forced-kill case of the final matrix: 4 of 4),
and in **no** guest-ended or SIGTERM run. Files: `logs/fix1/phase1/`, the original artefact is preserved unchanged in `logs/fix1/r11m-noflush/`.

## 3. Guest completion versus emulator persistence (Phase 1.3)

The write trace of the run above has 27 sector writes to the volume. The last three are

```
W 25 lba=53   (directory sector)
W 26 lba=13   (FAT 1 sector that holds the freed cluster)
W 27 lba=37   (FAT 2 sector, the same entries)
```

LBA 13 and 37 are exactly FAT 1 and FAT 2 (reserved 1 sector, 24 sectors per FAT) of the entry at fault. So the guest freed the chain of the killed `WRITER`
job's file, wrote FAT 1 and then FAT 2, **both ATA write commands reached the emulator**; the image file on disk contained write 26 and not write 27.
The kernel's order (FAT 1, then FAT 2) is the R11L write-through order. No guest fix is justified, and none was made.

## 4. Termination modes (Phase 1.4), baseline emulator unchanged except for tracing

| End of run | Emulator exit | FAT copies / fsck.fat / fatwalk |
|---|---|---|
| guest request (`RUN SHUTDOWN.AOU`, string on port 0x8900), baseline exit path | by itself, status 0 | equal / clean / 0 errors (image `fdd74387...`) |
| SIGTERM | by itself, status 0 | equal / clean / 0 errors (identical image) |
| SIGKILL | killed | **differ in 1 entry** (cluster 3229: 0x0000 / 0xFFFF) |

So the graceful path was already correct in effect; what was missing was an explicit, checked, synchronised teardown and a way for the guest to ask for it.

## 5. Change (`patches/tiny386-fix1-shutdown.patch`, details in `docs/R11M-FIX1-SHUTDOWN-ABI.md`)

* `ide_flush_all(close_files)`: for every writable image, `fflush`, `ferror`, `fsync` (`_commit` on Windows), `fclose`; each failure is named on stderr; a failed `fwrite` is
  remembered and reported there as well.
* `pc_shutdown(pc)`: calls it after the CPU loop stopped, prints one line, returns 0 or 3. All frontends end through it.
* Guest side: `SHUTDOWN.AOU` (refuses while launched jobs exist, flushes stdio, writes the string).

## 6. After the change

| Check | Result |
|---|---|
| 3 rounds x 4 boots (JOBT; mkdir/long names/copies/renames; read and rename after reboot; delete everything) on three independent volumes, every boot a fresh process ended by `SHUTDOWN.AOU`, **no extra read** | every boot: exit 0, FAT copies equal, `fsck.fat -n` clean, `fatwalk --strict --vfat` 0 errors; alias/long-name checks and mtools agree (`logs/fix1/matrix/judge.txt`) |
| JOBT frees a chain in both copies (the original case), then shutdown | clean in all three rounds |
| wrong string (`SHUTDOWN /W`) | ignored, machine keeps running, the real request afterwards exits with status 0 |
| shutdown refused while a job runs | refused; after `KILL` the shutdown succeeds, image clean |
| shutdown right after boot (no writes) | status 0; volume byte-identical to the initial volume |
| injected `fsync` EIO | emulator exits by itself with **status 3**, stderr names both images, "FLUSH FAILED" |
| injected `fflush` EIO | the same, status 3 |
| SIGTERM after JOBT | status 0, image clean |
| SIGKILL after JOBT (forced case) | **image still differs in one FAT entry**: reported, not hidden, see section 7 |

## 7. What remains true

* **Forced power loss / SIGKILL / host crash can still lose the last buffered write.** That is a property of a userspace emulator writing through stdio, not a regression of R11M. The success criterion
  of this fix is the graceful, guest-requested exit, as agreed. The forced case is kept in the matrix as an information line.
* The emulator still does not implement ATA FLUSH CACHE and the kernel's PC disk driver does not issue it. A write-through guest on an emulator that buffers is only as durable as the next flush.
* `fsync` guarantees that the host has the data, not that the drive's cache does; there is no directory fsync and no `F_FULLFSYNC` on macOS.
* The emulator does not report host write errors to the guest (the IDE protocol is unchanged). An injected EFBIG (RLIMIT_FSIZE) made later reads return garbage (guest: "bad cluster chain");
  this is the emulator's pre-existing behaviour on a failing image, recorded in `logs/fix1/fsize-injection/`, and is not judged.
* The guest still has no mount-time comparison of the two FAT copies and no repair; a storage-hardening milestone remains the place for that (and for an explicit flush/barrier in the PC driver once an
  emulator implements FLUSH CACHE).
* The extra-read workaround (`scripts/r11m-run-pc.sh`) is retired from every FIX1 script; it was never a guarantee.
