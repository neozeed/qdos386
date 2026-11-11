/*
 * QDOS/386 R12A - physical frame allocator.  See frames.h.
 */
#include "frames.h"

#define BIT(a, i)    ((a)[(i) >> 3] & (unsigned char) (1u << ((i) & 7u)))
#define SET(a, i)    ((a)[(i) >> 3] |= (unsigned char) (1u << ((i) & 7u)))
#define CLR(a, i)    ((a)[(i) >> 3] &= (unsigned char) ~(1u << ((i) & 7u)))

void fr_init(struct fr_pool *p)
{
	unsigned int i;

	for (i = 0; i < FR_NFRAMES / 8u; i++) {
		p->member[i] = 0;
		p->used[i] = 0;
	}
	p->pool = 0;
	p->nused = 0;
	p->hint = 0;
	p->low_water = 0;
	p->allocs = 0;
	p->frees = 0;
	p->refusals = 0;
}

int fr_add_range(struct fr_pool *p, unsigned int lo, unsigned int hi)
{
	unsigned int i, n = 0;

	if ((lo & (FR_PAGE - 1u)) || (hi & (FR_PAGE - 1u)) || hi <= lo || hi > FR_LIMIT || lo == 0)
		return FR_ERR_ARG;
	for (i = lo / FR_PAGE; i < hi / FR_PAGE; i++)
		if (!BIT(p->member, i)) {
			SET(p->member, i);
			n++;
		}
	p->pool += n;
	p->low_water = fr_free_count(p);       /* pool set-up happens at boot, before any allocation */
	return (int) n;
}

int fr_remove(struct fr_pool *p, unsigned int pa)
{
	unsigned int i = pa / FR_PAGE;

	if ((pa & (FR_PAGE - 1u)) || pa >= FR_LIMIT)
		return FR_ERR_ARG;
	if (!BIT(p->member, i))
		return FR_ERR_POOL;
	if (BIT(p->used, i))
		return FR_ERR_FREE;
	CLR(p->member, i);
	p->pool--;
	p->low_water = fr_free_count(p);
	return FR_OK;
}

unsigned int fr_alloc(struct fr_pool *p)
{
	unsigned int n, i;

	if (p->nused >= p->pool) {
		p->refusals++;
		return 0;
	}
	for (n = 0; n < FR_NFRAMES; n++) {
		i = (p->hint + n) % FR_NFRAMES;
		if (BIT(p->member, i) && !BIT(p->used, i)) {
			SET(p->used, i);
			p->nused++;
			p->allocs++;
			p->hint = (i + 1u) % FR_NFRAMES;
			if (fr_free_count(p) < p->low_water)
				p->low_water = fr_free_count(p);
			return i * FR_PAGE;
		}
	}
	p->refusals++;               /* counters disagree with the bits: fr_check reports it */
	return 0;
}

int fr_free(struct fr_pool *p, unsigned int pa)
{
	unsigned int i = pa / FR_PAGE;

	if ((pa & (FR_PAGE - 1u)) || pa >= FR_LIMIT || pa == 0)
		return FR_ERR_ARG;
	if (!BIT(p->member, i))
		return FR_ERR_POOL;
	if (!BIT(p->used, i))
		return FR_ERR_FREE;
	CLR(p->used, i);
	p->nused--;
	p->frees++;
	return FR_OK;
}

int fr_in_pool(const struct fr_pool *p, unsigned int pa)
{
	return pa < FR_LIMIT && BIT(p->member, pa / FR_PAGE) != 0;
}

int fr_is_used(const struct fr_pool *p, unsigned int pa)
{
	return pa < FR_LIMIT && BIT(p->used, pa / FR_PAGE) != 0;
}

unsigned int fr_free_count(const struct fr_pool *p)
{
	return p->pool - p->nused;
}

int fr_check(const struct fr_pool *p)
{
	unsigned int i, m = 0, u = 0;

	for (i = 0; i < FR_NFRAMES; i++) {
		if (BIT(p->member, i))
			m++;
		if (BIT(p->used, i)) {
			u++;
			if (!BIT(p->member, i))
				return 0;
		}
	}
	return m == p->pool && u == p->nused && p->nused <= p->pool;
}
