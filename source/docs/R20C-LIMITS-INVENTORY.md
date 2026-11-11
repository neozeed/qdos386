# R20C limits inventory

Every hardcoded bound that matters for the supported R18/R20 PC builds, recorded as it is in the source. Nothing here was
changed by the consolidation; the values are the ones the frozen Phase 2B images use. "R20 value" is the value that the
supported build selects, worked out from the `#if` chain shown; the other values are the ones other targets select.

Counting rules used below: a *job slot* is an entry of the kernel job table (`jobs[]`); a *session* is a text or graphics
session of the virtual-console layer (`vcon`); a *channel* is an entry of the channel table; a *CPU* is a logical processor
of the topology; a *generation* is a counter bit field inside a handle, not a count of objects.

## Jobs, processes and CPUs

| bound | where | R20 value | other values | note |
|---|---|---|---|---|
| job table `JOB_MAX` | `kernel/jobs.h:25-31` | **24** (`R17_JOB_MAX`, selected by `QDOS_DYNPROC && QDOS_R17`) | 16 (DYNPROC without R17), 6 (R10 table) | The mission text says "16 job slots". In the R20 build 16 are the ordinary slots and up to 8 more are AP idle jobs (one per CPU beyond the BSP; `dead_mask < 32`). |
| `R17_JOB_MAX` | `kernel/jobs.h:25` | 24 (overridable with `-DR17_JOB_MAX`) | | Do not raise without a `dead_mask` check. |
| CPU slots `KX_NCPU` | `kernel/jobs.h:36-38` | **8** (`QDOS_R17`) | 2 | R17 targets up to 8 CPUs. There is no configurable 1/4/8/32-CPU build. |
| AMP worker count `MAXCPU` | `kernel/amp.c:56` | 16 | | AMP is not in the supported builds. |
| process child limit `PROC_CHILD_LIMIT` | `kernel/proc.h:50` | 8 unreaped children per parent | | `PROC_ERR_LIMIT` (-69) when exceeded. |
| private page tables per space `VAS_MAX_TABLES` | `kernel/vas.h:27-29` | 16 or 32 (see the `#if`) | | 16 private tables plus the directory. |
| paged processes `PG_PROC_MAX` | `kernel/paging.h:55` | 3 | | Live paged processes. |
| per-process heap `QDOS_PROC_HEAP_MAX` | `kernel/proc.h:21-24` | 64 MiB (R17) | 8 MiB | Virtual heap, frames are the real limit. |
| console job tracking `VCON_JOBS` | `kernel/vcon.c:20-22` | **16** (`QDOS_DYNPROC`) | 8 | Must match the job table (`JOB_MAX`). |
| R20 request table `R20_NQ` | `kernel/r20w.h:41` | 8 | | Fixed. |

## Channels, sessions and surfaces

| bound | where | R20 value | other values | note |
|---|---|---|---|---|
| channel table `CHAN_MAX` | `kernel/channel.h:17-24` | **16** (`QDOS_FS_WRITE`) | 8 | Channel ids are `(tag << 16) | index`; tags wrap to 1 at `CHAN_TAG_MAX` (0x7fff). |
| bytes per channel write `CHAN_MAX_WRITE` | `kernel/channel.h:25` | 256 | | |
| channel name `CHAN_NAME_MAX` | `kernel/channel.h:27-29` | 72 (R11M) | 18 | |
| virtual sessions `VSESS_MAX` | `kernel/vcon.h:103` | 20 (0..3 legacy consoles, 4..19 private sessions) | | Each has a key queue `KEV_QSIZE` (below). |
| key events per session `KEV_QSIZE` | `kernel/kev.h:12` | 64 (oldest kept, newest dropped and counted) | | |
| INDEX8 surfaces `SURF_GFX_MAX` | `kernel/surface.h:90` | 4 | | Zombie surfaces `ZOMBIE_MAX` = `SURF_GFX_MAX + 4`. |
| registered fonts `KFONT_SLOTS` | `kernel/fontreg.h:17` | 8 (2 built in) | | |
| max font file `QFNT_MAX_FILE` | `kernel/fontreg.h:14` | 33,344 bytes | | |
| app profile/slot sizes | `kernel/appmap.h` | `APP_SLOTS` 2; slot sizes and profile sizes per `#if` | | |
| app image transfer entries `XFER_MAX_IMAGES` | `kernel/aout.h:18` | 4 | | |
| environment `ENV_MAX_VARS` / `ENV_BYTES_MAX` | `kernel/env.h:15-18` | 32 variables, 2048 bytes; name 31, value 255 | | |
| path candidates `PATH_CAND_MAX` | `kernel/pathres.h:19` | 128 | | |
| directory entries per FAT16 directory `DIR_MAX_ENTRIES` | `kernel/fat16.c:1155` | 65536 (FAT limit) | | |
| FAT16 clusters `FAT16_MAX_CLUSTERS` | `kernel/fat16.c:10` | 65524 | | Above this the volume is not FAT16. |

