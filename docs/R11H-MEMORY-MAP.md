# R11H memory map

Base: R11G `ffe2349`. R11H commits `db42de5` (audit), `4dfeecc` (allocator), `a3c432a` (Phase 2 loader,
concurrency test), and the final commit recorded in `qdos386-R11H.tar.gz`. Addresses are physical.

This document is the verified map for the R11H release. Every address below is either a `kernel/appmap.h`
constant (the header is read by the host preprocessor in `scripts/r11h-memmap-check.py`) or a value from a
kernel's own a.out symbol table (`_bss_end`, linked by `scripts/r11h-maps-link.sh`).

## 1. Layouts

`kernel/appmap.h` has two maps, selected by `QDOS_LAYOUT_R9`:

| Map | Slot 0 (shell) | Slot 1 (child) | Transfer | RAM disk | RAM top (constant) |
|---|---|---|---|---|---|
| default (R5–R8) | 256 KiB at 0x200000 | 256 KiB at 0x240000 | 0x280000, 256 KiB | 0x2C0000, 3 MiB | 0x800000 |
| `QDOS_LAYOUT_R9` (R9–R11H) | 256 KiB at 0x200000 | 1 MiB at 0x240000 | 0x340000, 256 KiB | 0x900000, 3 MiB (i386VM) | 0x1000000 (i386VM) |

The R9–R11H release uses the R9 map. The "256 KiB" in older text describes only the default map.
`RAM_TOP` is not read by any kernel or platform code; it is a documentation constant.

## 2. Verified reservations

Kernel and heap bounds come from the link maps (a.out `_bss_end`, aligned up to 16 bytes as in
`kernel/main.c`). The Tiny386 kernel is linked at 0x100000; the i386VM kernel at 0.

| Target | Kernel image + BSS | Heap arena | Heap end |
|---|---|---|---|
| Tiny386 PC, R9 map | 0x100000 .. 0x11A640 | 0x11A640 .. 0x200000 | 0x200000 |
| i386VM, R9 map | 0 .. 0x1A170 | 0x1A170 .. 0x200000 | 0x200000 |
| i386VM, default map (R8 baseline) | 0 .. 0x15DC0 | 0x15DC0 .. 0x200000 | 0x200000 |

The default-map value 0x15DC0 (89,536) matches the `Heap: arena 89536` boot line recorded in an earlier
R8 run, so the link-map method agrees with a runtime value.

Full reservation table, R9 map (checked by `scripts/r11h-memmap-check.py`, result in section 6):

```
0x000000 .. kernel + BSS          i386VM (0x100000 on the PC)
heap   .. 0x200000                 job stacks, channels, user blocks
0x200000 .. 0x23FFFF  slot 0       shell, 256 KiB
0x240000 .. 0x33FFFF  slot 1       child, 1 MiB
0x340000 .. 0x37FFFF  transfer     read-only to applications
0x380000 .. 0x47FFFF  profile 2    1 MiB (R9 only)
0x480000 .. 0x4FFFFF  unassigned   512 KiB: not reserved, not a profile
0x500000 .. 0x5FFFFF  profile 3    1 MiB (R9 only)
0x600000 .. 0x7FFFFF  boot stack (top 0x7FFFF0) and firmware data
0x800000 .. 0x800FFF  device page  i386VM only (ports, not RAM)
0x900000 .. 0xBFFFFF  FAT16 RAM disk  i386VM only, 3 MiB (transport, see section 4)
0xC00000 .. 0xFFFFFF  unassigned   i386VM only, 4 MiB: not reserved, not used
```

The boot-stack top 0x7FFFF0 is verified for the Tiny386 (`platform/pc/entry.s` sets `%esp`). The i386VM
initial stack pointer is not set anywhere in this tree; the i386VM harness provides it. The R5 ABI
reserves 0x600000–0x7FFFFF for the boot stack, and the R11H check uses that range for i386VM, but the
exact i386VM top is not verified in R11H.

The Tiny386 has no device page and no RAM disk. Its FAT16 volume is the second IDE drive (`hdb`), and
`run.ini` sets `mem_size = 8M`, so its RAM ends at 0x800000. That RAM size is an assumption: the PC
kernel does not probe memory. All R9 profile and reservation addresses on the PC are below 0x600000,
and every test on Tiny386 used them successfully.

