# Apollo R7 native disk tools

FDISK, FORMAT and CHKDSK are GCC 1.40 native QDOS/386 programs. An explicit
`make apollo-r7` kernel provides their maintenance trap. Older kernels reject
that trap; their production profiles remain available. Build libraries in
`source/sdk/{lib,libc,libm,compiler}`, then `make -C source/apps/disktools`, with
`QDOS386_TOOLS` set. Build and run tests only in an isolated copy under `/tmp`.
`make host` builds a host test adapter from the same C89 metadata code.

## Targets and safety

`--image ide0` and `--image ide1` identify primary IDE master and slave in the
guest. The kernel determines which is the boot disk using the validated R6
contract, rather than assuming BIOS 0x80 means master. No writes or maintenance
claims on the **entire live boot/system disk** are allowed. A secondary disk
must be unmounted. There is currently no mount/unmount command and no secondary
volume mount driver. GPT, extended/logical partitions, FAT12, non-512-byte
sectors and LBA48 are unsupported. Capacity is limited to LBA28.

Device identity combines ATA serial/model/capacity and physical unit. It is an
identity check for a static IDE topology, not cryptographic authentication or
hot-plug support. The kernel verifies identity and bounds on every transfer,
serializes controller selection with the existing storage worker, and restores
the immutable boot selection before returning. Secondary transfers bypass the
boot disk cache; no secondary mount/cache can race with maintenance. A tagged
job owns a maintenance lease; other clients are refused until release or job
reaping. Unexpected I/O failures may leave partial updates; no transaction,
journaling, secure erase or power-loss recovery guarantee is made.

All mutation requires a final typed `WRITE IDE1-<identity>-<sectors>` confirmation.
Tests may explicitly pass `--test-disposable`, together with `--image`, on a
new ordinary backing file in `/tmp`. This bypasses the final identity typing,
**not** CHKDSK's repair decision or the kernel's live-disk and identity checks.
Do not use this test option for real media. The host adapter requires canonical
regular non-hardlinked `/tmp` paths ending `.r7-disposable.img` for writes;
symlinks, devices, missing files and paths outside `/tmp` are refused.

## FDISK

```
RUN /FDISK.AOU --image ide1
RUN /FDISK.AOU --image ide1 --edit
```

Inspection is the default. Edit commands stage changes only:

```
create 1 2048 65536 16
create 2 67584 131072 32
active 1
show
write
```

`delete N` removes a staged primary entry. `quit` or EOF discards changes.
`write` displays the proposal and requires confirmation. It rejects overlaps,
wrapped/out-of-disk extents, unsupported types and conflicting active flags.
The original MBR boot code and disk signature are preserved; no boot loader is
installed. Nonzero invalid tables are refused, not treated as blank disks.
Free-space totals include reserved/alignment gaps and are not permission to
use every apparently free sector. Starts are in 512-byte sectors; use 2048-sector
alignment unless a deliberate compatible layout requires otherwise.

## FORMAT

```
RUN /FORMAT.AOU --image ide1 --partition 1 --fat 16
RUN /FORMAT.AOU --image ide1 --partition 2 --fat 32
```

The MBR type must agree with FAT16/FAT32. Geometry is computed and classified
before metadata writes. Two FAT copies, reserved metadata, root storage,
FAT32 FSInfo and backup structures are initialized. New FAT32 root storage is
zeroed. Other free data clusters are not securely erased. A future writable
filesystem must zero newly allocated clusters before exposing them.

An existing VBR must describe the same FAT type and supported reserved-sector
layout. Its jump/OEM and executable region are preserved. Unknown VBRs,
reserved loader layouts, type conversion and unexpected FAT32 reserved-sector
payloads are refused. Apollo's live boot disk is always refused; its MBR gap,
VBR and stage 2 cannot be formatted through this interface. Newly formatted
data partitions have no boot loader.

## CHKDSK

```
RUN /CHKDSK.AOU --image ide1 --partition 2
RUN /CHKDSK.AOU --image ide1 --partition 2 --repair
```

The default scan is read-only, including FSInfo. It reports chain bounds,
cycles, cross-links, lost clusters, size mismatches, directory/dot entry errors,
long-name sequence/checksum problems, mirrored FAT differences, reserved FAT
entries, backup BPB differences and FSInfo inconsistencies. FAT32 active-FAT
selection is honored. Limits: 262144 data clusters, directory depth 32,
one 32-bit ownership word per cluster. Exhausted limits or allocation/I/O
failure produce a fatal/unsupported result, never a clean result. Issue output
is capped per class while totals continue counting. This is a bounded metadata
checker, not a general data-recovery or Unicode filename-repair tool.

Only a validated advisory FSInfo correction is repairable in this checkpoint.
Any structural/ambiguous error refuses automatic repair. The prompt is:

```
Fix (Y/N/A/Q)? [N]
```

Y accepts this FSInfo repair; N skips; A accepts safe FSInfo-class corrections
only; Q stops further repairs. Enter and EOF mean No. On the native raw console, Ctrl+D or Ctrl+Z ends
input without authorizing a repair. Both advisory copies are
one repair plan. A separate disk identity confirmation still precedes writes
unless explicit disposable-test authorization is provided. The scanned free
count is recorded and the next-free hint becomes unknown. No file chains,
FAT copies, orphan clusters or cross-linked data are guessed or discarded.

Exit status: 0 clean/success; 1 errors unfixed or cancelled; 2 repaired;
3 fatal I/O, unsupported geometry or invalid invocation. A successful format
must still be independently checked before trusting the volume.

## Alpha test checklist

Use a copy of the accepted FAT16 fallback. Attach a **new disposable** secondary
raw disk, inspect both disks, verify live-disk refusal, create supported primary
partitions, format each, and scan. Inject corruption offline only with QEMU
stopped. Exercise N/Q/Enter/EOF without changes, then Y/A for safe FSInfo repair.
After shutdown, extract each partition and run Linux `fsck.fat -n`; compare
payload hashes and boot-sector bytes. Run the native storage-worker test and
GCC/HI on the FAT16 boot volume. Never claim host tests as guest acceptance.

The accepted FAT16 fallback still boots with exactly one drive:

```
qemu-system-i386 -machine pc -accel tcg -cpu pentium -m 32 -smp 4 \
  -vga std -snapshot -drive file=FAT16-FALLBACK.img,format=raw,if=ide,index=0 \
  -boot order=c -no-reboot
```

For maintenance tests, add a second `-drive` pointing to a newly created
ordinary disposable file. A FAT16+FAT32 one-drive system is a separate gate;
these utilities alone do not make FAT32 a mounted kernel filesystem.
