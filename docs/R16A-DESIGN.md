# R16A — MBR-partitioned FAT16 data disk + CPU-aware Task Manager (design)

Two small, localized changes on top of the frozen R16 BKL/SMP source. Both are compiled only under `-DQDOS_R16`
(the `qdos386-smp-pc` / R16A build); the ordinary KCTX-FIX2 kernels and every frozen target are byte-identical.

## Baseline

Branch `r16a-mbr-taskcpu` from R16 `HEAD` (tag `r16-bkl-smp-experiment`). Default kernels unchanged:
`qdos386-vga.img` = `f8e5d5e1…`, `qdos386.img` = `7c675978…`.

## Subsystem 1 — MBR-partitioned FAT16 data disk

**Where.** The WIN1_ data volume is the PIIX3 **primary slave** (QEMU `hdb`, `platform/pc/disk.c`); the kernel/boot
image is the separate master (`hda`). So the "data disk" is exactly WIN1_. The only code that learns about partitions
is the block-volume adapter in `kernel/win1.c` (`disk_thunk` / `disk_thunk_w`); `kernel/fat16.c` is untouched and keeps
seeing partition-relative LBAs. `platform/pc/disk.c` is untouched and keeps operating on physical LBAs.

**Drive layout accepted.** Either (a) an unpartitioned FAT16 superfloppy — sector 0 is the FAT16 VBR, exactly as every
prior image; or (b) an MBR at LBA 0 (`0x55AA` at offset 510) with one primary FAT16 partition beginning at a nonzero
LBA (e.g. 2048), partition type `0x04`, `0x06` or `0x0E`, holding a normal FAT16 VBR/BPB.

**Mount decision (`choose_partition`).**
1. Read physical sector 0. If it `looks_like_vbr` (0x55AA, bytes/sector = 512, sectors/cluster a power of two 1..128,
   reserved ≥ 1, FATs = 2) → **superfloppy**: `part_lba = 0`, `part_count = device_sectors`. Behaviour is identical to
   before, so all existing images keep working.
2. Else if sector 0 has `0x55AA` → parse the four primary MBR entries. A candidate is type 04/06/0E with nonzero
   start and size, `start < device`, `size ≤ device`, no 32-bit overflow of `start+size`, and `start+size ≤ device`.
   Choose the candidate with the lowest start LBA (deterministic); if more than one FAT16 primary exists, say so and
   use the lowest — never silently an arbitrary one. `part_lba = start`, `part_count = size`.
3. Else (neither a VBR nor a valid MBR) → refuse (`FAT_ERR_BPB`). A corrupt/invalid disk is never treated as empty and
   is never formatted or repaired.

Then `fat16_mount(&vol, disk_thunk, 0, part_count)` validates the **partition's** VBR/BPB through the offset — reusing
the FAT driver's own BPB/geometry/cluster-count/FAT-length checks, including `total ≤ part_count`. An MBR whose chosen
partition has an invalid VBR fails the mount with a clear code rather than mounting anything.

**Sector mapping.** `physical_lba = part_lba + fat_relative_lba`. Every `disk_thunk`/`disk_thunk_w` call checks
`relative < part_count`, guards `part_lba + lba` against overflow, and checks `physical < device_sectors` before
calling `disk_read_sector`/`disk_write_sector` (which also bound against the IDE sector count). Reads and writes,
including FAT-copy flushes and `SYNC`, all go through the same offset, so they target partition-relative sectors.
Sectors outside the partition (the MBR, any gap, other partitions) are never addressed.

**Out of scope:** extended/logical partitions, GPT, FAT32, NTFS, exFAT, a partitioned bootloader, or any change to
boot-time layout. QEMU exposes VHD/VPC as ordinary IDE sectors, so the kernel need not parse VHD containers.

## Subsystem 2 — Task Manager CPU column

**Data.** The R16 scheduler already records per-job ownership: `running_on` (−1 not running, else the CPU) and `aff`.
R16A adds one read-only annotation, `last_cpu`, set beside `running_on` in `schedule()` (it changes no scheduling
decision) and initialized to −1; it records the CPU a job last ran on. SYSTASK's `snapshot()` already runs with
interrupts off and reads `jobs[]` directly, so it copies `running_on` and `last_cpu` into the row with no new API.

**Display semantics (`cpu_cell`).** A **running** job shows its confirmed CPU, `0` or `1`. A job **not currently
running** shows the CPU it last ran on as `~0` / `~1` (the `~` means "last ran on", never "running now"). A job
**never dispatched** shows `-`. An inactive job is therefore never labelled as actively on a CPU. SYSTEM shows `0`
(it runs on CPU 0 from boot); the CPU-1 idle job is affine to and runs on CPU 1.

**Layout (80×25).** Under `-DQDOS_R16` the `TITLE` column narrows from 28 to 24 and a 3-wide `CPU` column fills the
freed gap at column 35; `PROGRAM`, `STATE`, `CON`, `PARENT` and the flags column keep their exact positions and widths.
No control is removed or truncated. Task Manager still switches and ends tasks and never writes the job table or
affinity.

## Gates

- **Gate 0** (this doc): call sites identified, no speculative filesystem features. ✓
- **Gate 1**: superfloppy still mounts R/W; an MBR FAT16 partition at LBA 2048 mounts R/W (incl. a VFAT long name);
  corrupt signature / bad length / out-of-range LBA / wrong type / invalid BPB all fail safely without modifying the
  image; sectors outside the partition are byte-identical across writes.
- **Gate 2**: the Task Manager shows grounded CPU ownership (verified against `R16T RACE`) and still switches/ends tasks.
