/*
 * QDOS/386 R12A - per-process address spaces.  See vas.h (and its RULE on the active directory).
 */
#include "vas.h"

static void zero_page(unsigned int pa)
{
	unsigned int *w = VAS_PTR(pa), i;

	for (i = 0; i < 1024u; i++)
		w[i] = 0;
}

void vas_template_build(unsigned int tpl_dir, const unsigned int *tpl_tab, unsigned int ndirs)
{
	unsigned int *dir = VAS_PTR(tpl_dir), *pt, i, k;

	for (i = 0; i < 1024u; i++)
		dir[i] = 0;
	for (i = 0; i < ndirs; i++) {
		pt = VAS_PTR(tpl_tab[i]);
		for (k = 0; k < 1024u; k++)
			pt[k] = ((i * 1024u + k) * VAS_PAGE) | VAS_P | VAS_RW;   /* supervisor */
		dir[i] = tpl_tab[i] | VAS_P | VAS_RW | VAS_US;
	}
}

static int add_table(struct vas *v, unsigned int idx, unsigned int pa)
{
	if (v->ntab >= VAS_MAX_TABLES)
		return VAS_ERR_TABLES;
	v->tidx[v->ntab] = idx;
	v->tpa[v->ntab] = pa;
	v->ntab++;
	return VAS_OK;
}

static unsigned int table_of(const struct vas *v, unsigned int idx)
{
	int t;

	for (t = 0; t < v->ntab; t++)
		if (v->tidx[t] == idx)
			return v->tpa[t];
	return 0;
}

int vas_create(struct vas *v, struct fr_pool *fp, unsigned int tpl_dir, unsigned int ndirs)
{
	unsigned int *dir, *tdir, *pt, *src, i, k, pa, npool, base;
	int rc;

	v->dir = 0;
	v->ntab = 0;
	v->owned = 0;
	pa = fr_alloc(fp);
	if (pa == 0)
		return VAS_ERR_NOFRAMES;
	v->dir = pa;
	dir = VAS_PTR(pa);
	tdir = VAS_PTR(tpl_dir);
	for (i = 0; i < 1024u; i++)
		dir[i] = tdir[i];
	for (i = 0; i < ndirs; i++) {
		base = i * VAS_SPAN;
		npool = 0;
		for (k = 0; k < 1024u; k++)
			if (fr_in_pool(fp, base + k * VAS_PAGE))
				npool++;
		if (npool == 0)
			continue;                        /* no pool page in this span: share the template table */
		if (npool == 1024u) {
			dir[i] = 0;                      /* the whole span is pool: not present until a private page needs it */
			continue;
		}
		pa = fr_alloc(fp);
		if (pa == 0) {
			vas_destroy(v, fp);
			return VAS_ERR_NOFRAMES;
		}
		rc = add_table(v, i, pa);
		if (rc != VAS_OK) {
			fr_free(fp, pa);
			vas_destroy(v, fp);
			return rc;
		}
		pt = VAS_PTR(pa);
		src = VAS_PTR(tdir[i] & ~(VAS_PAGE - 1u));
		for (k = 0; k < 1024u; k++)
			pt[k] = fr_in_pool(fp, base + k * VAS_PAGE) ? 0u : src[k];
		dir[i] = pa | VAS_P | VAS_RW | VAS_US;
	}
	return VAS_OK;
}

int vas_map_new(struct vas *v, struct fr_pool *fp, unsigned int va, unsigned int flags)
{
	unsigned int idx = va >> 22, *dir, *pt, tpa, pa;
	int rc;

	if ((va & (VAS_PAGE - 1u)) || v->dir == 0)
		return VAS_ERR_ARG;
	dir = VAS_PTR(v->dir);
	tpa = table_of(v, idx);
	if (tpa == 0) {
		if (dir[idx] & VAS_P)
			return VAS_ERR_ARG;              /* a shared template table: never written through a space */
		tpa = fr_alloc(fp);
		if (tpa == 0)
			return VAS_ERR_NOFRAMES;
		rc = add_table(v, idx, tpa);
		if (rc != VAS_OK) {
			fr_free(fp, tpa);
			return rc;
		}
		zero_page(tpa);
		dir[idx] = tpa | VAS_P | VAS_RW | VAS_US;
	}
	pt = VAS_PTR(tpa);
	if (pt[(va >> 12) & 1023u] & VAS_P)
		return VAS_ERR_ARG;
	pa = fr_alloc(fp);
	if (pa == 0)
		return VAS_ERR_NOFRAMES;
	zero_page(pa);
	pt[(va >> 12) & 1023u] = pa | (flags & (VAS_RW | VAS_US)) | VAS_P | VAS_OWN;
	v->owned++;
	return VAS_OK;
}

int vas_hide(struct vas *v, unsigned int va)
{
	unsigned int tpa = table_of(v, va >> 22), *pt;

	if (tpa == 0 || (va & (VAS_PAGE - 1u)))
		return VAS_ERR_ARG;
	pt = VAS_PTR(tpa);
	if ((pt[(va >> 12) & 1023u] & (VAS_P | VAS_OWN)) != VAS_P)
		return VAS_ERR_ARG;                  /* not a shared template page */
	pt[(va >> 12) & 1023u] = 0;
	return VAS_OK;
}

unsigned int vas_lookup(const struct vas *v, unsigned int va)
{
	unsigned int pde;

	if (v->dir == 0)
		return 0;
	pde = VAS_PTR(v->dir)[va >> 22];
	if (!(pde & VAS_P))
		return 0;
	return VAS_PTR(pde & ~(VAS_PAGE - 1u))[(va >> 12) & 1023u];
}

int vas_copy_to(const struct vas *v, unsigned int va, const unsigned char *src, unsigned int n)
{
	unsigned int pte, off, chunk, i;
	unsigned char *dst;

	while (n != 0) {
		pte = vas_lookup(v, va);
		if (!(pte & VAS_P))
			return VAS_ERR_ARG;
		off = va & (VAS_PAGE - 1u);
		chunk = VAS_PAGE - off;
		if (chunk > n)
			chunk = n;
		dst = (unsigned char *) VAS_PTR(pte & ~(VAS_PAGE - 1u)) + off;
		for (i = 0; i < chunk; i++)
			dst[i] = src[i];
		src += chunk;
		va += chunk;
		n -= chunk;
	}
	return VAS_OK;
}

unsigned int vas_destroy(struct vas *v, struct fr_pool *fp)
{
	unsigned int *pt, k, n = 0;
	int t;

	for (t = 0; t < v->ntab; t++) {
		pt = VAS_PTR(v->tpa[t]);
		for (k = 0; k < 1024u; k++)
			if ((pt[k] & (VAS_P | VAS_OWN)) == (VAS_P | VAS_OWN)) {
				if (fr_free(fp, pt[k] & ~(VAS_PAGE - 1u)) == FR_OK)
					n++;
				pt[k] = 0;
			}
		if (fr_free(fp, v->tpa[t]) == FR_OK)
			n++;
	}
	if (v->dir != 0 && fr_free(fp, v->dir) == FR_OK)
		n++;
	v->dir = 0;
	v->ntab = 0;
	v->owned = 0;
	return n;
}

unsigned int vas_frames(const struct vas *v)
{
	return v->owned + (unsigned int) v->ntab + (v->dir != 0 ? 1u : 0u);
}
