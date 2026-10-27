# R11J — Memory maps: physical, kernel template, per-process directories

R9 layout throughout.  Default (non-POC) images keep the R11I maps unchanged
(`docs/R11I-MEMORY-MAP.md`); everything below applies only to `-DQDOS_PAGING_POC` images.
Values marked *observed* are from the runtime transcripts in `logs/r11j/`.

## 1. Physical maps

| Physical range | i386VM block (16 MiB harness) | Tiny386 PC (8 MiB) / QEMU (`-m 16`) |
|---|---|---|
| 0x000000–0x0FFFFF | kernel text/data/BSS from 0 (incl. page-table pool), heap | real-mode area, BIOS, VGA text 0xB8000 (I/O) |
| 0x100000–0x1FFFFF | heap | kernel from 0x100000 (incl. page-table pool), heap |
| 0x200000–0x23FFFF | slot 0: shell | slot 0: shell |
| 0x240000–0x33FFFF | slot 1: ordinary RUN programs (e.g. SAMELNCH) | same |
| 0x340000–0x37FFFF | transfer area | transfer area |
| 0x380000–0x47FFFF | profile 2 | profile 2 — **POC frame pool** |
| 0x480000–0x4FFFFF | unassigned | unassigned (inside the pool range, never allocated) |
| 0x500000–0x5FFFFF | profile 3 | profile 3 — **POC frame pool** |
| 0x600000–0x7FFFFF | boot stack (top 0x7FFFF0), firmware data | same |
| 0x800000–0x800FFF | device page (console, input, timer, block device) | (PC: none; QEMU: RAM) |
| 0x801000–0x8FFFFF | RAM, unused | (PC: absent) |
| 0x900000–0xFFFFFF | R11I spare span — **POC frame pool** | (PC: absent) |
| ≥ 0x1000000 | does not exist | does not exist |

Frame pool per build: `PG_POOL_LO..PG_POOL_HI` in `kernel/paging.h`.  i386VM block: 0x900000–0x1000000
(7 MiB = 7 windows).  PC/QEMU: 0x380000–0x600000 (two 1 MiB windows: profiles 2 and 3).  The TEST-only
cap image limits the i386VM pool to 0x900000–0xB00000.

## 2. Page-table pool (kernel BSS)

`pg_mem` in `paging.c`: 21 pages, aligned at run time to the first page boundary inside the array.

| Page | Use | i386VM | PC |
|---|---|---|---|
| 0 | template directory | 0x1B000 (observed) | 0x11B000 (observed) |
| 1–4 | template tables, identity 0–16 MiB | 0x1C000–0x1F000 (derived) | 0x11C000–0x11F000 (derived) |
| 5–9 | record 0: directory + up to 4 private tables | dir 0x20000 (observed) | dir 0x120000 (observed) |
| 10–14 | record 1 | dir 0x25000 (observed) | dir 0x125000 (observed) |
| 15–19 | record 2 | dir 0x2A000 (derived) | dir 0x12A000 (derived) |

*Observed* values are printed by QT_PG_INFO in the transcripts; *derived* ones follow from the layout
(consecutive pages after the aligned base).

## 3. Kernel template (CR3 of SYSTEM, IDLE, the shell and every unpaged job)

* Directory entries 0–3 present, each pointing at an identity table: virtual = physical for 0–16 MiB.
* PTEs: present, read/write, **supervisor** (U/S = 0).  PDEs carry U/S = 1 so a user PTE inside is
  usable; at CPL 0 neither bit is checked.
* Entries 4–1023 not present: any access at or above 0x1000000 faults.
* The frame pool is mapped (identity) here, so the paged loader, running in an unpaged caller's
  trap, writes a new process's frames through their physical addresses.

## 4. A paged process's directory

Built by `pg_rec_build` from the template.  Entries whose 4 MiB range holds no window page and no
pool page are **the template's own tables** (shared, identical mappings).

### i386VM block

| Entry | Range | Table | Mapping |
|---|---|---|---|
| 0 | 0x000000–0x3FFFFF | private copy | 0–0x23FFFF identity (kernel, heap, shell); **0x240000–0x33FFFF → frames** (user PTEs); 0x340000–0x3FFFFF identity |
| 1 | 0x400000–0x7FFFFF | template | identity (profiles 2/3, boot stack) |
| 2 | 0x800000–0xBFFFFF | private copy | 0x800000–0x8FFFFF identity (device page); 0x900000–0xBFFFFF **not present** |
| 3 | 0xC00000–0xFFFFFF | none | **not present** (entirely pool) |

### Tiny386 PC / QEMU

| Entry | Range | Table | Mapping |
|---|---|---|---|
| 0 | 0x000000–0x3FFFFF | private copy | identity except **0x240000–0x33FFFF → frames** and 0x380000–0x3FFFFF not present |
| 1 | 0x400000–0x7FFFFF | private copy | 0x400000–0x5FFFFF not present; 0x600000–0x7FFFFF identity (boot stack) |
| 2–3 | 0x800000–0xFFFFFF | template | identity (PC: no RAM there) |

Consequences, all live-proven:

* Two processes with the same virtual window map different frames (observed i386VM: copy 1
  PTE 0x00900027, copy 2 PTE 0x00a00027; PC: 0x00380027 / 0x00500027).
* A process cannot reach any frame of the pool, its own included, except through its window:
  reading another copy's frame address faults (observed CR2 = 0x900000 on i386VM, 0x380000 on PC/QEMU).
* The kernel, every job stack, heap block, the IDT/GDT, the page tables and the device page have the
  same virtual address in every directory.

## 5. Allocation and release

| Object | Allocated by | Owner | Released by |
|---|---|---|---|
| Frame run (1 MiB, contiguous) | `app_frames_alloc`: `region_reserve_at` scan of the pool, page step | region owner 0x7F00 + record | `pg_release_owner` in `reclaim_dead`, or the loader's rollback |
| Directory + private tables | record `pages[]` (static) | record ↔ tagged job id ↔ slot | same |
| Job stack | kernel heap (`mem_alloc`) | tagged job id | `mem_reclaim_owner` (unchanged) |

Release never runs in the dead job's own context: `reclaim_dead` skips `job_current`, and the
reclaiming job's directory is its own.  `pg_release_owner` also refuses (and reports) a directory
that is active, as a guard; the slot then stays dead and is retried.  It never fired in any run.

Frame-run contiguity is required on i386VM: its CPU core sends a multi-byte access that crosses a
page boundary above 0x800000 to the harness with the first physical address only.
