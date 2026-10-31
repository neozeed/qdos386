/*
 * QDOS/386 R11H application-region allocator.  See region.h.
 */
#include "region.h"

static int span_valid(const struct region_span *s)
{
	if (s->kind != REGION_SPAN_FREE && s->kind != REGION_SPAN_RESERVED)
		return 0;
	if (s->base % REGION_PAGE != 0 || s->end % REGION_PAGE != 0)
		return 0;
	if (s->end <= s->base)
		return 0;
	return 1;
}

int region_init(struct region_pool *p, const struct region_span *spans, int n)
{
	int i, k;

	if (p == 0 || spans == 0 || n < 1 || n > REGION_MAX_SPANS)
		return REGION_ERR_ARG;
	for (i = 0; i < n; i++) {
		if (!span_valid(&spans[i]))
			return REGION_ERR_ARG;
		if (i > 0 && spans[i].base < spans[i - 1].end)
			return REGION_ERR_ARG;     /* unsorted or overlapping */
	}
	for (k = 0; k < n; k++)
		p->spans[k] = spans[k];
	p->nspans = n;
	for (k = 0; k < REGION_MAX; k++) {
		p->regions[k].used = 0;
		p->regions[k].base = 0;
		p->regions[k].end = 0;
		p->regions[k].owner = 0;
		p->regions[k].gen = 0;
	}
	p->gen = 0;
	return REGION_OK;
}

/* Rounds bytes up to whole pages.  Returns 0 if bytes is zero or the result
 * overflows. */
static unsigned long round_pages(unsigned long bytes)
{
	unsigned long r;

	if (bytes == 0)
		return 0;
	if (bytes > (unsigned long) -1 - (REGION_PAGE - 1))
		return 0;
	r = (bytes + REGION_PAGE - 1) / REGION_PAGE * REGION_PAGE;
	return r;
}

/* Returns the span index containing [base, base+size), or -1.  Overflow-safe. */
static int free_span_for(const struct region_pool *p, unsigned long base, unsigned long size)
{
	int i;

	for (i = 0; i < p->nspans; i++) {
		const struct region_span *s = &p->spans[i];
		if (s->kind != REGION_SPAN_FREE)
			continue;
		if (base >= s->base && base < s->end && size <= s->end - base)
			return i;
	}
	return -1;
}

/* True if [base, base+size) overlaps a live region. */
static int overlaps_live(const struct region_pool *p, unsigned long base, unsigned long size)
{
	int i;

	for (i = 0; i < REGION_MAX; i++) {
		const struct region *r = &p->regions[i];
		if (!r->used)
			continue;
		if (base < r->end && r->base < base + size)
			return 1;
	}
	return 0;
}

/* Finds a free slot, or -1. */
static int free_slot(const struct region_pool *p)
{
	int i;

	for (i = 0; i < REGION_MAX; i++)
		if (!p->regions[i].used)
			return i;
	return -1;
}

/* Builds the handle: slot index in the low 8 bits (index+1, so 0 is never a
 * handle), generation above.  Fits a positive long for generations up to
 * 2^23 on 32-bit targets; wrap is caught by the generation check. */
static long make_handle(int idx, unsigned long gen)
{
	return (long) (((gen & 0x7FFFFFul) << 8) | (unsigned long) (idx + 1));
}

static int decode_handle(const struct region_pool *p, long handle, int *idx)
{
	unsigned long h;
	int i;

	if (handle <= 0)
		return REGION_ERR_HANDLE;
	h = (unsigned long) handle;
	i = (int) (h & 0xFFul) - 1;
	if (i < 0 || i >= REGION_MAX)
		return REGION_ERR_HANDLE;
	if (!p->regions[i].used)
		return REGION_ERR_HANDLE;
	if ((p->regions[i].gen & 0x7FFFFFul) != ((h >> 8) & 0x7FFFFFul))
		return REGION_ERR_HANDLE;
	*idx = i;
	return REGION_OK;
}

