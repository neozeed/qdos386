# R7B2 F2 host qualification — PASS

Accepted with the user-approved active-FAT checker compatibility interpretation
in `R7B2-ACTIVE-FAT-DECISION.md`. This is host acceptance only; F3/F4 remain gated.
No kernel source repair was needed during this resumed qualification.

Existing twelve successful matrix records and the root-7/high-70010 cases were
retained. New evidence is under
`/tmp/qdos386-r7b2-fat32-glru8dtm/f2-resume-jcqdsh61/`.

| Coverage | Result / evidence |
| --- | --- |
| Active FAT1 | All seven compatibility conditions pass; both Linux exit-1 results explicitly retained; inactive FAT unchanged |
| Fragmentation; clusters 65535, 65536, 131071 | Exact content, ownership and Linux checks pass |
| Last cluster 161287 | Allocation at final valid cluster passes both judges |
| Full / two-cluster-near-full | FULL reported, published prefix readable, deletion restores free space; clean judges |
| Malformed file chains | Loop, free, reserved, bad, out-of-range and premature EOC refused without writes; malformed input retained |
| Long names / rename | Successful operation passes independent decoding and Linux checker |
| Read-only media | Refused, image byte-identical |
| WIN1 open exclusion | Real driver registry rejects conflicting readers/writers, truncate, rename and delete; releases correctly |
| FAT16 regression | Full mutation exercise, Linux checker and strict independent FAT16 walker pass |
| Fresh-process remount | 65536/131071 contents read back exactly without image changes |
| Cancellation | Eight abrupt process exits at creation write boundaries match independently judged partial-operation images exactly |
| Injected write errors | **1320** cases: all 440 writes across create/exercise/long-name sequences, each failing after 0/256/512 bytes |

For each injected error, the actual engine reports WIO, flush reports WIO, the
next attempted mutation reports WIO and issues zero further writes. Every byte
of the output equals the successful callback prefix plus the injected partial
sector. Existing fragmented-file content remains unchanged. Both independent
filesystem judges are recorded on the unmodified output. **715 cases contain
recorded structural damage**; they are passing error/prefix tests, not clean
filesystems. Mirrored-copy disagreement, orphan allocation, interrupted size or
directory publication and interrupted rename are non-journaled failure states.
They are never accepted as successful mutations or repaired before inspection.

The failure suite exposed a test-decoder gap: it ignored unpublished directory
entries beyond an end marker. The decoder now separately examines these slots
and reports hidden aliases/crosslinks. The original disagreement at
`faults/longname-0019-0.*` is preserved, including the original decoder output,
Linux diagnosis, and the corrected diagnosis. All 1320 existing outputs were
rejudged with the strengthened decoder without rerunning their mutations.

The host comparison harness initially exhausted tmpfs by mapping sparse holes.
No kernel ran during that interruption. Streamed reads replaced these mappings;
zero extents of this run's images were deallocated with before/after SHA-256
equality, with no file removed. The interrupted run and progress receipts remain
preserved. Completed mutations were resumed rather than restarted.

Key receipts: `results.json`, `faults/results.json`,
`faults/rejudge-strengthened.json`, `cancellation-remount.json`,
`fat16-regression.json`, and `logs/exclusion.*`. Reproduction scripts are
`validation/r7/fat32-resume-f2.py`, `fat32-faults.py`, `fat32-mutation.c`,
`fat32-open-exclusion.c`, and `fat32-decode.py`. Run scripts from isolated `/tmp`
copies; do not rerun the original matrix over its retained output directory.

No claim is made for all possible 28-bit volume sizes: physical test boundaries
include 131071 and the final cluster of the 80 MiB fixture. Geometry and file-size
arithmetic remain bounded. This acceptance does not claim power-loss atomicity,
automatic repair, native runtime acceptance or a writable system disk.
