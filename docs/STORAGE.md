# Storage, filesystem and worker contract

> Preserved R3 baseline document. For current R4 resource changes and profile status, see [R4 configuration](R4-CONFIGURATION.md), [ABI review](R4-ABI-REVIEW.md) and [known limitations](R4-KNOWN-LIMITATIONS.md). The R3 text below is retained for provenance.

`channel.c` validates owned tagged handles and caller pointers; `win1.c` owns the mounted WIN1 state and per-job directory context; `fat16.c` parses FAT16/VFAT, cursors and metadata. The PC disk backend performs bounded polling PIO on the existing IDE slave. No FAT32, DMA, IRQ14 redesign or new storage API is added.

Mount and synchronous file/metadata operations run in serialized kernel context. Callers must preserve their owned cursor/channel and valid buffers until completion. Reads return byte counts, EOF or existing negative errors; partial counts are meaningful. Writes remain synchronous and invalidate affected cached sectors before and after the write; mount/media reset invalidates all cached trust. Existing filesystem synchronization semantics and explicit SYNC behavior are unchanged.

IOW0 is an ordinary Ring 0 kernel job with its own supervisor stack, started after AP topology is known. Its internal request table has eight slots. Submit returns a generation-tagged ticket or FULL; wait sleeps/retries; only the worker may take/info/complete its active ticket. Completion/abandonment/wakeup transitions occur under BKL. The worker's PIO body holds a storage claim without BKL and clears the claim before DONE reacquires BKL. A sync trap may poll that claim without causing a BKL cycle.

## Restartable reads and cache

A file read probes a copy of the cursor into bounded kernel storage. On a miss it queues/joins a fill and sleeps; the saved trap EIP is rewound to retry. Only a complete probe commits cursor advancement and copies bytes into user memory. Worker fills belong to the cache, never the process buffer, so process death cannot leave a dangling write target.

Entries use hash lookup, bounded clock eviction, fill generations, worker pins and per-job reservations. Loading/pinned/reserved entries are not victims. Completed fills reserve contents for waiting readers across restart; release occurs on commit/error/invalidation/death with generation checks. Readers holding reservations use the original direct-read fallback rather than waiting cyclically for an unavailable victim. Keep the existing 64-miss retry bound and four-reservation bound; increasing them is not a cleanup fix.

Backing ownership is in MEMORY. Process reclaim calls r20w_job_gone to abandon worker requests, remove waits and release reservations. Quiescent invariants are zero pins/loading/waiters/reservation bits/held lists and stable heap/frames. Tests retain the original eight-reader workloads, kill cases, write/reboot checks and explicit injected-device failures. Injection models returned fill errors, not real controller timing; previously documented superseded-retry observations and cold streaming limitations remain.

Accepted FIX1 and frozen disks are inputs. Every runtime test boots disposable copies/overlays; never attach a writable accepted disk directly. Shell/GCC/applications are read from that preserved volume, not silently rebuilt by the kernel build.
