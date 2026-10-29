/*
 * QDOS/386 R12A - private process heap (QDOS_DYNPROC kernels).
 *
 * The same discipline as the kernel heap (memory.c: 16-byte headers with size/magic/owner/next, first fit in address
 * order, coalescing, zero-filled blocks, the same error codes), on an arena that lives in the PROCESS's own address space
 * and grows upward page by page through a callback, up to a cap.  Every header access is a dereference of a process
 * address, so in the kernel the functions run only in the owning process's own trap (its directory active).  The growth
 * callback maps frames (proc.c, under the template) and returns to the process's directory before the allocator writes
 * the new free block.
 *
 * Pure C89; tests/host/r12a builds it with -DPH_HOST against a simulated arena.
 */
#ifndef QDOS386_PHEAP_H
#define QDOS386_PHEAP_H

#define PH_ALIGN       16u
#define PH_HEADER      16u
#define PH_MIN_BLOCK   32u
#define PH_GROW_MIN    0x4000u               /* grow by at least 16 KiB at a time */

/* Same values as MEM_ERR_* (memory.h), so callers see the codes they know. */
#define PH_OK            0
#define PH_ERR_NOMEM   (-3)
#define PH_ERR_ZERO    (-4)
#define PH_ERR_OVERFLOW (-5)
#define PH_ERR_BADPTR  (-6)
#define PH_ERR_DOUBLE  (-7)
#define PH_ERR_OWNER   (-8)
#define PH_ERR_CORRUPT (-9)

#ifdef PH_HOST
extern unsigned char *ph_host_mem;           /* simulated process memory; addresses are offsets into it */
#define PH_W(a) ((unsigned int *) (ph_host_mem + (a)))
#else
#define PH_W(a) ((unsigned int *) (a))
#endif

/* Grow callback: make [brk, want) usable (page multiples).  Returns the new end actually reached (>= brk). */
typedef unsigned int (*ph_grow_fn)(void *ctx, unsigned int brk, unsigned int want);

struct pheap {
	unsigned int base;          /* page aligned start */
	unsigned int brk;           /* end of the usable part */
	unsigned int cap;           /* base + maximum size */
	unsigned int free_head;     /* first free block (address order), 0 = none */
	unsigned int owner;
	unsigned int used;          /* bytes in used blocks, headers included */
	unsigned int nused;         /* used blocks */
	unsigned long grows;        /* successful growth steps */
	ph_grow_fn grow;
	void *ctx;
};

struct ph_stats {
	unsigned int total;         /* brk - base */
	unsigned int used;
	unsigned int free;
	unsigned int largest_free;
	unsigned int used_blocks;
	unsigned int free_blocks;
};

void ph_init(struct pheap *h, unsigned int base, unsigned int cap, unsigned int owner, ph_grow_fn grow, void *ctx);
/* Pointer (as an unsigned value) or a negative PH_ERR_*. */
unsigned int ph_alloc(struct pheap *h, unsigned int size);
int ph_free(struct pheap *h, unsigned int ptr, unsigned int owner);
int ph_stats(const struct pheap *h, struct ph_stats *out);   /* PH_OK or PH_ERR_CORRUPT */
int ph_validate(const struct pheap *h);                       /* PH_OK or PH_ERR_CORRUPT */

#endif
