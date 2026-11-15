# Apollo R7 handover: disk tools and FAT32 read-only accepted

The minimum R7 milestone is delivered. The full writable-FAT32/system-volume
moonshot is **not complete**. Stop at the accepted A/B1 checkpoints: B2 has a
preserved writer-corruption counterexample and B3 is not accepted. No push,
publication, package installation or real-disk access occurred.

## Source checkpoints

Canonical checkout: `/data/projects/qdos386`, branch `r7-native-disk-tools`.

- `da9d217`: original authoritative source, retained by `r7-safety-da9d217`.
- `0f9be7f`: verified Gate 0 receipt; earlier read-only Git blockage resolved.
- `3e49154`: R7A native disk tools, retained by `r7a-disk-tools-checkpoint`.
- `3aa5998`: B1 read-only FAT32, retained by
  `r7b1-fat32-readonly-checkpoint`; this is the fresh-build source commit.
- The final handover commit adds these receipts and hardens the **host-only**
  negative experiment's path checks. Production source/build inputs remain
  byte-identical to the fresh-build commit. Obtain the final receipt HEAD with
  `git rev-parse HEAD`; it cannot embed its own hash.

The user's original untracked instruction document remains untracked and
unchanged. No historical application source/assets/ROMs or existing resources
were removed or modified. Nine existing kernel/build files were intentionally
edited; new files are source, tests and documentation only. All generated
output is under `/tmp/qdos386-r7-1wn7185l/`. No accepted branch/tag was moved.

## Accepted behavior

Native FDISK stages standard primary FAT16/FAT32 partition edits and requires
explicit commit authorization. Native FORMAT creates supported FAT16/FAT32
data partitions. Native CHKDSK scans without writes, reports bounded metadata
errors, and offers `Fix (Y/N/A/Q)?` only for validated advisory FSInfo repairs.
N/Q/Enter/EOF make no changes; A is restricted to the FSInfo class. Ambiguous
chain/data repairs are refused. The whole live boot/system disk is protected
against maintenance writes. See `source/apps/disktools/README.md` for limits,
examples, confirmations and exit codes.

The optional `apollo-r7-ro` kernel mounts one FAT32 partition on the same
physical disk as read-only `WIN0:`/`WIN0_`; FAT16 remains `WIN1` and supplies
the shell, SDK and historical applications. The shared existing FAT engine
handles 28-bit clusters, high directory cluster words, cluster-chain roots,
active/mirrored FATs and bounded malformed-input refusal. Read-only close is a
valid no-op; invalid mandatory callbacks are rejected before channel creation.
Invalid optional FAT32 media leave FAT16 recovery usable. No FAT32 sector is
written during read-only mounts/reads, including FSInfo.

Six B1 cases cover Linux geometry, a nonstandard root cluster, fragmented
clusters above 65535, inactive-FAT divergence, malformed chains and bad FSInfo.
Native read/seek/close/reopen/exit cleanup and negative callback checks pass.
All test FAT32 partitions were byte-identical afterward. Mirrored valid cases
pass Linux fsck. The deliberately stale inactive FAT produces fsck status 1
(`FATs differ - using second FAT`); it is **not counted as fsck-clean**. Its
active-FAT-aware independent scan, actual native data checks and entire-image
hash pass. Corrupt fixtures retain their expected fsck errors without mutation.

## Builds and final verification

Use a copied/extracted source tree under `/tmp`, never the original checkout:

```sh
make apollo-r7-ro HOSTTOOLS=/tmp/qdos386-r7-1wn7185l/hosttools
make disk-tools HOSTTOOLS=/tmp/qdos386-r7-1wn7185l/hosttools
```

`make apollo-r7` builds the maintenance-only kernel. The default, explicit
64-job and R6A targets remain intact. The external historical host toolchain is
GCC 1.40/GAS 1.38/i386-ld 1.94; source/runtime hashes and discovery evidence are
retained in the workspace. System binutils build BIOS stages. No toolchain was
downloaded or installed.

`git archive 3aa5998` was extracted into `fresh-source/` and all seven root
build entry points passed, including SDK dependencies and native utilities.
The 19 prior kernel/shell/BIOS oracles match. R7 read-only kernel and utilities
match their qualified artifacts. The complete R6A fallback was recreated with
its exact accepted hash from fresh components and the explicitly identified
accepted private application volume. The populated application payload was
reused, not falsely claimed as rebuilt.

The final fresh four-CPU, 32 MiB, **one-drive** QEMU run passed FAT32 native
reads, FDISK inspection, CHKDSK read-only scanning, storage-worker tests,
GCC compile/link/run and visible/hidden Bounce switching. Both partitions pass
Linux fsck. All original FAT16 file hashes and boot structures are preserved;
only the expected test executable and Bounce log were created on the disposable
run copy. The final candidate image itself was not changed by testing.

Exact commands/hashes/results: `R7A-ACCEPTANCE.json`, `B1-ACCEPTANCE.json`,
`FINAL-VERIFICATION.json`. Failed attempts remain in `/tmp` with their diagnosed
fixes; no result was silently converted into PASS.

## Private images and manual testing

Final **read-only FAT32 demonstration**, 97 MiB, FAT16 system plus FAT32 fixture:

```
/tmp/qdos386-r7-1wn7185l/products/qdos386-r7-readonly.img
SHA256 49b211df1686e7f4f07a8b24f331e5c9e80b95997ead97cfab39b117fc4b888c
```

```sh
cd /tmp/qdos386-r7-1wn7185l
TMPDIR="$PWD/tmp" qemu-system-i386 -machine pc -accel tcg \
  -cpu pentium -m 32 -smp 4 -vga std -snapshot \
  -drive file=products/qdos386-r7-readonly.img,format=raw,if=ide,index=0 \
  -boot order=c -no-reboot
```

Inside the guest, `RUN /DTABI.AOU` runs the FAT32 read/seek/cleanup probe;
`RUN /FDISK.AOU --image ide0` inspects the disk; and
`RUN /CHKDSK.AOU --image ide0 --partition 2` scans FAT32 without writing.
To test formatting, attach a new disposable secondary image and use `ide1`;
never use the preserved fallback in place.

Exact recreated FAT16 fallback, 33 MiB:

```
/tmp/qdos386-r7-1wn7185l/products/fresh-fat16-fallback.img
SHA256 3b3ceb41210c9726af2cd65c55225a5571f9df7193c96d722ce017506560396a
```

The original accepted image and `checkpoint/fat16-fallback.img` remain intact.
These are private development images with historical payloads; no distribution
or rights review is implied.

## Precise B2/B3 boundary

The host-only negative experiment in `B2-WRITER-BLOCKER.md` shows a successful
write/flush corrupting a high-cluster FAT32 file: cluster 70010 becomes 4474 in
the directory entry, and its FAT end marker is the 16-bit value 65535. Independent
fsck confirms the discrepancy. This is a multi-path writer port, not a flag
change. The unsupported flag now causes a compiler error. The corrupt disposable
image, actual experiment source/binary and decoded fields are retained.

Further work must port FAT entry writes, high directory words, active/mirrored
updates, root growth, advisory accounting and recovery together, then qualify
real native mutations and injected failures. WIN1 compatibility/system startup,
FAT32 application execution, 1/4/8-CPU write matrices, power-loss behavior and
the FAT16-recovery/FAT32-system alpha remain **untested and unaccepted**. Do not
remove the safety guard, weaken the independent judges or label this image a
writable FAT32 alpha. Resume from B2 with the preserved counterexample and A/B1
checkpoints; do not restart completed gates.
