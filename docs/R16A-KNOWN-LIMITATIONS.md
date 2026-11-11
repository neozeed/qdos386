# R16A — known limitations and boundaries

R16A adds exactly two user-visible things on top of R16: an MBR-partitioned FAT16 data disk on `hdb`, and a CPU
column in the Task Manager. Both are opt-in (compiled only under `-DQDOS_R16`); the default KCTX-FIX2 kernels and the
frozen R12–R16 targets are byte-identical. The scope was kept deliberately small.

## Disk

* **One primary FAT16 partition only.** Supported partition types are `0x04`, `0x06`, `0x0E`; the BPB is validated
  rather than trusting the type. No extended/logical partitions, no GPT, no FAT32/NTFS/exFAT. If several FAT16
  primaries exist, the lowest-LBA one is chosen and a line is printed; it is never a silent arbitrary choice.
* **512-byte sectors only.** The validator requires `bytes_per_sector == 512`.
* **The data disk only.** The feature affects the WIN1_ volume (the PIIX3 primary slave, QEMU `hdb`). The boot/kernel
  image stays a separate, unpartitioned IDE disk; there is no partitioned bootloader and boot-time layout is unchanged.
* **No VHD container parsing.** QEMU exposes a VHD/VPC as ordinary IDE sectors (`format=vpc`), so the kernel reads a
  plain MBR. A raw image works with `format=raw`. The kernel does not read `.vhd` footers itself.
* **No repair or format.** A disk that is neither a FAT16 VBR nor a valid MBR with a usable FAT16 primary is refused
  (`FAT_ERR_BPB`/`FAT_ERR_TYPE`); it is never treated as empty, formatted or repaired. Because WIN1_ is also where the
  shell loads from, a refused disk means the system cannot boot (a clear "WIN1_ volume not mounted" boot failure),
  which is why the reject matrix is covered by a host unit test rather than by bricking a QEMU boot.
* **Read-only media** behave as before: writes fail cleanly, reads work.
* **Windows preparation caveat.** Creating a VHD in Windows does not by itself produce a FAT16 partition — Windows may
  format FAT32 or NTFS. The volume must be deliberately formatted FAT16. Never let Windows and QEMU mount the same
  writable image at the same time.

## Task Manager CPU column

* **Two CPUs.** The column shows `0`, `1`, `~0`, `~1`, or `-`. It reflects the R16 static-affinity model: there is no
  migration, so a job's CPU is fixed once pinned.
* **`~N` means "last ran on CPU N", not "running now".** The Task Manager snapshot executes inside SYSTASK on CPU 0
  with interrupts off, so at the instant of the snapshot whichever CPL3 job was running on CPU 0 is momentarily
  displaced and reads as not-running (`~0`); a job running on CPU 1 is captured live (`1`). This is honest, not a bug:
  a `~N` job is runnable/blocked and will next run on CPU N. `last_cpu` is a read-only annotation set beside
  `running_on` in the scheduler; it changes no scheduling decision.
* **No usage %, no TOP, no migration, no priority/affinity editing** from the Task Manager — display only, plus the
  existing switch/end/start controls. The job table and affinity are never written by the Task Manager.
* **Width.** To fit 80×25 the TITLE column is 24 wide under R16A (was 28); no control or other column was removed.

## Bug found and fixed (R16 AP idle affinity)

Adding the CPU column revealed that `kernel/smp.c` set `jobs[ap_idle].aff = 1` using the job **id** returned by
`job_alloc` (`tag<<16 | slot`) as the array index — an out-of-bounds write that also left the real CPU-1 idle slot at
`aff = 0`, so CPU 0 could schedule the CPU-1 idle job when it was momentarily free. Fixed by masking to the slot
(`ap_idle &= 0xffff`) before indexing. This is a one-line correctness fix; the two-CPU gates (`R16T RACE`, Gate 1)
still pass and the default kernels remain byte-identical.

## Not attempted

Multiple/partitioned data disks beyond one FAT16 primary, a partitioned boot disk, FAT32/long-term large volumes,
per-CPU usage accounting, task migration, or any change to the frozen releases or the ordinary KCTX-FIX2 kernel.
