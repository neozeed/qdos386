# R12A memory map (QDOS_DYNPROC kernels)

Applies to `build/r12a/pc-vga/r12a-vga.img` (QEMU) and `build/r12a/pc/r12a.img` (headless/Tiny386). R12 and all default
images keep their own maps (R12A-MIGRATION-DESIGN.md §1).

## 1. Physical memory

| Range | Use | Mapped in a process directory |
|---|---|---|
| 0x000000 .. 0x0FFFFF | real-mode area, BIOS data, VGA text 0xB8000 | identity (template) |
| 0x100000 .. bss_end | kernel text, data, BSS (incl. template directory + 4 tables, TSSs, #DF stack) | identity |
| bss_end .. 0x1FFFFF | kernel heap: kernel stacks (16 KiB per process), channels, kernel objects | identity |
| 0x200000 .. 0x33FFFF | **frame pool** (former shell slot and slot 1) | only as the process's own pages, else not present |
| 0x340000 .. 0x37FFFF | transfer area (boot loader) | identity |
| 0x380000 .. 0x6FFFFF | **frame pool** (former profiles 2/3, R12 paged pool) | own pages or not present |
| 0x700000 .. 0x7FFFFF | boot stack (SYSTEM, top 0x7FFFF0) and BIOS data | identity |
| 0x800000 .. RAM top | **frame pool** | not present (own pages only) |

RAM top = 1 MiB + CMOS 0x30/0x31 (KiB), capped at 16 MiB (the identity map). Each pool frame is presence-tested at boot
(three words written and read back); a failing frame is removed and counted. Boot line, measured:

| Machine | RAM | Pool |
|---|---|---|
| QEMU harness (`-m 16`) | 16,384 KiB | 3,264 frames (12.75 MiB) |
| Tiny386 smoke (`mem_size = 8M`) | 8,192 KiB | 1,216 frames (4.75 MiB) |

The pool is a bitmap allocator (`kernel/frames.c`): frames are handed out one by one, no contiguity, with counters that
always satisfy used + free = pool.

## 2. Virtual memory of a process

| Virtual range | Contents | Frames |
|---|---|---|
| 0x00000000 .. 0x001FFFFF | kernel, heap, VGA (template identity, supervisor) | shared |
| origin .. image end | text + data + BSS, page by page; origin = the a.out link origin: **0x240000** (canonical), or 0x200000 / 0x380000 / 0x500000 for historical images | private, zero-filled, then loaded |
| rest of the historical window | not present (a stray access faults) | — |
| window top − 4 KiB | argument page; the R8 block is in its last 512 bytes (0x33FE00 for 0x240000 images) | private |
| 0x00340000 .. 0x0037FFFF | transfer area | shared |
| 0x00700000 .. 0x007FFFFF | boot stack | shared |
| other pool ranges | not present | — |
| 0x10000000 .. 0x10000000 + 8 MiB | private heap: grows page by page on QT_MEM_ALLOC (16 KiB steps), never shrinks | private |
| 0x1FFF0000 .. 0x20000000 | user stack, 64 KiB, committed at creation | private |
| below 0x1FFF0000 (to the heap cap) | unmapped guard | — |

Window sizes (from the historical profiles): 0x200000 → 256 KiB, the others → 1 MiB; image limits as in R8/R9
(0x38000 for 0x200000 images, 0xFFE00 otherwise).

Per process: 1 directory + private copies of the two low template tables (they contain pool frames) + one table per 4 MiB
of heap in use + one for the stack. A process with a small image uses about 30 frames (MAPTEST: 28, of which 16 are the
stack).

## 3. Kernel memory per process

16 KiB kernel stack + channel objects + a `struct proc` slot (static table, `JOB_MAX` = 16). The kernel heap is no longer
used by applications: `malloc`/`qd_alloc` from a process allocate in its private heap.

## 4. Limits

| Limit | Value | Where |
|---|---|---|
| jobs (SYSTEM, IDLE, shell + 13) | 16 | `JOB_MAX` |
| unreaped children per parent | 8 | `PROC_CHILD_LIMIT` |
| child records system-wide | 32 | `CREC_MAX` |
| private heap per process (virtual) | 8 MiB | `QDOS_PROC_HEAP_MAX` |
| user stack | 64 KiB | `QDOS_PROC_STACK` |
| kernel stack | 16 KiB | `PROC_KSTACK_BYTES` |
| private page tables per process | 16 | `VAS_MAX_TABLES` |
| physical memory used | ≤ 16 MiB | identity map |
