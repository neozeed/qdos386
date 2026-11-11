# Apollo 16 / R20 - Phase 2A: restartable sector-cache miss

Status: implemented and measured on the R20 PC targets (2 CPUs and 1 CPU). Opt-in (`make qdos386-r20-pc`). Frozen R18
images are byte-identical to `scratchpad/r20-ident/r18-baseline.sha256`.

## What was built

Option 1 from the Phase 2 directive: a constrained restart, not mid-trap suspension, and no scheduler change.

- **Probe on a copy.** `win1_read` on a file channel calls `r20_file_read`. The file cursor is copied, and the whole read
  runs on that copy into a 256-byte kernel buffer (`QD_READ_MAX` is the kernel's per-call limit). Nothing user-visible
  changes during the probe.
- **Miss means restart.** A missing sector returns `FAT_ERR_MISS` through `rd_sector` and `fat16_read`. The caller is put
  to sleep on a sector-cache entry (`r20c_block`), and `chan_read` returns `R20_RESTART`. `QT_CH_READ` in `jobs.c` then
  moves the saved user EIP back over its own two-byte `int $0x30` and schedules. When the caller wakes, the same call runs
  again with the same registers, and EAX is never written by the restart.
- **Commit once.** Only a complete, successful probe commits: the cursor is copied back and the bytes are copied to the
  user buffer. A read is never applied twice.
- **Sector cache.** Eight 512-byte kernel-owned entries keyed by LBA, in states FREE, REQ, LOADING, VALID, ERROR, with
  generation numbers and an LRU clock. Cache hits, and sync reads, go through `r20c_read`.
- **Worker fills.** The Phase 1 Ring 0 worker takes a cache fill from `QT_R20_TAKE` (a ticket with `R20_CACHE_BIT`). It
  claims the controller (`r20_claim = 1`) and its frame's IF is cleared, so no timer interrupt can need the BKL during the
  claim. It reads the disk with the BKL released, then `QT_R20_DONE` publishes the entry (VALID or ERROR), restores IF,
  and wakes the sleepers on that entry.
- **Sync traps wait for the claim.** Any non-probe disk access (directory, FAT open, seek, write) spins on `r20_claim`
  with the BKL held, bounded by `R20_DEV_SPIN`. The worker clears the claim before it needs the BKL for DONE, so this
  cannot deadlock.
- **Writes invalidate.** `r20c_write` waits for the claim, invalidates any cache entry for the sector (generation bump, so
  an in-flight fill of the old data is discarded at its DONE), wakes its waiters, and then writes synchronously. Writes
  are not restartable and were not made so.
- **Bounds and cleanup.** A process may restart at most `R20_MAX_MISS` (64) times in a row, after which the read fails
  with `FAT_ERR_IO`. Job reclaim clears its wait record (`r20w_job_gone`). Wakeups check that the sleeper still owns its
  slot and still sleeps on that entry.

Not restartable and left synchronous: directory operations, open, seek (`fat16_seek` walks the chain through sync reads),
and all writes. Only `QT_CH_READ` on a file channel can restart.

## Test programs (tests/a16)

- `r20f.c` (R20F sequential; R20FR random): reads `WIN1:/PAT.BIN` (65536 bytes, P(i) = (i*37 + (i>>8)*11 + (i>>16)) & 0xff),
  pinned to CPU 0. Sequential reads 256-byte calls and verifies every byte, then checks EOF. Random does 300 seeks to LCG
  positions, each followed by a verified read of 1..256 bytes.
- `r20p.c` (R20P): unrelated background application pinned to CPU 0. Three million cheap kernel calls, with a progress
  line every 65536 calls and its tick.
- Sequences: `p2a.seq` (R20) and `p2a-r18.seq` (R18 control, same volume): LEAK; start R20P; reader; reader; wait 15 s; LEAK.
- Volume: `scripts/a16-p2a-sysvol.sh`, a copy of the Apollo 15 test volume with PAT.BIN and the protected binaries added.
  The base volume is not modified.

## Results (evidence in docs/evidence/a16-phase2a/)

Exact contents, all runs: **zero mismatches**, EOF correct, no fill errors, no give-ups.

| Run | Sequential (65536 B, 256 calls) | Random (300 seeks, 39820 B) |
|---|---|---|
| R20, 2 CPUs | window 843..1107 (264 ticks); 133 misses = 133 fills = 133 restarts = 133 wakes | window 1248..1997 (749 ticks); 492 cumulative misses |
| R18, 2 CPUs (control) | window 828..838 (10 ticks), too short to measure overlap | window 980..1288 (308 ticks) |
| R20, 1 CPU | PASS | PASS |

Background progress (R20P, pinned to CPU 0, reader pinned to CPU 0):

- R20, 2 CPUs: during the sequential window, 327,680 calls (about 1,240 per tick). During the random window, 1,048,576
  calls (about 1,400 per tick). That is essentially the unloaded rate (R20P finished 3,000,000 calls at tick 2386).
- R18, 2 CPUs: during the random window, 65,536 calls (about 210 per tick), roughly 15-20% of the unloaded rate. The
  synchronous reader holds the BKL through each disk wait, so CPU 0 is held in the kernel for that time.

Two honest notes on these numbers:

- The restartable read is slower in wall time. The sequential reader spent 264 ticks on 133 misses, about two ticks per
  miss. The likely cause is that the worker, woken on CPU 1, is only dispatched at the next timer tick. This was not
  separately measured. On this emulated IDE disk the synchronous path costs only a few ticks in total, so the benefit
  appears as the protection of the other application's progress, not as an absolute speed-up.
- The reader's random window includes synchronous seeks, so the random numbers are not purely the async path.

Phase 1 regression on the R20 kernel, 2 CPUs (tests/a16/p3.seq, sequence unchanged): functional 40/40 OK, 300 race pairs
bad=0, stale ticket -41, backpressure accepted 8 / refused 4, background calls bad=0, exit while active abandoned=1,
worker blocked-idle at the end, and the heap unchanged across the run (one value, 622224). The Phase 1 1-CPU regression
was not rerun for this checkpoint.

## Limitations (not tested or not implemented)

- Only `QT_CH_READ` on a file channel is restartable. Every other FAT path is synchronous under the BKL.
- Writes are not audited for restart. They invalidate and wait only. They need a separate audit before any change.
- No timeout fail-stop policy beyond the bounded spin and the restart bound. The disk poll limit is the only timeout.
- No error injection (a failing sector read was not forced), and no test of killing a process while it sleeps on a fill.
  Those paths are implemented (`r20w_job_gone`, the ERROR state) but were not exercised.
- Measured on one emulated IDE disk and one test file. No 8-CPU run, no other memory sizes.
