/*
 * QDOS/386 R3 - native kernel heap (see memory.h for the interface and the
 * interrupt rules).
 *
 * Layout.  The arena is a run of blocks that tile [arena_base, arena_end)
 * exactly, in address order.  Each block starts with a 16-byte header:
 *
 *     +0  size   total block bytes including the header (multiple of 16)
 *     +4  magic  MEM_USED or MEM_FREE
 *     +8  owner  allocation owner (used blocks)
 *     +12 next   next free block by address (free blocks; 0 terminates)
 *
 * The user pointer is block + 16.  Free blocks are kept in one list sorted
 * by address; adjacent free blocks are always merged, so no two free blocks
 * are physically adjacent.
 *
 * Invariants (checked by mem_validate):
 *   I1  blocks tile the arena: each size is a multiple of 16, at least
 *       MEM_MIN_BLOCK, and the walk ends exactly at arena_end;
 *   I2  every magic is MEM_USED or MEM_FREE;
 *   I3  no two physically adjacent blocks are both free;
 *   I4  the free list contains exactly the free blocks, once each, in
 *       strictly increasing address order.
 *
 * Pointer checks (mem_free).  A pointer is accepted only if it lies inside
 * the arena, is 16-byte aligned, and its header holds MEM_USED with the
 * caller's owner.  A header holding MEM_FREE is reported as a double free.
 * Coalescing leaves the magic of the absorbed header as MEM_FREE, so a
 * double free of a merged block is still detected while the arena is not
 * reused.  A stale pointer whose header has since been reused is caught by
 * the magic and owner checks, not by a generation counter.
 *
 * Only the rules above are implemented.  There is no defence against a
 * program that writes into a block it does not own, beyond the owner check
 * on free.  Canary words in guest tests detect overruns instead.
 */
#include "memory.h"

#define MEM_USED 0x55534421u     /* "USD!" */
#define MEM_FREE 0x46524545u     /* "FREE" */

struct mem_hdr {
	unsigned int size;
	unsigned int magic;
	unsigned int owner;
	unsigned int next;
};

extern unsigned int irq_save(void);
extern void irq_restore(unsigned int flags);

static unsigned int arena_base = 0;
static unsigned int arena_end = 0;
static unsigned int free_head = 0;      /* address of first free block, 0 = none */

static struct mem_hdr *hdr(unsigned int addr)
{
	return (struct mem_hdr *) addr;
}

static void zero_bytes(unsigned int addr, unsigned int n)
{
	unsigned char *p = (unsigned char *) addr;
	while (n-- != 0)
		*p++ = 0;
}

/* Insert the free block at cur (header already holds size) into the sorted
 * free list and merge it with its physical neighbours.  IF must be off. */
static void free_insert(unsigned int cur)
{
	unsigned int p = 0;
	unsigned int q = free_head;
	struct mem_hdr *h = hdr(cur);

	while (q != 0 && q < cur) {
		p = q;
		q = hdr(q)->next;
	}
	h->magic = MEM_FREE;
	h->owner = 0;
	h->next = q;
	if (p != 0)
		hdr(p)->next = cur;
	else
		free_head = cur;

	/* merge with the following free block */
	if (q != 0 && cur + h->size == q) {
		h->size += hdr(q)->size;
		h->next = hdr(q)->next;
	}
	/* merge into the preceding free block */
	if (p != 0 && p + hdr(p)->size == cur) {
		hdr(p)->size += h->size;
		hdr(p)->next = h->next;
	}
}

int mem_init(unsigned int start, unsigned int end)
{
	unsigned int s;
	unsigned int e;

	if (end < start)
		return MEM_ERR_RANGE;
	s = (start + MEM_ALIGN - 1) & ~(MEM_ALIGN - 1);
	e = end & ~(MEM_ALIGN - 1);
	if (s < start || e <= s || e - s < MEM_MIN_BLOCK)
		return MEM_ERR_RANGE;

	arena_base = s;
	arena_end = e;
	free_head = s;
	hdr(s)->size = e - s;
	hdr(s)->magic = MEM_FREE;
	hdr(s)->owner = 0;
	hdr(s)->next = 0;
	return MEM_OK;
}

