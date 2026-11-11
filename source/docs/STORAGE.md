# Storage (canonical R18/R20 PC baseline)

Files: `platform/pc/disk.c` (the IDE PIO driver), `kernel/fat16.c|h` (FAT16 and VFAT), `kernel/win1.c|h` (the `WIN1_`
file driver), `kernel/sync.c|h` (storage sync), `kernel/r20w.c|h` (R20 storage worker and sector cache). Design and
measurements: `docs/A16-CACHE-DESIGN.md`, `docs/A16-CACHE-RESULTS.md`, `docs/R12C-SYNC-DURABILITY.md`.

## Device

- The FAT16 volume is the primary slave of the PIIX3 IDE controller (ports 0x1F0–0x1F7).
- Only IDENTIFY DEVICE (0xEC) and READ SECTORS (0x20) are issued on the read path; the write path uses WRITE SECTORS and
  FLUSH CACHE through `sync`. Addressing is LBA28, transfer is polled PIO. No IRQ14, no DMA.
- Sector size is 512 bytes. The partition offset is applied before any cache lookup (cache keys are physical LBAs).

## Layers (R20 build)

    channel / file read (fat16, win1)
          |  r20_file_read: probe on a copy, commit only on success
          v
    r20c_read / r20c_write  (sector cache, kernel/r20w.c)
          |  miss -> fill request
          v
    storage worker job (ring 0, kernel/r20w.c)  -- takes the storage claim
          v
    disk_read_worker / disk_read_body / disk_write_body  (platform/pc/disk.c)

`platform/pc/disk.c` renames the original routines under `QDOS_R20` (`disk_read_sector` becomes `disk_read_body`) and
exposes wrappers that wait for the claim and keep the cache coherent. Non-R20 builds use the original routines directly.

## Sector cache: contract

- Budget: `min(5 % of usable RAM, 2 MiB)`, counting entry, bucket and queue metadata. Limits are in
  `docs/R20C-LIMITS-INVENTORY.md`.
- Entry states: FREE, LOADING, VALID, ERROR. Each entry has a generation, pin count, reference bit and reservation mask.
- Lookup is hashed by (device, LBA). Simultaneous misses to one sector share one fill (coalesced).
- Eviction is clock (second chance), bounded to two sweeps. LOADING and pinned entries are never evicted.
- Writes are synchronous: a write invalidates the sector before and after the disk write, whatever the outcome.
- Mount or media change calls `r20c_invalidate_all()`; orphaned fills finish into dead generations.
- Reservations: a completed fill is kept for the waiting reader until the reader consumes it (at most `R20_RSV_MAX` per
  reader). A read that needs more sectors than the cache can hold, or that cannot get a victim while it holds
  reservations, reads the sector directly and is counted (`R20_STAT_SYNC_FALLBACK`), so the cache cannot deadlock.
- Retries: at most `R20_MAX_MISS` restarts in a row per process, then the read fails. Cache-full waits are not device
  errors and do not consume that budget.

## Per-function contracts

Signatures are from `kernel/r20w.h`; behaviour notes are from the comments in `kernel/r20w.c`.

| function | purpose | lock and interrupt state | blocking | failure codes and callers |
|---|---|---|---|---|
| `void r20c_init(void)` | size the cache from firmware RAM (capped at `R20C_BUDGET_MAX`) and allocate it from the heap | boot, interrupts off | no | none returned; called from `shellboot.c` before the mount |
| `int r20w_start(void)` | create the storage worker job, after the APs are up | boot | no | result as in `r20w.c`; called from `shellboot.c` |
| `int r20w_trap(unsigned fn, unsigned a, unsigned b, int caller_job, int *sleep)` | trap entry for `QT_R20_*` (0x40–0x45); returns the result and sets `*sleep` when the caller must sleep | BKL held, interrupts off (trap) | returns `*sleep = 1` for a sleep; the caller re-executes on wake | `R20_ERR_*`, `R20_RETRY`; called from `jobs.c` trap dispatch |
| `void r20w_job_gone(int slot, unsigned id)` | a job slot is being freed: abandon its requests, waits and reservations | BKL held, interrupts off | no | none; called from `jobs.c` reclaim |
| `int r20w_is_worker(int slot)` | is this slot the storage worker (EFLAGS.IF is managed around its claims) | any | no | 0 or 1 |
| `unsigned long r20w_stat(int idx)` | counter read for diagnostics (`R20_STAT_*`) | any | no | index values as in the `R20_STAT_*` enum |
| `int r20_file_read(struct fat16_file *ff, unsigned char *ubuf, unsigned n)` | file read in two steps: probe on a copy, then commit or restart. The user buffer is written only on commit | BKL held (trap) | may return `R20_RESTART`, which rewinds the caller's trap | `R20_RESTART` (-101) to re-run; other negative FAT codes; called from `win1.c` |
| `int r20c_read(unsigned lba, unsigned char *buf)` | probe: a hit or a miss; non-probe: a hit, or a disk read that keeps the sector | BKL held (trap) | a miss returns to the caller, which arranges the restart | `FAT_ERR_MISS` (-100, kernel-internal) for a probe miss; called from `r20w.c` (inside `r20_file_read`) and from `disk.c` |
| `int r20c_write(unsigned lba, const unsigned char *buf)` | write one sector: invalidate, wait for the claim, write, invalidate | BKL held (trap) | waits for the claim (bounded: `R20_DEV_SPIN`) | device error codes; called from `disk.c` |
| `int r20c_wait_dev(void)` | wait (bounded) until the controller is free | BKL held in a sync trap; the worker clears the claim without it | bounded spin (`R20_DEV_SPIN`) | 0 when the claim is free; the timeout result is as in `r20w.c`; called from `disk.c` |
| `void r20c_invalidate_all(void)` | mount or media change: nothing cached is trusted | BKL held (trap or boot) | no | none; called from `win1.c` and `r20w.c` |
| `int disk_read_worker(unsigned lba, unsigned char *buf)` | the worker's raw read (no cache) | ring 0 job, BKL not held while polling | polls the device | disk codes; called from `r20w.c` through `disk.c` |
| `int disk_read_body(unsigned lba, unsigned char *buf)` | raw PIO read of one sector; the wrappers in `disk.c` take the claim around it | as the caller of the wrapper | polls the device | as in `disk.c` |
| `int disk_write_body(unsigned lba, const unsigned char *buf)` | raw PIO write of one sector; the wrappers in `disk.c` take the claim around it | as the caller of the wrapper | polls the device | as in `disk.c` |
| `unsigned long disk_sector_count_body(void)` | IDENTIFY DEVICE sector count | as the caller of the wrapper | polls the device | as in `disk.c` |

## Diagnostics

- `R20_STAT_DUMP` (35) prints entry states, waiters, queue, worker state and reservation counters to the debug port 0x402.
- `R20_STAT_MY_GIVEUP` (34) reports this job's give-up count.
- The test programs in `tests/a16/` read these; see `docs/BUILD.md` for building them.

## Known limits

- Cold streaming reads are slower with the cache than without it (measured: `docs/A16-CACHE-RESULTS.md`). This is a known
  regression, not an optimisation.
- Phase 2A large-set failures were reproduced; their root cause is not verified (`docs/A16-CACHE-RESULTS.md`).
- Under the injected-error runs the reader-visible error count is not one-to-one with device failures (documented there).
- Polled PIO only. No IRQ14, no DMA, no real-hardware qualification.
