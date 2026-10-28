# R11K memory map

All addresses physical unless noted. "R9 map" is the layout every R11H/R11I/R11J/R11K image uses
(`-DQDOS_LAYOUT_R9`). The paged window is a *virtual* range that coincides with slot 1.

## Boot

| address | what |
|---|---|
| 0x0000–0x04FF | real-mode IVT, BIOS data area (conventional-memory size at 0x413) |
| 0x7C00–0x7DFF | boot sector (DAP 0x7DB2, GDT 0x7DE0) |
| 0x10000–0x7FFFF | R11K loader buffer (448 KiB): kernel + transfer area as read by `INT 13h`; reused as free memory afterwards |
| 0x90000 | loader's protected-mode stack top |
| 0x100000 | kernel text+data (copied here), entry `kstart` |
| 0x340000 | transfer area copy target (R9; 0x280000 on the default map) |

The original loader read into 0x10000–0x1FFFF only (64 KiB, 120 sectors).

## Kernel and applications (R9 map)

| range | use |
|---|---|
| 0x100000 – (BSS end) | kernel text, data, BSS, page-table pool (paging builds: +21 static pages for directories and private tables) |
| (BSS end) – 0x1FFFFF | kernel heap arena: job stacks, `qd_alloc`/`malloc`, channel objects — **shared by all processes** |
| 0x200000–0x23FFFF | slot 0 (the shell) |
| 0x240000–0x33FFFF | slot 1, 1 MiB. Unpaged: physical = virtual. Paged: **virtual window**; each process maps its own frames here |
| 0x340000– | transfer area |
| 0x380000–0x47FFFF | profile 2 (1 MiB) — **PC/QEMU frame pool, part 1** |
| 0x500000–0x5FFFFF | profile 3 — **PC/QEMU frame pool, part 2** |
| 0x800000.. | i386VM device page (console, timer, block registers) |
| 0x900000–0xFFFFFF | i386VM block map only: spare span = **frame pool, 7 windows** |
| ≥ 0x1000000 | not mapped in paging builds; any access faults |

## Paged address space (paging builds)

| virtual range | in a paged process | in the template (everything else) |
|---|---|---|
| 0–16 MiB | identity map, 4 KiB supervisor pages, shared page tables | same |
| 0x240000–0x33FFFF | **private** frames from the pool | identity (the unpaged slot 1) |
| frame pool | **unmapped** (a stray access faults: CR2 0x900000 on i386VM, 0x380000 on PC) | identity |

Page directories: one template + one per paged process (3 records). Page tables covering neither the window nor
the pool are shared with the template. CR3 is loaded only when it changes, from the scheduler with interrupts off;
the stack the switch runs on is kernel heap, present in every directory.

## Why 2 windows on PC and QEMU

The default 8 MiB PC map has no spare span, so the pool is profile 2/3. The tests ran with `-m 16` (QEMU) and
`mem_size = 8M` (Tiny386): more windows would need a different memory map and were not tested.
