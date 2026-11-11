# Apollo 16 Phase 2B: the bounded sector cache

Status: implemented under `QDOS_R20`; the frozen R18 kernel is byte-identical and the Phase 2A tree is unchanged.
Builds on Phase 2A (`a16-gate2a`, `cf1cbcc`), the probe-and-restart file read on the Ring 0 disk worker.

## Scope

A read-through, write-coherent sector cache between the file layer and the storage worker. It does not change the
application ABI (the 256-byte `QD_READ_MAX` read cap, the ticket and file-handle formats), does not make writes
asynchronous, and does not touch FAT32, DMA, IRQ14, the display worker or the scheduler.

## Sizing

- Usable RAM comes from `memmap_summary` (valid only after `pg_init`), so the cache is created after paging is set up
  and before the first mount.
- Budget = min(5 % of usable RAM, 2 MiB). Data and metadata both count. Entries are 512-byte sectors; the metadata
  per entry is the `struct r20_ce` record plus its hash bucket and queue slot.
- The entry index is 12 bits (at most 4096 entries). Below 8 entries the cache is disabled and file reads run
  synchronously, as in Phase 2A.
- The cache is one kernel-heap allocation (`mem_alloc`, owner `0x7E20`), made once at boot.

## Keys, states and generations

- Key: physical LBA (the partition offset is applied above the cache). Device identity is fixed at 0 (the single IDE
  slave); the field is kept for a future second device.
- States: `FREE`, `LOADING` (queued, or being filled by the worker), `VALID`, `ERROR` (a device failure, reported once).
- Every entry has a generation. A fill's ticket carries the generation; a completion whose generation no longer matches
  is an orphan (the sector was invalidated while in flight) and publishes nothing.
- Lookup is hashed (power-of-two buckets). Eviction is a clock with a bounded two sweeps; a full cache with every entry
  loading, pinned or reserved returns "no victim", and the caller waits (or reads directly, see below).

## Coherence

- Every sector write goes through one choke point (`r20c_write`): the entry is invalidated before and after the disk
  write, and an in-flight fill of the old contents is discarded at its completion.
- `win1_mount` and media change call `r20c_invalidate_all`; nothing cached is trusted across a mount.
- Write coherence is tested by `tests/a16/r20c.c` (rewrite, partial overwrite across a sector boundary, remove and
  reuse, extend, and a readback after reboot).

## Read path

1. A file read runs as a probe on a copy of the file cursor into a 256-byte kernel buffer. Nothing user-visible changes
   during the probe.
2. If every sector the read needs is VALID, the probe commits: the cursor is updated and the data copied to the user
   buffer in one step (cursor atomicity as in Phase 2A).
3. If a sector is missing, the probe returns `FAT_ERR_MISS` (kernel-internal). The reader queues the fill (or joins a
   fill already in flight, a coalesced miss), sleeps, and is re-executed from the start when the fill completes.
4. A fill completes on the worker (ring 0, interrupts off during the controller access, no BKL held while waiting).
   The completed sector is VALID and reserved for every reader that was waiting for it (see below).

## Reservations (the cache-lifetime rule)

A sector that a waiting reader has just filled must still be there when that reader retries. Without a reservation a
busy cache can evict it first, and the whole-operation restart can repeat indefinitely. That is the failure this rule
removes.

- At fill completion each reader asleep on that entry gets a reservation bit (one bit per job slot). An entry with any
  reservation bit is not evicted.
- A reader holds at most `R20_RSV_MAX` (4) reservations. A 256-byte read touches at most three sectors (two data
  sectors across a 512-byte boundary, and one FAT sector for the next cluster), so four covers any single read. The
  list is generation-safe: a reservation whose entry has since been freed or re-filled is stale and is skipped.
- A reader keeps its reservations across restarts, so a restarted read makes progress: each restart adds at most one
  sector and the reader never needs to find those sectors again. The reservations are released when the read commits,
  when it fails, when the process ends, and on invalidation (the entry is freed and the bits are cleared).
- Multiple readers may share one fill. Each keeps its own bit and releases only its own.

### Deadlock avoidance

A reader that already holds reservations never waits for a victim. If it misses, no entry is free, and it cannot take
another reservation, the probe reads that one sector directly (uncached, under the BKL, as the synchronous path does).
Such a read completes, commits, and releases its reservations. A reader with no reservations may wait for a victim
(`NOSPACE`, backpressure); that wait does not use the retry budget, because every wake-up means an entry has just
become free. Reserved readers therefore never sleep on victims, so no cycle can form. Their only waits are on fills
that the worker is already processing.

## Retry budget and errors

- `R20_MAX_MISS` (64) is the number of consecutive restarts one process may take before its read fails with
  `FAT_ERR_IO`. It is unchanged. With reservations, every restart adds a sector, so a read that makes progress cannot
  reach it; the budget is a safety net against a pathological loop, not a normal path.
- Device failures are not restarts. A fill that fails at the device goes to `ERROR`, is reserved for its waiters, and
  is reported to the first reader that probes it (`FAT_ERR_IO`, once). The other waiters retry and fill again.
- A waiter that cannot take a reservation (its list is full) is not protected; if its sector is evicted before it
  retries, the retry simply fills again. No successful retry is reported as an error.

## Termination

A process that ends while it has queued, active or waiting fills: `r20w_job_gone` releases its reservations and drops
its waits. The fills themselves belong to the cache, not to the process: they complete into kernel memory and are
published (or discarded, if orphaned) with no reference to the process's buffers. A process's buffer is written only
by the process's own trap, from the kernel copy, at commit.

## Diagnostics

`R20_STAT_DUMP` (stat index 35) prints, on the debug port: entry states, queue depth, worker state, claim, waiters
(per job, with what each waits for), the counters (hit, miss, fill, coalesced, evict, nospace, restart, device
failures, reservations set/released/full, sync fallbacks), and the reservations still held. At quiescence `held` and
`bits` must be zero and `set` must equal `release` plus entries freed by eviction or invalidation.
`R20_STAT_MY_GIVEUP` (34) gives a job its own give-up count.

## Test-only switches (not in the release build)

- `R20C_TEST_ENT=N`: cap the entry count (N = 8 gives the contention configuration).
- `R20C_INJECT_EVERY=N`: every Nth fill fails at the device, alternately as an IDE error (-1) and a timeout (-2). The
  sector is not read. This simulates the device result; it does not model the controller's timing.
- `R20_RSV_MAX=1`: forces the direct-read fallback path.
