#ifndef QDOS386_APPMAP_H
#define QDOS386_APPMAP_H

/*
 * QDOS/386 fixed memory map shared by kernel, loader and applications.
 *
 * Two maps are selected at build time:
 *
 *   default (R5-R8)  both application slots 256 KiB; RAM disk at 0x2C0000
 *                    (i386VM, 8 MiB guest).  Reproduces the R8 baseline.
 *   QDOS_LAYOUT_R9   slot 0 (shell) 256 KiB, slot 1 (child) 1 MiB; transfer
 *                    area moved to 0x340000; i386VM guest RAM is 16 MiB with
 *                    the device page at 0x800000 and the RAM disk at 0x900000.
 *
 * Both maps keep slot 1 at 0x240000, so an a.out linked for slot 1 keeps its
 * entry address across builds.  The kernel is linked at 0 on the i386VM
 * harness and at 0x100000 on the Tiny386 PC; its BSS ends below HEAP_END.
 *
 * R9 map (addresses are physical; "free" is not in the heap):
 *
 *   0x000000 .. kernel image and BSS               (target dependent)
 *   HEAP_START .. 0x200000   kernel heap (job stacks, channels, user blocks)
 *   0x200000 .. 0x23FFFF     application slot 0 (shell), 256 KiB
 *   0x240000 .. 0x33FFFF     application slot 1 (child), 1 MiB
 *   0x340000 .. 0x37FFFF     transfer area (read-only to applications)
 *   0x380000 .. 0x5FFFFF     free, reserved, not in the heap
 *   0x600000 .. 0x7FFFFF     boot stack (top 0x7FFFF0) and firmware data
 *   0x800000 .. 0x800FFF     i386VM device page (ports; not RAM)     [i386VM]
 *   0x900000 .. 0xBFFFFF     FAT16 RAM disk, 3 MiB                    [i386VM]
 *   0xC00000 .. 0xFFFFFF     free (i386VM RAM top is 0x1000000)      [i386VM]
 *
 * An application is linked for exactly one slot base, which is its entry
 * address.  The loader accepts an image only at a slot base.
 */
#define APP_SLOTS      2

/*
 * R11H application profiles.  A profile is a fixed link origin with a size.
 * Profiles 0 and 1 are the shell and child slots above.  Profiles 2 and 3 are
 * extra origins for concurrently resident children; they exist only on the R9
 * map, where 0x380000..0x5FFFFF is free and outside the heap.  On the default
 * map that range holds the FAT16 RAM disk, so the profiles have size 0 and
 * loads for them are refused with APP_ERR_NOPROFILE.
 *
 * An image is linked for exactly one profile base; the loader never moves it.
 */
#define APP_PROFILES   4

#ifdef QDOS_LAYOUT_R9
#define APP_PROFILE2_BASE 0x00380000u
#define APP_PROFILE2_SIZE 0x00100000u
#define APP_PROFILE3_BASE 0x00500000u
#ifdef QDOS_PG_WIN3
/* R12 (opt-in): profile 3 is 2 MiB, so the paged frame pool 0x380000..0x6FFFFF holds THREE 1 MiB windows (the default 0x380000..0x5FFFFF holds
 * two).  Still below the boot stack (top 0x7FFFF0).  Default images do not define QDOS_PG_WIN3 and are unchanged. */
#define APP_PROFILE3_SIZE 0x00200000u
#else
#define APP_PROFILE3_SIZE 0x00100000u
#endif
#else
#define APP_PROFILE2_BASE 0x00380000u
#define APP_PROFILE2_SIZE 0x00000000u   /* default map: the RAM disk is here */
#define APP_PROFILE3_BASE 0x00500000u
#define APP_PROFILE3_SIZE 0x00000000u
#endif

#define APP_PROFILE_BASE(n) ((n) == 0 ? APP_SLOT0_BASE : (n) == 1 ? APP_SLOT1_BASE : \
                             (n) == 2 ? APP_PROFILE2_BASE : APP_PROFILE3_BASE)
#define APP_PROFILE_SIZE(n) ((n) == 0 ? APP_SLOT0_SIZE : (n) == 1 ? APP_SLOT1_SIZE : \
                             (n) == 2 ? APP_PROFILE2_SIZE : APP_PROFILE3_SIZE)

#ifdef QDOS_LAYOUT_R9

#define APP_SLOT0_BASE 0x00200000u
#define APP_SLOT0_SIZE 0x00040000u
#define APP_SLOT1_BASE 0x00240000u
#define APP_SLOT1_SIZE 0x00100000u

#define XFER_BASE      0x00340000u
#define XFER_SIZE      0x00040000u