## 3. Application profiles and the loader's pool

R11H has four fixed-origin profiles. An image is linked for one profile base (its entry address) and
runs only there. The loader never moves an image.

| Profile | Base | Size (R9) | Size (default) | Owner |
|---|---|---|---|---|
| 0 | 0x200000 | 256 KiB | 256 KiB | shell (boot) |
| 1 | 0x240000 | 1 MiB | 256 KiB | child launched by RUN |
| 2 | 0x380000 | 1 MiB | 0 (absent) | `qd_start(name, 2)` |
| 3 | 0x500000 | 1 MiB | 0 (absent) | `qd_start(name, 3)` |

On the default map, profiles 2 and 3 have size 0 and any load for them is refused with
`APP_ERR_NOPROFILE` (-35), because that range holds the RAM disk there.

The loader's region pool (`kernel/aout.c`, `app_pool_init`) has FREE spans only where profiles exist:
0x200000–0x340000 (slots 0 and 1), 0x380000–0x480000 (profile 2) and 0x500000–0x600000 (profile 3). The
kernel, heap, transfer area, stacks, device page and RAM disk are not in the pool and cannot be reserved
through it. `scripts/r11h-memmap-check.py` checks that each pool span lies inside the profiles and
overlaps no other reservation.

The unassigned 512 KiB at 0x480000–0x4FFFFF is not a profile and nothing loads there. Earlier text in
this project called 0x380000–0x5FFFFF "free": that was true of the range but profiles cover only
0x380000–0x47FFFF and 0x500000–0x5FFFFF.

## 4. FAT16 transport

- **Tiny386 PC:** the volume is read from the second IDE disk. It is not in guest RAM and does not
  appear in the memory map.
- **i386VM:** the volume is preloaded into guest RAM at 0x900000 (`platform/i386vm/disk.c` reads
  `RAMDISK_BASE + lba*512`). The map records it as RESERVED with the kind `TRANSPORT`.

The allocator does not depend on the transport. The pool is built from the profile constants and
nothing else, so a future change that moves the volume out of guest RAM (a block device) would change the
i386VM platform and the disk driver, not the region allocator or the loader. Per the R11H plan, that
block-device change is not part of R11H. Nothing in R11H treats the RAM-backed volume as permanent: it is
reserved today only because it occupies that address range.

## 5. Stacks, heap and per-job memory

- Kernel job stacks: heap, `JOB_STACK_BYTES` (4096), 16-byte aligned, canary `JOB_STACK_CANARY`.
- Application jobs: `APP_STACK_BYTES` (16384) from the heap through `job_alloc_sized`.
- An application's image starts at its profile base. Its argument block sits at the top of its profile
  (`base + size - APP_ARG_BYTES`).
- Heap blocks owned by a job, and its channels, are reclaimed at exit.
- `JOB_MAX` is 6: SYSTEM, IDLE, shell and room for two applications. With four profiles this is the
  binding limit for concurrency: the job table is full exactly when all four profiles are live.

## 6. Verification

`scripts/r11h-memmap-check.py build/pc-r11h/kernel.aout build/r11h-maps/i386-r9.aout
build/r11h-maps/i386-r8.aout` reads the layout constants from `kernel/appmap.h` for each layout, takes the
heap bounds from the three kernel link maps, and checks:

- every reservation is inside its target's RAM;
- no two reservations overlap, on each of the three targets;
- the stack top (0x7FFFF0) is inside the boot-stack reservation;
- the loader's pool spans lie inside profiles and overlap no reservation;
- profile 3 does not reach the boot stack;
- on the default map, profiles 2 and 3 are absent and the RAM disk is at 0x2C0000.

Its output is recorded in `docs/R11H-RUNTIME-REPORT.md`. Compile-time checks in `appmap.h` cover the same
invariants for the R9 map.

## 7. Open items after R11H

- The full platform map is not fed to the allocator. The pool covers profile ranges only, so the heap, transfer
  area and RAM disk are excluded by construction rather than by a platform-supplied span list.
- Memory size is not discovered at run time (no E820 or BIOS probe). The Tiny386 RAM top is the
  `run.ini` value.
- The i386VM initial stack top is not verified from source (section 2).
- The 512 KiB gap at 0x480000 is unused. Making it a profile would need a new linker script and an SDK
  profile; that is not part of R11H.
