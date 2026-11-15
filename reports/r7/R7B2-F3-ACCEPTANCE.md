# R7B2 F3 native secondary-volume qualification — PASS

F2 checkpoint: `apollo-r7b2-fat32-host` (`90f5754`). Native evidence is retained
under `/tmp/qdos386-r7b2-fat32-glru8dtm/f3-native/`. The accepted GCC 1.40 SHA-256
is `06371b4e8765b296c0082f89eecea73df441a831550dd7e294c8780981abfafc`.

Fresh isolated `make apollo-r7-ro disk-tools` builds and SDK-linked native
probes completed. The kernel's production FAT32 namespace remained read-only.
The new shared writer ran as actual i386 code on a separately claimed,
previously unmounted IDE1 FAT32 volume through the existing R7 disk-maintenance
ABI. No raw port access, boot-disk write bypass or public ABI changes were added.

| Native qualification | Result |
| --- | --- |
| Complete mutation exercise, 1/4/8 CPUs, 32 MiB | PASS; all resulting FAT32 images exactly match the independently qualified host image |
| Reboot on 1/4/8 CPUs | PASS; reopened final file and checked all 2000 bytes |
| High cluster 70010 / root cluster 7 | PASS, including full-width creation, append, shrink, zero extension, directory growth, nested paths, rename/delete |
| Active FAT1, mirroring disabled | PASS with the same documented checker limitation; entire output matches the retained host counterexample and inactive FAT remains unchanged |
| Native injected failures | **116 PASS**: 24 creation cases (0/256/512 bytes) and 92 further logical metadata-boundary cases (256/512 bytes) |
| Disk owner process exit | PASS; a process leaves its claim unreleased, and the next process successfully claims and mutates the volume |
| Eight-reader contention | PASS; eight native file-channel readers each verify two passes over the 1.5 MiB PAT2 file while the parent mutates FAT32 on the other device |
| Public channel / cache regression | PASS on FAT16 system volume: exclusion, write/close/reopen, append/seek/truncate, mkdir/rename/delete, process-exit flush and cache invalidation |
| Foreground switching | PASS during the public-channel test; content and completion checks still pass |

The common complete-mutation output SHA-256 is
`cdd9060262618848914c9ccb89429ecfec8fcf44a5e9d821470ea1cc05f6b37e`.
Normal mirrored outputs pass both independent decoding and Linux checks. All
116 injected-error outputs equal their corresponding host images byte-for-byte;
their actual checker statuses and structural damage are retained. They are
error-propagation tests, not claims of clean filesystems after torn writes.
Native partial-sector injection uses a real sector read/modify/write followed
by an explicit error return, reproducing the selected torn-sector bytes.

Contention returned exactly to **6722 free frames, 645184 heap bytes and two live
processes**. Final pins, held reservations, loading entries, queued entries,
waiters and device claims are zero. Reservation set/release counters both equal
47162; all eight readers exit zero. The FAT16 boot image is unchanged by this
secondary-volume run. The separate public-channel test records 107 cache
invalidations and independently verifies the 700-byte file flushed on owner exit.

Two harness failures are preserved and explained: requesting nine direct children
exceeded the unchanged eight-child ABI limit, so the parent now performs writes;
the initial 6000-tick read deadline expired before the approximately 6476-tick
workload finished, so the same full workload was rerun with a bounded 12000-tick
deadline. No kernel, filesystem or compiler workaround was introduced for either.

Receipts: `native4-judgment.json`, `native-more.json`, `native-metadata.json`,
`native-active.json`, `contention-120.json`, `channels.json`,
`independent-postchecks.json`, plus per-run commands, debug logs, images and judges
under `logs/` and `products/`. Reproduction drivers are retained as
`first-native.py`, `more-native.py`, `metadata-native.py`, `active-native.py`,
`contention-120.py` and `channels.py`. The permanent C probes are in
`validation/r7/fat32-native-{writer,contention,channels}.c`.

F3 deliberately separates the unmounted secondary FAT32 engine from the live
FAT16 kernel namespace. The disk-worker contention exercises both devices;
public-channel cache invalidation/owner cleanup exercises the existing mounted
filesystem path. **Writable FAT32 as the mounted WIN1 namespace is still F4**,
including its public-channel and application tests. No combined disk was accepted
as part of this checkpoint, and no old acceptance reference was moved.
