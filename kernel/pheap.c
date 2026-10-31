/*
 * QDOS/386 R12A - private process heap.  See pheap.h.  Block header (4 words): size, magic, owner, next-free.
 */
#include "pheap.h"

#define PH_USED 0x50555344u   /* "DSUP" */
#define PH_FREE 0x50465245u   /* "ERFP" */

#define SZ(a)   PH_W(a)[0]
#define MG(a)   PH_W(a)[1]
#define OW(a)   PH_W(a)[2]
#define NX(a)   PH_W(a)[3]

void ph_init(struct pheap *h, unsigned int base, unsigned int cap, unsigned int owner, ph_grow_fn grow, void *ctx)
{
	h->base = base;
	h->brk = base;
	h->cap = cap;
	h->free_head = 0;
	h->owner = owner;
	h->used = 0;
	h->nused = 0;
	h->grows = 0;
	h->grow = grow;
	h->ctx = ctx;
}

/* Insert the block at cur (SZ set) into the address-ordered free list, merging with neighbours. */
static void free_insert(struct pheap *h, unsigned int cur)
{
	unsigned int p = 0, q = h->free_head;

	while (q != 0 && q < cur) {
		p = q;
		q = NX(q);
	}
	MG(cur) = PH_FREE;
	OW(cur) = 0;
	NX(cur) = q;
	if (p != 0)
		NX(p) = cur;
	else
		h->free_head = cur;
	if (q != 0 && cur + SZ(cur) == q) {
		SZ(cur) += SZ(q);
		NX(cur) = NX(q);
	}
	if (p != 0 && p + SZ(p) == cur) {
		SZ(p) += SZ(cur);
		NX(p) = NX(cur);
	}
}

/* Extend the arena so that a block of need bytes fits.  Returns 1 if it grew at all. */
static int grow_for(struct pheap *h, unsigned int need)
{
	unsigned int want, got, old, room = h->cap - h->brk;

	if (h->grow == 0 || room < PH_MIN_BLOCK)
		return 0;
	want = need < PH_GROW_MIN ? PH_GROW_MIN : need;
	want = (want + 4095u) & ~4095u;
	if (want > room)
		want = room & ~4095u;
	if (want < need)
		return 0;
	old = h->brk;
	got = h->grow(h->ctx, old, old + want);
	if (got <= old || got > old + want || got - old < PH_MIN_BLOCK)
		return 0;
	SZ(old) = got - old;
	h->brk = got;
	h->grows++;
	free_insert(h, old);                     /* merges with a free block that ends at the old brk */
	return 1;
}

unsigned int ph_alloc(struct pheap *h, unsigned int size)
{
	unsigned int need, prev, cur, blk, remain, i, *w;
	int tries;

	if (size == 0)
		return (unsigned int) PH_ERR_ZERO;
	if (size > 0x7FFFFFE0u)
		return (unsigned int) PH_ERR_OVERFLOW;
	need = ((size + PH_ALIGN - 1u) & ~(PH_ALIGN - 1u)) + PH_HEADER;
	for (tries = 0; tries < 2; tries++) {
		prev = 0;
		cur = h->free_head;
		while (cur != 0 && SZ(cur) < need) {
			prev = cur;
			cur = NX(cur);
		}
		if (cur != 0) {
			remain = SZ(cur) - need;
			if (remain >= PH_MIN_BLOCK) {
				blk = cur + need;
				SZ(blk) = remain;
				MG(blk) = PH_FREE;
				OW(blk) = 0;
				NX(blk) = NX(cur);
				if (prev != 0)
					NX(prev) = blk;
				else
					h->free_head = blk;
				SZ(cur) = need;
			} else {
				if (prev != 0)
					NX(prev) = NX(cur);
				else
					h->free_head = NX(cur);
			}
			MG(cur) = PH_USED;
			OW(cur) = h->owner;
			NX(cur) = 0;
			w = PH_W(cur + PH_HEADER);
			for (i = 0; i < (SZ(cur) - PH_HEADER) / 4u; i++)
				w[i] = 0;
			h->used += SZ(cur);
			h->nused++;
			return cur + PH_HEADER;
		}
		if (tries == 0 && !grow_for(h, need))
			break;
	}
	return (unsigned int) PH_ERR_NOMEM;
}

int ph_free(struct pheap *h, unsigned int ptr, unsigned int owner)
{
	unsigned int cur;

	if (ptr < h->base + PH_HEADER || ptr >= h->brk || ((ptr - h->base) & (PH_ALIGN - 1u)))
		return PH_ERR_BADPTR;
	cur = ptr - PH_HEADER;
	if (MG(cur) == PH_FREE)
		return PH_ERR_DOUBLE;
	if (MG(cur) != PH_USED)
		return PH_ERR_BADPTR;
	if (SZ(cur) < PH_MIN_BLOCK || (SZ(cur) & (PH_ALIGN - 1u)) || SZ(cur) > h->brk - cur)
		return PH_ERR_CORRUPT;
	if (OW(cur) != owner)
		return PH_ERR_OWNER;
	h->used -= SZ(cur);
	h->nused--;
	free_insert(h, cur);
	return PH_OK;
}

static int walk(const struct pheap *h, struct ph_stats *st)
{
	unsigned int cur = h->base, n = 0, limit = (h->brk - h->base) / PH_MIN_BLOCK + 1u;
	int prev_free = 0;

	st->total = h->brk - h->base;
	st->used = st->free = st->largest_free = st->used_blocks = st->free_blocks = 0;
	while (cur != h->brk) {
		if (n++ > limit || SZ(cur) < PH_MIN_BLOCK || (SZ(cur) & (PH_ALIGN - 1u)) || SZ(cur) > h->brk - cur)
			return PH_ERR_CORRUPT;
		if (MG(cur) == PH_USED) {
			st->used += SZ(cur);
			st->used_blocks++;
			prev_free = 0;
		} else if (MG(cur) == PH_FREE) {
			if (prev_free)
				return PH_ERR_CORRUPT;           /* two adjacent free blocks: not coalesced */
			st->free += SZ(cur);
			st->free_blocks++;
			if (SZ(cur) > st->largest_free)
				st->largest_free = SZ(cur);
			prev_free = 1;
		} else {
			return PH_ERR_CORRUPT;
		}
		cur += SZ(cur);
	}
	return PH_OK;
}

int ph_stats(const struct pheap *h, struct ph_stats *out)
{
	return walk(h, out);
}

int ph_validate(const struct pheap *h)
{
	struct ph_stats st;
	unsigned int q, last = 0, n = 0;

	if (walk(h, &st) != PH_OK || st.used != h->used || st.used_blocks != h->nused)
		return PH_ERR_CORRUPT;
	for (q = h->free_head; q != 0; q = NX(q)) {
		if (n++ > st.free_blocks || q < h->base || q >= h->brk || MG(q) != PH_FREE || (last != 0 && q <= last))
			return PH_ERR_CORRUPT;
		last = q;
	}
	return n == st.free_blocks ? PH_OK : PH_ERR_CORRUPT;
}
