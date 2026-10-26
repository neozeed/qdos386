/*
 * QDOS/386 R3 - native kernel heap.
 *
 * A non-moving, fixed-address arena with explicit 16-byte block headers,
 * a free list sorted by address, and coalescing of adjacent free blocks.
 * Every allocation carries an owner value.  Blocks are never moved, so a
 * pointer returned by mem_alloc is valid until it is freed or reclaimed.
 *
 * This is the internal kernel API.  The trap-level services that job code
 * uses (QT_MEM_ALLOC and friends in jobs.h) are a separate layer that
 * supplies the owner from the calling job.  Neither layer is a compatibility
 * implementation of the JS ROM allocator: see docs/JS-ROM-MEMORY-MAP.md and
 * docs/R3-MEMORY-ABI.md.
 *
 * Interrupt rules:
 *   - mem_alloc, mem_free, mem_reclaim_owner and mem_init mutate the heap.
 *     They are called only from trap context (interrupt gate, IF=0) or
 *     from boot code with IF=0.  Each one also saves and restores IF so it
 *     is safe if that rule is broken, but the timer ISR must never call them.
 *   - mem_validate and mem_stats walk the heap and are for job or boot
 *     context only.
 */
#ifndef QDOS386_MEMORY_H
#define QDOS386_MEMORY_H

#define MEM_ALIGN       16u          /* every user pointer is 16-byte aligned */
#define MEM_HEADER      16u          /* block header size (also the alignment) */
#define MEM_MIN_BLOCK   32u          /* smallest block that may be split off */

/* Arena: from the 16-byte-aligned end of BSS up to MEM_ARENA_END.  Both
 * targets have 8 MiB of RAM.  The top 2 MiB (0x600000..0x7ffff0) is kept
 * for the boot stack and for SeaBIOS data on the PC, so the arena never
 * reaches it.  The lower bound is the linker symbol bss_end. */
#define MEM_ARENA_END   0x600000u

/* Owner values.  Job owners are tagged job ids; SYSTEM's id is 0. */
#define MEM_OWNER_KERNEL 0xffffffffu /* kernel-owned, never reclaimed */

/* Return codes (negative values are errors; pointer results are unsigned). */
#define MEM_OK             0
#define MEM_ERR_NOMEM    (-3)        /* same value as the JS ROM out-of-memory */
#define MEM_ERR_ZERO     (-4)        /* zero-size request */
#define MEM_ERR_OVERFLOW (-5)        /* size too large to round or to fit */
#define MEM_ERR_BADPTR   (-6)        /* not a block start inside the arena */
#define MEM_ERR_DOUBLE   (-7)        /* block is already free */
#define MEM_ERR_OWNER    (-8)        /* block belongs to another owner */
#define MEM_ERR_CORRUPT  (-9)        /* header or list invariant broken */
#define MEM_ERR_RANGE   (-10)        /* invalid arena bounds at init */

struct mem_stats {
	unsigned int total;          /* arena bytes */
	unsigned int used;           /* bytes in allocated blocks, headers included */
	unsigned int free;           /* bytes in free blocks */
	unsigned int largest_free;   /* largest single free block */
	unsigned int used_blocks;
	unsigned int free_blocks;
};

/* Initialise the arena [start, end).  Bounds are rounded inward to 16 bytes.
 * Returns MEM_OK or MEM_ERR_RANGE. */
int mem_init(unsigned int start, unsigned int end);

/* Allocate size bytes for owner.  The block is zero-filled.  Returns the
 * user pointer, or an error code as an unsigned value (see mem_is_error). */
void *mem_alloc(unsigned int size, unsigned int owner);

/* Free the block at ptr.  Requires owner to match the block's owner.
 * Returns MEM_OK or a negative error code. */
int mem_free(void *ptr, unsigned int owner);

/* Free every block owned by owner.  Returns the number of bytes released
 * (headers included). */
unsigned int mem_reclaim_owner(unsigned int owner);

/* Fill *out with current figures.  Returns MEM_OK, or MEM_ERR_CORRUPT if
 * the walk found a broken invariant (out is then partly filled). */
int mem_stats(struct mem_stats *out);

/* Walk the whole arena and check every invariant (see memory.c).
 * Returns MEM_OK or MEM_ERR_CORRUPT. */
int mem_validate(void);

/* Owner of the block at ptr, or MEM_OWNER_KERNEL with *ok = 0 when the
 * pointer is not a valid block start.  Used for stale-owner checks. */
unsigned int mem_owner_of(void *ptr, int *ok);

/* Returns nonzero if v, read as a pointer or count result, is an error code. */
#define mem_is_error(v) (((int)(v)) < 0 && ((int)(v)) >= -64)

#endif
