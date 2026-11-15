# R7A native disk-tools checkpoint

Accepted within the limits in `source/apps/disktools/README.md`: native FDISK
inspection/staged primary-table changes, FAT16/FAT32 data-partition FORMAT,
read-only CHKDSK and interactive advisory FSInfo repair. This does not accept
FAT32 as a mounted kernel filesystem or a combined system disk.

Source baseline is `da9d217`; Gate 0 receipt is `0f9be7f`. Development is on
`r7-native-disk-tools`; the safety branch retains the original HEAD. All builds,
fixture construction and QEMU runs were confined to
`/tmp/qdos386-r7-1wn7185l/`. No source-layout changes, historical application
edits, deletions, package installations, network access or publication occurred.

`make apollo-r7 HOSTTOOLS=...` adds the private maintenance ABI; `make disk-tools
HOSTTOOLS=...` builds the SDK dependencies and three native tools. Use these
commands only in an isolated filesystem copy. Original R5/R6A profiles remain
available; all 19 baseline comparison hashes still match after these changes.

The shared C89 metadata engine uses the production FAT sector callback ABI.
Maintenance operations check device identity, 512-byte/LBA28 geometry and
partition bounds. The kernel serializes controller selection against the
storage worker and restores the boot selection on every return. Unmounted
secondary transfers bypass the boot-device cache. Whole-live-disk writes are
refused. Boot-code/signature preservation and stale-table checks are explicit.

Evidence includes 44 bounded host invocations, malformed MBR/FAT cases,
read-only/no-write invariants, partial I/O failure, independent Linux formatting,
MBR/VBR preservation and refusal of reserved loader payloads. Native QEMU runs
created primary partitions, formatted FAT16/FAT32, scanned both, refused live
boot writes, and exercised eleven direct ABI identity/bounds/claim/pointer
checks. Four-CPU worker/GCC/HI and visible/hidden Bounce regressions passed.
Independent Linux fsck passed the resulting data volumes.

Native CHKDSK N/Q/Enter/EOF left the deliberately corrupted image byte-identical.
Y/A repaired only the advisory FSInfo sectors; each repaired partition passed
Linux fsck. `A` is scoped to that safe class; any structural error refuses
automatic repair. The initial EOF harness exposed raw Ctrl+D being returned as
a byte rather than stream EOF. The utility's bounded input routine now handles
Ctrl+D/Z explicitly; the new native EOF check passes. A separate Linux fixture
exposed an overly strict allocated-cluster search-hint check. The corrected
checker accepts valid-range advisory hints without requiring them to be free;
final binaries passed a focused native Linux-volume and ABI qualification.
Failed first attempts and their logs remain in the workspace.

Exact artifact hashes, commands, prompt results and preservation inventory are
in `R7A-ACCEPTANCE.json`. The private qualified utility disk is:

```
/tmp/qdos386-r7-1wn7185l/products/r7a-tools-qualified.img
SHA256 0b42d9d1503a37f3695b47a4be99b083a5e6450a7133d383de1a286a37397edc
```

The unchanged accepted FAT16 fallback remains:

```
/tmp/qdos386-r7-1wn7185l/checkpoint/fat16-fallback.img
SHA256 3b3ceb41210c9726af2cd65c55225a5571f9df7193c96d722ce017506560396a
```

Next is B0/B1: review the locally preserved FAT32 extension against the current
shared file engine, supply valid read-only close callbacks, reject invalid
driver registrations, and run the smallest real guest cleanup reproducer.
Do not enable FAT32 writes until read-only guest acceptance passes.