void *mem_alloc(unsigned int size, unsigned int owner)
{
	unsigned int need;
	unsigned int flags;
	unsigned int prev = 0;
	unsigned int cur;
	unsigned int blk;
	unsigned int remain;
	struct mem_hdr *h;

	if (size == 0)
		return (void *) MEM_ERR_ZERO;
	if (size > 0xffffffe0u)
		return (void *) MEM_ERR_OVERFLOW;
	need = ((size + MEM_ALIGN - 1) & ~(MEM_ALIGN - 1)) + MEM_HEADER;

	flags = irq_save();
	if (arena_base == 0) {
		irq_restore(flags);
		return (void *) MEM_ERR_CORRUPT;
	}

	/* first fit, in address order */
	cur = free_head;
	while (cur != 0 && hdr(cur)->size < need) {
		prev = cur;
		cur = hdr(cur)->next;
	}
	if (cur == 0) {
		irq_restore(flags);
		return (void *) MEM_ERR_NOMEM;
	}

	h = hdr(cur);
	remain = h->size - need;
	if (remain >= MEM_MIN_BLOCK) {
		/* split: the tail stays free and takes this block's place in the list */
		blk = cur + need;
		hdr(blk)->size = remain;
		hdr(blk)->magic = MEM_FREE;
		hdr(blk)->owner = 0;
		hdr(blk)->next = h->next;
		if (prev != 0)
			hdr(prev)->next = blk;
		else
			free_head = blk;
		h->size = need;
	} else {
		/* take the whole block */
		if (prev != 0)
			hdr(prev)->next = h->next;
		else
			free_head = h->next;
	}
	h->magic = MEM_USED;
	h->owner = owner;
	h->next = 0;
	zero_bytes(cur + MEM_HEADER, h->size - MEM_HEADER);
	irq_restore(flags);
	return (void *) (cur + MEM_HEADER);
}

/* Check a user pointer and return its header address, or 0. */
static unsigned int block_of(void *ptr)
{
	unsigned int addr = (unsigned int) ptr;
	unsigned int cur;

	if (arena_base == 0 || addr < arena_base + MEM_HEADER || addr >= arena_end)
		return 0;
	cur = addr - MEM_HEADER;
	if ((cur - arena_base) & (MEM_ALIGN - 1))
		return 0;
	return cur;
}

int mem_free(void *ptr, unsigned int owner)
{
	unsigned int flags;
	unsigned int cur;
	struct mem_hdr *h;
	int rc = MEM_OK;

	flags = irq_save();
	cur = block_of(ptr);
	if (cur == 0) {
		irq_restore(flags);
		return MEM_ERR_BADPTR;
	}
	h = hdr(cur);
	if (h->magic == MEM_FREE) {
		irq_restore(flags);
		return MEM_ERR_DOUBLE;
	}
	if (h->magic != MEM_USED) {
		irq_restore(flags);
		return MEM_ERR_BADPTR;
	}
	if (h->size < MEM_MIN_BLOCK || (h->size & (MEM_ALIGN - 1)) ||
	    h->size > arena_end - cur) {
		irq_restore(flags);
		return MEM_ERR_CORRUPT;
	}
	if (h->owner != owner) {
		irq_restore(flags);
		return MEM_ERR_OWNER;
	}
	free_insert(cur);
	irq_restore(flags);
	return rc;
}

unsigned int mem_reclaim_owner(unsigned int owner)
{
	unsigned int flags;
	unsigned int freed = 0;
	unsigned int cur;
	unsigned int n;
	int again;

	if (owner == MEM_OWNER_KERNEL || arena_base == 0)
		return 0;
	flags = irq_save();
	/* Each free changes the physical layout, so rescan from the start.  The
	 * arena holds a bounded number of blocks, so this terminates. */
	do {
		again = 0;
		cur = arena_base;
		n = 0;
		while (cur < arena_end && n < (arena_end - arena_base) / MEM_MIN_BLOCK) {
			struct mem_hdr *h = hdr(cur);
			if (h->size < MEM_MIN_BLOCK || (h->size & (MEM_ALIGN - 1)) ||
			    h->size > arena_end - cur)
				break;
			if (h->magic == MEM_USED && h->owner == owner) {
				freed += h->size;
				free_insert(cur);
				again = 1;
				break;
			}
			cur += h->size;
			n++;
		}
	} while (again);
	irq_restore(flags);
	return freed;
}

