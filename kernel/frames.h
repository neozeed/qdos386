/*
 * QDOS/386 R12A - physical frame allocator (QDOS_DYNPROC kernels).  See docs/R12A-MEMORY-MAP.md.
 *
 * One bit per 4 KiB frame below FR_LIMIT.  A frame can be handed out only if it was added to the pool (fr_add_range) and
 * is not in use.  Frames are allocated singly: no contiguity is promised or needed.  The counters always satisfy
 * used + free == pool, which the kernel and the host tests check (fr_check).
 *
 * Pure C89, no kernel dependencies: tests/host/r12a builds this file directly.
 */
#ifndef QDOS386_FRAMES_H
#define QDOS386_FRAMES_H

#define FR_PAGE      4096u
#define FR_LIMIT     0x01000000u                 /* 16 MiB: the identity-mapped physical range */
#define FR_NFRAMES   (FR_LIMIT / FR_PAGE)        /* 4096 */

#define FR_OK         0
#define FR_ERR_ARG   (-1)    /* unaligned, out of range or empty range */
#define FR_ERR_POOL  (-2)    /* frame is not in the pool */
#define FR_ERR_FREE  (-3)    /* frame is already free (double free) */

struct fr_pool {
	unsigned char member[FR_NFRAMES / 8];       /* frame belongs to the pool */
	unsigned char used[FR_NFRAMES / 8];         /* frame is allocated */
	unsigned int pool;                          /* frames in the pool */
	unsigned int nused;                         /* frames allocated */
	unsigned int hint;                          /* next frame index to try */
	unsigned int low_water;                     /* fewest free frames seen */
	unsigned long allocs, frees, refusals;      /* lifetime counters */
};

void fr_init(struct fr_pool *p);
/* Add [lo, hi) (page aligned, below FR_LIMIT) to the pool.  Returns the number of frames added (already-present frames
 * are not counted twice), or FR_ERR_ARG. */
int fr_add_range(struct fr_pool *p, unsigned int lo, unsigned int hi);
/* Remove one frame from the pool (boot pattern test failure).  The frame must be free. */
int fr_remove(struct fr_pool *p, unsigned int pa);
/* One free frame: its physical address, or 0 if none (0 is never a pool frame). */
unsigned int fr_alloc(struct fr_pool *p);
int fr_free(struct fr_pool *p, unsigned int pa);
int fr_in_pool(const struct fr_pool *p, unsigned int pa);
int fr_is_used(const struct fr_pool *p, unsigned int pa);
unsigned int fr_free_count(const struct fr_pool *p);
/* Recount both bitmaps: 1 if the counters agree with the bits and no used frame is outside the pool. */
int fr_check(const struct fr_pool *p);

#endif
