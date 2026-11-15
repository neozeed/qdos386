# R7B2 F4 combined system disk — PASS

Prerequisites are accepted at `apollo-r7b2-fat32-host` (`90f5754`) and
`apollo-r7b2-fat32-native` (`4ad51a7`). Evidence remains under
`/tmp/qdos386-r7b2-fat32-glru8dtm/f4-system/`.

The new `make apollo-r7-rw` profile selects the qualified mutation implementation
and FAT32 WIN1. It does not enable the prohibited `QDOS_APOLLO_FAT32_WRITE` flag.
At boot, validated partitions on the BIOS-selected device supply writable
FAT32 WIN1 and read-only FAT16 WIN0. If FAT32 validation fails, FAT16 remains
WIN1. Physical-LBA cache keys, disk ownership, SDK and filesystem ABIs are
unchanged. The bootloader, MBR loader gap and FAT16 VBR are preserved.

The disk has one active FAT16 partition at sector 2048 (65536 sectors) and one
FAT32 partition at 67584 (196608 sectors), total 129 MiB. FAT32 uses two mirrored
FATs, one sector per cluster and root cluster 2. The qualified shared writer
populates payload starting at cluster 70010. All 92 recovery payload files are
copied byte-for-byte with their long-name references, plus the F32LIVE probe.
Historical guest GCC, games and assets are reused explicitly. Fresh kernel,
FDISK/FORMAT/CHKDSK and probes are identified in the manifests; fresh shell and
SH hashes exactly equal the reused payload copies.

| Acceptance | Result |
| --- | --- |
| One-disk boot; FAT32 WIN1, FAT16 WIN0 | PASS, 1 CPU/16 MiB, 4 CPUs/32 MiB, 8 CPUs/256 MiB |
| Native GCC/HI on FAT32 | PASS on all three configurations; HI SHA-256 exactly equals accepted C4 output |
| Native SH and EXIT | PASS on FAT32 |
| Public filesystem API | PASS: create/write/close/reopen, append/seek/shrink/zero extension, rename/delete, directories, exclusive opens, process-exit flush, cache invalidation |
| Eight readers plus public-channel writer | PASS: each reader checks two full 1.5 MiB PAT2 passes while the parent performs 32 write/read/delete cycles |
| Resource recovery | Exact 6734/6734 free frames, 643536/643536 heap bytes, 2/2 live processes; zero pins/reservations/loading/queue/claim/waiters; reservation set/release both 48796 |
| Recovery namespace | WIN0 reads succeed, creation returns write-protected (-80); FAT16 partition unchanged during every normal FAT32 run |
| Mount-failure recovery | Invalid FAT32 primary boot signature is refused; FAT16 WIN1 runs GCC/HI and channel tests; invalid FAT32 partition remains exactly unchanged |
| Disk tools | FDISK reports both partitions; FORMAT and CHKDSK repair refuse the live system disk, retaining exit statuses 1 and 3 |
| Existing applications | Bounce 180 frames: 45 shown/135 hidden, zero errors and exact model checksum; Hack starts/quits; Elite starts/renders/quits with graphics/task switching |
| Reboot readback | 1/4/8 CPUs execute previously compiled HI and read all 700 owner-exit bytes; reboot disk images are byte-identical before/after |
| Independent post-shutdown judges | All valid FAT16/FAT32 partitions pass Linux fsck and independent ownership/chain/content decoding; no repairs applied |

HI SHA-256: `5f2973ad508006aeefe79e3e4c0f0c5675e3cb4c25d773fc4d372c3c85a7820f`.
Pristine combined disk:
`products/qdos386-r7b2-fat32-v3-master.disposable.img`, SHA-256
`34ec7a9a0fc8dbb4f43bedf72de9fb2672d4193b3e5b725cdeab2ecde4d56074`.
The accepted R6A fallback is unchanged, SHA-256
`3b3ceb41210c9726af2cd65c55225a5571f9df7193c96d722ce017506560396a`.

Receipts: `system4-v3.json`, `more-system.json`, `judgment-*.json`,
`native-hi-comparison.json`, `reboot1-readback-judgment.json`, `reboot48.json`,
`apps-interactive.json`, plus exact QEMU commands, driver inputs, screenshots,
fsck outputs and independent decodings in `logs/`. All raw failure evidence is
retained: the first payload omitted two SDK long names, fixed in subsequent
manifests; the first kernel link needed an explicit freestanding byte-copy
helper instead of an implicit hosted memcpy; the first Hack script sent input
through a More prompt; TYPE's non-newline output defeated the initial prompt
regex. The latter readback was independently certified from all 700 bytes and
unchanged whole-image hashes without rerunning it.

Elite's initial 30 ms automation presses were missed by its polled input.
Observed title prompts were answered N then X using 300 ms holds, followed by
backslash, Q, Y. The task manager then showed seven baseline jobs, no Elite.
No application sources were modified. The original user manual acceptance and
all earlier timeouts remain historical records, not overwritten by these runs.

Reproduction uses `validation/r7/fat32-combined.py` from an isolated source
copy, with the pinned fallback, fresh package/disk tools, F32IO/F32LIVE probes
and `fat32-populate` host utility. The original input disk is never writable.
F5 will verify a fresh extraction and byte-identical package/disk reproduction.