/* i386VM only: device page and RAM disk.  RAM top is the guest RAM size. */
#define DEVICE_BASE    0x00800000u
#define DEVICE_SIZE    0x00001000u
#define RAM_TOP        0x01000000u   /* i386VM guest RAM: 16 MiB */
#ifdef QDOS_I386VM_BLOCK
/* R11I: the FAT16 volume is a host file behind the device page (platform/i386vm/disk.c).
 * 0x900000..0xFFFFFF is not a RAM disk.  It is SPARE: the allocator holds it as a FREE span,
 * but no profile origin lies in it, so the loader (which reserves only profile origins) cannot
 * place an image there.  RAMDISK_* are defined as empty so no code can use them by mistake. */
#define RAMDISK_BASE   0x00000000u
#define RAMDISK_SIZE   0x00000000u
#define SPARE_BASE     0x00900000u
#define SPARE_END      RAM_TOP
#else
#define RAMDISK_BASE   0x00900000u
#define RAMDISK_SIZE   0x00300000u
#endif

#else /* default: R5-R8 map */

#define APP_SLOT0_BASE 0x00200000u
#define APP_SLOT0_SIZE 0x00040000u
#define APP_SLOT1_BASE 0x00240000u
#define APP_SLOT1_SIZE 0x00040000u

#define XFER_BASE      0x00280000u
#define XFER_SIZE      0x00040000u

/* R6: FAT16 volume on the i386VM (preloaded RAM disk, see platform/i386vm).
 * The region is 3 MiB; the volume image is exactly this size. */
#define DEVICE_BASE    0x00800000u   /* not used by the R8 map */
#define DEVICE_SIZE    0x00000000u
#define RAMDISK_BASE   0x002c0000u
#define RAMDISK_SIZE   0x00300000u
#define RAM_TOP        0x00800000u   /* i386VM guest RAM: 8 MiB */

#endif

/* Slot helpers.  APP_SLOT_SIZE is the slot 0 size; use the _OF forms for
 * slot-indexed code. */
#define APP_SLOT_SIZE       APP_SLOT0_SIZE
#define APP_SLOT_BASE_OF(n) ((n) == 0 ? APP_SLOT0_BASE : APP_SLOT1_BASE)
#define APP_SLOT_SIZE_OF(n) ((n) == 0 ? APP_SLOT0_SIZE : APP_SLOT1_SIZE)

/* Heap: from the kernel BSS end up to the first application slot.  The heap
 * never reaches the transfer area, the slots or the stacks. */
#define HEAP_END       APP_SLOT0_BASE

/* User pointers (channel buffers, names) are range-checked to the flat
 * window that contains the kernel, the heap, the application slots and the
 * transfer area.  This is a range check, not a protection check. */
#ifdef QDOS_LAYOUT_R9
/* R9: applications may keep buffers in profile 3, which ends at 0x600000 (0x700000 with QDOS_PG_WIN3). */
#define USER_RANGE_END (APP_PROFILE3_BASE + APP_PROFILE3_SIZE)
#else
#define USER_RANGE_END (XFER_BASE + XFER_SIZE)
#endif

/* Layout invariants, checked at compile time. */
#if XFER_BASE < APP_SLOT1_BASE + APP_SLOT1_SIZE
#error "transfer area overlaps application slot 1"
#endif
#if APP_SLOT0_BASE + APP_SLOT0_SIZE > APP_SLOT1_BASE
#error "application slots overlap"
#endif
#ifdef QDOS_LAYOUT_R9
#if APP_PROFILE2_BASE < XFER_BASE + XFER_SIZE
#error "profile 2 overlaps the transfer area"
#endif
#if APP_PROFILE2_BASE + APP_PROFILE2_SIZE > APP_PROFILE3_BASE
#error "profiles 2 and 3 overlap"
#endif
#ifdef QDOS_PG_WIN3
#if APP_PROFILE3_BASE + APP_PROFILE3_SIZE > 0x00700000u
#error "profile 3 reaches the boot stack"
#endif
#else
#if APP_PROFILE3_BASE + APP_PROFILE3_SIZE > 0x00600000u
#error "profile 3 reaches the boot stack"
#endif
#endif
#endif
#if RAMDISK_SIZE != 0
#if RAMDISK_BASE < (XFER_BASE + XFER_SIZE)
#error "RAM disk overlaps the transfer area"
#endif
#if RAMDISK_BASE + RAMDISK_SIZE > RAM_TOP
#error "RAM disk does not fit in guest RAM"
#endif
#ifdef QDOS_LAYOUT_R9
#if RAMDISK_BASE < 0x00800000u
#error "RAM disk overlaps the stacks or firmware (0x600000..0x7FFFFF)"
#endif
#if RAMDISK_BASE < DEVICE_BASE + DEVICE_SIZE
#error "RAM disk overlaps the device page"
#endif
#else
#if RAMDISK_BASE + RAMDISK_SIZE > 0x00600000u
#error "RAM disk overlaps the stacks or firmware (0x600000 and up)"
#endif
#endif
#endif

#endif
