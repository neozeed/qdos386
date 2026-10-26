#ifndef QDOS386_APPMAP_H
#define QDOS386_APPMAP_H

/*
 * QDOS/386 R5 - fixed memory map shared by kernel, loader and applications.
 *
 * Both targets have 8 MiB of RAM.  The kernel is linked at 0 on the i386VM
 * harness and at 0x100000 on the Tiny386 PC; its BSS ends below HEAP_END.
 *
 *   0x000000 .. kernel image and BSS (target dependent, below HEAP_END)
 *   HEAP_START .. HEAP_END     kernel heap (job stacks, channels, user blocks)
 *   APP_SLOT0 0x200000 .. 0x23FFFF   application slot 0 (256 KiB)
 *   APP_SLOT1 0x240000 .. 0x27FFFF   application slot 1 (256 KiB)
 *   XFER 0x280000 .. 0x2BFFFF        transfer area: directory + a.out bytes,
 *                                     written by the transport, read-only to
 *                                     applications after load
 *   0x2C0000 .. 0x5FFFFF             unused in R5 (reserved, not in the heap)
 *   0x600000 .. 0x7FFFFF             boot stack and firmware data (R3 policy)
 *
 * An application is linked for exactly one slot base, which is its entry
 * address.  The loader accepts an image only at a slot base.
 */
#define APP_SLOT0_BASE 0x00200000u
#define APP_SLOT1_BASE 0x00240000u
#define APP_SLOT_SIZE  0x00040000u
#define APP_SLOTS      2

#define XFER_BASE      0x00280000u
#define XFER_SIZE      0x00040000u

/* R6: FAT16 volume on the i386VM (preloaded RAM disk, see platform/i386vm).
 * Reserved outside the kernel, heap, application slots, transfer area and
 * stacks (boot stack top 0x7ffff0 grows down; firmware area from 0x600000).
 * The region is 3 MiB; the volume image is exactly this size. */
#define RAMDISK_BASE   0x002c0000u
#define RAMDISK_SIZE   0x00300000u
#define RAMDISK_FLOOR  0x00600000u   /* first address the stacks or firmware use */

/* Heap: from the kernel BSS end up to the first application slot.  The heap
 * no longer reaches the top of RAM (R4 used 0x600000). */
#define HEAP_END       APP_SLOT0_BASE

/* User pointers (channel buffers, names) are range-checked to the flat
 * window that contains the kernel, the heap, the application slots and the
 * transfer area.  This is a range check, not a protection check. */
#define USER_RANGE_END (XFER_BASE + XFER_SIZE)

/* The RAM disk must sit above the transfer area and below the stack floor. */
#if RAMDISK_BASE < (XFER_BASE + XFER_SIZE)
#error "RAM disk overlaps the transfer area"
#endif
#if RAMDISK_BASE + RAMDISK_SIZE > RAMDISK_FLOOR
#error "RAM disk overlaps the stacks or firmware"
#endif

#endif