## Memory and frames

| bound | where | R20 value | other values | note |
|---|---|---|---|---|
| firmware memory map ceiling `MM_CEILING` | `kernel/memmap.h` | 256 MiB (`0x10000000`) | | `memmap.h` states the identity spans 0..63 (4 MiB units). |
| frame limit `FR_LIMIT` / frames `FR_NFRAMES` | `kernel/frames.h:15-19` | 256 MiB, 65,536 frames (R17) | 16 MiB, 4,096 frames | |
| memory-map records `MM_MAX` | `kernel/memmap.h:15` (`kernel/memmap.c`) | 32 | | |
| identity-mapped range | boot line of the R20 smoke run (16 MiB guest) | `identity 0..16 MiB, pool 3104 frames (0 above 16 MiB)` | | The R20 build reads the E820 map, so the pool grows with installed RAM up to `MM_CEILING`. |
| **R20 cache budget** `R20C_BUDGET_MAX` | `kernel/r20w.h:49` | min(5 % of usable RAM (`R20C_PCT`), **2 MiB**), counting entries, buckets and queue | | Measured sizes are in `docs/A16-CACHE-RESULTS.md`. |
| **R20 cache entry index** `R20C_MAX_ENT` | `kernel/r20w.h:47` | 4096 indices (12 bits); the budget gives about 3,900 entries | | |
| R20 minimum cache | `kernel/r20w.h:48` | 8 entries; below that the cache is disabled | | |
| **R20 completion reservations** `R20_RSV_MAX` | `kernel/r20w.h:54` | 4 per reader | | A 256-byte read needs at most 3 sectors. |
| **R20 restarts** `R20_MAX_MISS` | `kernel/r20w.h:56` | 64 in a row per process, then the read fails (bounded) | | Must not be raised to hide a stall. |
| R20 owner tag `R20C_OWNER` | `kernel/r20w.h:52` | 0x7E20 (heap owner, never a job id) | | |
| R20 storage claim wait `R20_DEV_SPIN` | `kernel/r20w.h:57` | 100,000,000 polls (sync trap, BKL held) | | |

## Application ABI and I/O

| bound | where | R20 value | note |
|---|---|---|---|
| one file read `QD_READ_MAX` | `sdk/include/qdos386.h:55` | **256 bytes** | Application ABI. Unchanged. |
| application stack `QDOS_APP_STACK` | build flag | 65536 bytes | `-DQDOS_APP_STACK=65536u` |
| trap vector | `kernel/r20w.h` (R20 comment) and the trap dispatcher | `int 0x30` (service traps `QT_*`) | |
| R20 trap numbers | `kernel/r20w.h:34-39` | `QT_R20_SUBMIT`..`QT_R20_STAT` = 0x40..0x45 | Internal; not an application ABI. |
| test-only switches | `kernel/r20w.c` | `R20C_INJECT_EVERY`, `R20C_TEST_ENT` | Off by default; used by the archived injection and contention runs. |

## Candidates for a future change (documentation only; not authorised here)

- Make the job-table bound a single documented constant that the console table (`VCON_JOBS`) derives from.
- Derive `KX_NCPU` from the topology instead of the `QDOS_R17` switch (only after the AP and dead-mask invariants are
  written down).
- Remove the `R20_MAX_MISS` ceiling only with a stall-detection design, not by raising it.