static long take(struct region_pool *p, int slot, unsigned long base, unsigned long size,
                 unsigned int owner)
{
	struct region *r = &p->regions[slot];

	p->gen++;
	r->used = 1;
	r->base = base;
	r->end = base + size;
	r->owner = owner;
	r->gen = p->gen;
	return make_handle(slot, p->gen);
}

long region_alloc(struct region_pool *p, unsigned long bytes, unsigned int owner)
{
	unsigned long size;
	int slot, si;
	const struct region_span *s;
	unsigned long cand;
	int progressed;
	int i;

	if (p == 0)
		return REGION_ERR_ARG;
	size = round_pages(bytes);
	if (size == 0)
		return REGION_ERR_ARG;
	slot = free_slot(p);
	if (slot < 0)
		return REGION_ERR_FULL;
	for (si = 0; si < p->nspans; si++) {
		s = &p->spans[si];
		if (s->kind != REGION_SPAN_FREE)
			continue;
		cand = s->base;
		for (;;) {
			if (cand >= s->end || size > s->end - cand)
				break;
			progressed = 0;
			for (i = 0; i < REGION_MAX; i++) {
				const struct region *r = &p->regions[i];
				if (!r->used)
					continue;
				if (cand < r->end && r->base < cand + size) {
					cand = r->end;        /* regions are page aligned */
					progressed = 1;
					break;
				}
			}
			if (!progressed)
				return take(p, slot, cand, size, owner);
		}
	}
	return REGION_ERR_NOMEM;
}

long region_reserve_at(struct region_pool *p, unsigned long base, unsigned long bytes,
                       unsigned int owner)
{
	unsigned long size;
	int slot;

	if (p == 0)
		return REGION_ERR_ARG;
	if (base % REGION_PAGE != 0)
		return REGION_ERR_ARG;
	size = round_pages(bytes);
	if (size == 0)
		return REGION_ERR_ARG;
	if (free_span_for(p, base, size) < 0)
		return REGION_ERR_BUSY;
	if (overlaps_live(p, base, size))
		return REGION_ERR_BUSY;
	slot = free_slot(p);
	if (slot < 0)
		return REGION_ERR_FULL;
	return take(p, slot, base, size, owner);
}

int region_release(struct region_pool *p, long handle, unsigned int owner)
{
	int idx, rc;

	if (p == 0)
		return REGION_ERR_ARG;
	rc = decode_handle(p, handle, &idx);
	if (rc != REGION_OK)
		return rc;
	if (p->regions[idx].owner != owner)
		return REGION_ERR_OWNER;
	p->regions[idx].used = 0;
	return REGION_OK;
}

int region_range(const struct region_pool *p, long handle, unsigned long *base,
                 unsigned long *end)
{
	int idx, rc;

	if (p == 0 || base == 0 || end == 0)
		return REGION_ERR_ARG;
	rc = decode_handle(p, handle, &idx);
	if (rc != REGION_OK)
		return rc;
	*base = p->regions[idx].base;
	*end = p->regions[idx].end;
	return REGION_OK;
}

int region_check(const struct region_pool *p)
{
	int i, j, si, inside;

	if (p == 0)
		return 0;
	for (i = 0; i < REGION_MAX; i++) {
		const struct region *r = &p->regions[i];
		if (!r->used)
			continue;
		if (r->end <= r->base)
			return 0;
		inside = 0;
		for (si = 0; si < p->nspans; si++) {
			const struct region_span *s = &p->spans[si];
			if (s->kind == REGION_SPAN_FREE && r->base >= s->base && r->end <= s->end)
				inside = 1;
		}
		if (!inside)
			return 0;
		for (j = i + 1; j < REGION_MAX; j++) {
			const struct region *q = &p->regions[j];
			if (q->used && r->base < q->end && q->base < r->end)
				return 0;
		}
	}
	return 1;
}

int region_live(const struct region_pool *p, unsigned long *bytes)
{
	int i, n = 0;
	unsigned long total = 0;

	for (i = 0; i < REGION_MAX; i++) {
		if (p->regions[i].used) {
			n++;
			total += p->regions[i].end - p->regions[i].base;
		}
	}
	if (bytes != 0)
		*bytes = total;
	return n;
}
