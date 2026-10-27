#ifndef QDOS386_REGION_H
#define QDOS386_REGION_H

/*
 * QDOS/386 R11H application-region allocator.
 *
 * A pool holds a platform memory map: a short list of spans, each either FREE
 * (may hold application regions) or RESERVED (never allocated: kernel, heap,
 * transfer area, boot stack, device page, FAT16 transport, ...).  Spans are
 * data, supplied by the platform.  Nothing here knows what a span is for, so a
 * RAM-backed FAT16 transport can be described as RESERVED today and changed to
 * FREE later without touching this code.
 *
 * Regions are page-aligned byte ranges [base, end) inside FREE spans.  An
 * allocation is either first-fit (region_alloc) or at a fixed address
 * (region_reserve_at), which the loader needs for an executable linked at a
 * given origin.  The allocator never moves a region.
 *
 * Handles carry a generation number, so a stale handle is rejected after its
 * region has been released and its slot reused.
 *
 * Pure C89, no kernel dependencies: the host test builds this file directly.
 */

#define REGION_PAGE        4096ul
#define REGION_MAX_SPANS   16
#define REGION_MAX         8

#define REGION_SPAN_FREE     1
#define REGION_SPAN_RESERVED 2

/* Error codes (negative).  A handle is a positive value. */
#define REGION_OK            0
#define REGION_ERR_ARG    (-1)   /* bad argument: zero/overflowing size, unaligned base, bad span */
#define REGION_ERR_NOMEM  (-2)   /* no free span can hold the request */
#define REGION_ERR_BUSY   (-3)   /* fixed address overlaps a live region or a reserved span */
#define REGION_ERR_HANDLE (-4)   /* stale, unknown or already-released handle */
#define REGION_ERR_OWNER  (-5)   /* handle belongs to a different owner */
#define REGION_ERR_FULL   (-6)   /* all region slots in use */

struct region_span {
	unsigned long base;      /* inclusive, page aligned */
	unsigned long end;       /* exclusive, page aligned, end > base */
	int kind;                /* REGION_SPAN_FREE or REGION_SPAN_RESERVED */
};

struct region {
	int used;
	unsigned long base;
	unsigned long end;
	unsigned int owner;
	unsigned long gen;       /* generation of the current or last use of this slot */
};

struct region_pool {
	struct region_span spans[REGION_MAX_SPANS];
	int nspans;
	struct region regions[REGION_MAX];
	unsigned long gen;       /* last generation issued */
};

/* Validates the spans (page aligned, non-empty, sorted, non-overlapping, known
 * kind) and copies them into the pool.  Clears all regions. */
int region_init(struct region_pool *p, const struct region_span *spans, int n);

/* First-fit allocation of bytes (rounded up to a page) for owner.  Returns a
 * positive handle, or a negative REGION_ERR_*. */
long region_alloc(struct region_pool *p, unsigned long bytes, unsigned int owner);

/* Allocation at a fixed base, for an executable linked at that origin.  The
 * whole [base, base+bytes) must lie inside one FREE span and overlap no live
 * region.  Returns a positive handle or REGION_ERR_BUSY / REGION_ERR_ARG. */
long region_reserve_at(struct region_pool *p, unsigned long base, unsigned long bytes,
                       unsigned int owner);

/* Releases a region.  The owner must match.  Returns REGION_OK or a negative code. */
int region_release(struct region_pool *p, long handle, unsigned int owner);

/* Reports the address range of a live handle.  Returns REGION_OK or a negative code. */
int region_range(const struct region_pool *p, long handle, unsigned long *base,
                 unsigned long *end);

/* Checks the invariants: every live region lies inside one FREE span, and live
 * regions are pairwise disjoint.  Returns 1 if they hold, 0 otherwise. */
int region_check(const struct region_pool *p);

/* Number of live regions and total bytes they hold (page-rounded). */
int region_live(const struct region_pool *p, unsigned long *bytes);

#endif