unsigned int mem_owner_of(void *ptr, int *ok)
{
	unsigned int flags;
	unsigned int cur;
	unsigned int owner = MEM_OWNER_KERNEL;

	flags = irq_save();
	cur = block_of(ptr);
	if (cur != 0 && hdr(cur)->magic == MEM_USED) {
		owner = hdr(cur)->owner;
		if (ok != 0)
			*ok = 1;
	} else if (ok != 0) {
		*ok = 0;
	}
	irq_restore(flags);
	return owner;
}

/* Physical walk shared by mem_stats and mem_validate.  Returns MEM_OK when
 * I1 and I2 hold and fills the counters; MEM_ERR_CORRUPT otherwise. */
static int walk(struct mem_stats *st, int *coalesced_ok)
{
	unsigned int cur = arena_base;
	unsigned int n = 0;
	unsigned int limit = (arena_end - arena_base) / MEM_MIN_BLOCK;
	int prev_free = 0;

	st->total = arena_end - arena_base;
	st->used = 0;
	st->free = 0;
	st->largest_free = 0;
	st->used_blocks = 0;
	st->free_blocks = 0;
	*coalesced_ok = 1;

	while (cur != arena_end) {
		struct mem_hdr *h = hdr(cur);
		if (n++ >= limit)
			return MEM_ERR_CORRUPT;
		if (h->size < MEM_MIN_BLOCK || (h->size & (MEM_ALIGN - 1)) ||
		    h->size > arena_end - cur)
			return MEM_ERR_CORRUPT;                        /* I1 */
		if (h->magic == MEM_USED) {
			st->used += h->size;
			st->used_blocks++;
			prev_free = 0;
		} else if (h->magic == MEM_FREE) {
			if (prev_free)
				*coalesced_ok = 0;                         /* I3 */
			st->free += h->size;
			st->free_blocks++;
			if (h->size > st->largest_free)
				st->largest_free = h->size;
			prev_free = 1;
		} else {
			return MEM_ERR_CORRUPT;                        /* I2 */
		}
		cur += h->size;
	}
	return MEM_OK;
}

int mem_stats(struct mem_stats *out)
{
	unsigned int flags;
	int coalesced;
	int rc;

	flags = irq_save();
	if (arena_base == 0) {
		irq_restore(flags);
		return MEM_ERR_CORRUPT;
	}
	rc = walk(out, &coalesced);
	irq_restore(flags);
	if (rc != MEM_OK || !coalesced)
		return MEM_ERR_CORRUPT;
	return MEM_OK;
}

int mem_validate(void)
{
	unsigned int flags;
	struct mem_stats st;
	unsigned int q;
	unsigned int last = 0;
	unsigned int nlist = 0;
	unsigned int limit;
	int coalesced;
	int rc;

	flags = irq_save();
	if (arena_base == 0) {
		irq_restore(flags);
		return MEM_ERR_CORRUPT;
	}
	rc = walk(&st, &coalesced);                              /* I1, I2 */
	if (rc != MEM_OK || !coalesced) {                        /* I3 */
		irq_restore(flags);
		return MEM_ERR_CORRUPT;
	}
	/* I4: the list visits every free block once, in address order */
	limit = (arena_end - arena_base) / MEM_MIN_BLOCK;
	for (q = free_head; q != 0; q = hdr(q)->next) {
		if (nlist++ >= limit ||
		    q < arena_base || q >= arena_end ||
		    hdr(q)->magic != MEM_FREE ||
		    (last != 0 && q <= last)) {
			irq_restore(flags);
			return MEM_ERR_CORRUPT;
		}
		last = q;
	}
	irq_restore(flags);
	return (nlist == st.free_blocks) ? MEM_OK : MEM_ERR_CORRUPT;
}
