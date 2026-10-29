/*
 * QDOS/386 R12B - per-process environment.  See env.h.  Strict C89.
 */
#include "env.h"

static int up(int c)
{
	return (c >= 'a' && c <= 'z') ? c - 'a' + 'A' : c;
}

int env_name_ok(const char *name, unsigned int nlen)
{
	unsigned int i;
	int c;

	if (nlen == 0 || nlen > ENV_NAME_MAX)
		return ENV_ERR_NAME;
	for (i = 0; i < nlen; i++) {
		c = (unsigned char) name[i];
		if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_' || (i > 0 && c >= '0' && c <= '9')))
			return ENV_ERR_NAME;
	}
	return ENV_OK;
}

/* Compare the entry name at e (up to '=') with name: <0, 0, >0 (names are stored upper case). */
static int cmp_name(const char *e, const char *name, unsigned int nlen)
{
	unsigned int i;
	int a, b;

	for (i = 0; i < nlen; i++) {
		a = (unsigned char) e[i];
		b = up((unsigned char) name[i]);
		if (a == '=')
			return -1;             /* entry name is shorter */
		if (a != b)
			return a < b ? -1 : 1;
	}
	return e[nlen] == '=' ? 0 : 1;
}

static unsigned int entry_len(const char *e, unsigned int room)
{
	unsigned int n = 0;

	while (n < room && e[n] != 0)
		n++;
	return n;
}

int env_find(const char *blk, unsigned int used, const char *name, unsigned int nlen, unsigned int *voff,
             unsigned int *vlen)
{
	unsigned int off = 0, len;

	while (off < used) {
		len = entry_len(blk + off, used - off);
		if (cmp_name(blk + off, name, nlen) == 0) {
			*voff = off + nlen + 1;
			*vlen = len - nlen - 1;
			return ENV_OK;
		}
		off += len + 1;
	}
	return ENV_ERR_NOTFOUND;
}

unsigned int env_count(const char *blk, unsigned int used)
{
	unsigned int off = 0, n = 0;

	while (off < used) {
		off += entry_len(blk + off, used - off) + 1;
		n++;
	}
	return n;
}

int env_entry(const char *blk, unsigned int used, unsigned int i, unsigned int *off, unsigned int *len)
{
	unsigned int o = 0, k = 0, l;

	while (o < used) {
		l = entry_len(blk + o, used - o);
		if (k == i) {
			*off = o;
			*len = l;
			return ENV_OK;
		}
		o += l + 1;
		k++;
	}
	return ENV_ERR_NOTFOUND;
}

int env_edit(const char *src, unsigned int used, char *dst, unsigned int *dused, const char *name,
             unsigned int nlen, const char *val, unsigned int vlen, int unset)
{
	unsigned int off = 0, d = 0, len, i, n = 0;
	int placed = 0, c, found = 0;

	if (env_name_ok(name, nlen) != ENV_OK)
		return ENV_ERR_NAME;
	if (!unset) {
		if (vlen > ENV_VALUE_MAX)
			return ENV_ERR_RANGE;
		for (i = 0; i < vlen; i++)
			if (val[i] == 0)
				return ENV_ERR_RANGE;
	}
	while (off < used || !placed) {
		c = off < used ? cmp_name(src + off, name, nlen) : 1;
		if (!placed && c >= 0) {
			placed = 1;
			if (c == 0)
				found = 1;
			if (!unset) {                          /* the new entry goes here, in name order */
				if (d + nlen + 1 + vlen + 1 > ENV_BYTES_MAX || n + 1 > ENV_MAX_VARS)
					return ENV_ERR_FULL;
				for (i = 0; i < nlen; i++)
					dst[d++] = (char) up((unsigned char) name[i]);
				dst[d++] = '=';
				for (i = 0; i < vlen; i++)
					dst[d++] = val[i];
				dst[d++] = 0;
				n++;
			}
			if (c == 0) {                             /* skip the old entry */
				off += entry_len(src + off, used - off) + 1;
				continue;
			}
		}
		if (off >= used)
			break;
		len = entry_len(src + off, used - off);
		if (d + len + 1 > ENV_BYTES_MAX || n + 1 > ENV_MAX_VARS)
			return ENV_ERR_FULL;
		for (i = 0; i <= len; i++)
			dst[d++] = src[off + i];
		off += len + 1;
		n++;
	}
	*dused = d;
	return (unset && !found) ? ENV_ERR_NOTFOUND : ENV_OK;
}

#ifndef ENV_HOST
/* ---- kernel glue -------------------------------------------------------------------------------------------- */
#include "jobs.h"
#include "memory.h"
#include "channel.h"

static char *env_blk[JOB_MAX];          /* kernel-heap block owned by env_own[slot], or 0 */
static unsigned int env_used[JOB_MAX];
static unsigned int env_own[JOB_MAX];
static unsigned int env_cap[JOB_MAX];   /* size of the block: entries rounded up to ENV_GRAIN */
static char env_scratch[ENV_BYTES_MAX];  /* traps do not nest */
unsigned long env_sets, env_copies, env_refusals;
static unsigned int env_fail_next;       /* diagnostic: refuse this many coming copies (QT_ENV_LIST index 0xFFFFFFFE) */

/* Blocks are allocated in steps of ENV_GRAIN bytes, so that most SET/UNSET pairs edit the block in place and leave the
 * kernel heap exactly as it was (no allocation churn, no fragmentation from a replace-then-free). */
#define ENV_GRAIN 256u

static unsigned int grain(unsigned int n)
{
	return (n + ENV_GRAIN - 1u) / ENV_GRAIN * ENV_GRAIN;
}

static int cur_slot(void)
{
	return job_current;
}

/* The live block of a slot (0 if none or stale). */
static char *blk_of(int slot, unsigned int *used)
{
	if (slot < 0 || slot >= JOB_MAX || env_blk[slot] == 0 || env_own[slot] != jobs[slot].id) {
		*used = 0;
		return 0;
	}
	*used = env_used[slot];
	return env_blk[slot];
}

/* Copy a NUL-terminated user string of at most max characters (plus the NUL) into out.  -14 range, -15 too long. */
static int copy_in(unsigned int addr, char *out, unsigned int max, unsigned int *len)
{
	unsigned int i;

	for (i = 0; i <= max; i++) {
		if (!chan_user_range_ok(addr + i, 1))
			return CHAN_ERR_RANGE;
		out[i] = *(const char *) (addr + i);
		if (out[i] == 0) {
			*len = i;
			return ENV_OK;
		}
	}
	return ENV_ERR_NAME;     /* not terminated within the limit */
}

static int copy_out(unsigned int buf, unsigned int size, const char *src, unsigned int n)
{
	unsigned int i, k;
	char *d;

	if (size == 0 || !chan_user_range_ok(buf, size))
		return CHAN_ERR_RANGE;
	k = n < size - 1 ? n : size - 1;
	d = (char *) buf;
	for (i = 0; i < k; i++)
		d[i] = src[i];
	d[k] = 0;
	return (int) n;          /* the full length; the copy is truncated to size - 1 */
}

/* Replace slot's entries with n bytes of env_scratch (n == 0: no block).  In place when the size step is unchanged;
 * otherwise a new block, the old one freed afterwards.  The old block stays on failure. */
static int install(int slot, unsigned int n)
{
	char *nb = 0;
	unsigned int i, owner = jobs[slot].id;
	int live = env_blk[slot] != 0 && env_own[slot] == owner;

	if (n != 0 && live && grain(n) == env_cap[slot]) {
		for (i = 0; i < n; i++)
			env_blk[slot][i] = env_scratch[i];
		env_used[slot] = n;
		return ENV_OK;
	}
	if (n != 0) {
		nb = (char *) mem_alloc(grain(n), owner);
		if (mem_is_error((unsigned int) nb))
			return ENV_ERR_NOMEM;
		for (i = 0; i < n; i++)
			nb[i] = env_scratch[i];
	}
	if (live)
		(void) mem_free(env_blk[slot], owner);
	env_blk[slot] = nb;
	env_used[slot] = n;
	env_cap[slot] = n != 0 ? grain(n) : 0u;
	env_own[slot] = owner;
	return ENV_OK;
}

int env_get_trap(unsigned int name_addr, unsigned int buf_addr, unsigned int size)
{
	char name[ENV_NAME_MAX + 1];
	unsigned int nlen, used, voff, vlen;
	const char *b;
	int rc;

	rc = copy_in(name_addr, name, ENV_NAME_MAX, &nlen);
	if (rc != ENV_OK)
		return rc;
	if (env_name_ok(name, nlen) != ENV_OK)
		return ENV_ERR_NAME;
	b = blk_of(cur_slot(), &used);
	if (b == 0 || env_find(b, used, name, nlen, &voff, &vlen) != ENV_OK)
		return ENV_ERR_NOTFOUND;
	return copy_out(buf_addr, size, b + voff, vlen);
}

int env_set_trap(unsigned int name_addr, unsigned int val_addr)
{
	static char val[ENV_VALUE_MAX + 2];
	char name[ENV_NAME_MAX + 1];
	unsigned int nlen, vlen = 0, used, n;
	const char *b;
	int rc, slot = cur_slot();

	rc = copy_in(name_addr, name, ENV_NAME_MAX, &nlen);
	if (rc != ENV_OK)
		return rc;
	if (val_addr != 0) {
		rc = copy_in(val_addr, val, ENV_VALUE_MAX, &vlen);
		if (rc == ENV_ERR_NAME)
			rc = ENV_ERR_RANGE;                   /* value longer than the limit */
		if (rc != ENV_OK) {
			env_refusals++;
			return rc;
		}
	}
	b = blk_of(slot, &used);
	rc = env_edit(b ? b : "", used, env_scratch, &n, name, nlen, val, vlen, val_addr == 0);
	if (rc != ENV_OK) {
		env_refusals++;
		return rc;
	}
	rc = install(slot, n);
	if (rc == ENV_OK)
		env_sets++;
	return rc;
}

int env_list_trap(unsigned int index, unsigned int buf_addr, unsigned int size)
{
	unsigned int used, off, len;
	const char *b = blk_of(cur_slot(), &used);

	if (index == 0xFFFFFFFFu)                          /* summary: (count << 16) | bytes */
		return (int) ((b ? env_count(b, used) : 0u) << 16 | used);
	if (index == 0xFFFFFFFEu) {                        /* diagnostic: the next buf_addr copies fail (spawn rollback test) */
		env_fail_next = buf_addr <= 16 ? buf_addr : 16;
		return (int) env_copies;
	}
	if (b == 0 || env_entry(b, used, index, &off, &len) != ENV_OK)
		return ENV_ERR_NOTFOUND;
	return copy_out(buf_addr, size, b + off, len);
}

int env_inherit(int child_slot, unsigned int child_id, int parent_slot)
{
	unsigned int used, i;
	const char *b = blk_of(parent_slot, &used);
	char *nb;

	env_blk[child_slot] = 0;
	env_used[child_slot] = 0;
	env_cap[child_slot] = 0;
	env_own[child_slot] = child_id;
	if (env_fail_next != 0) {
		env_fail_next--;
		env_refusals++;
		return ENV_ERR_NOMEM;
	}
	if (b == 0 || used == 0)
		return ENV_OK;
	nb = (char *) mem_alloc(grain(used), child_id);   /* owned by the child: job_cancel/reclaim free it */
	if (mem_is_error((unsigned int) nb))
		return ENV_ERR_NOMEM;
	for (i = 0; i < used; i++)
		nb[i] = b[i];
	env_blk[child_slot] = nb;
	env_used[child_slot] = used;
	env_cap[child_slot] = grain(used);
	env_copies++;
	return ENV_OK;
}

void env_job_gone(int slot)
{
	if (slot < 0 || slot >= JOB_MAX)
		return;
	env_blk[slot] = 0;                                 /* the block itself is freed with the job's heap blocks */
	env_used[slot] = 0;
	env_cap[slot] = 0;
	env_own[slot] = 0;
}

const char *env_value_of(int slot, const char *name, unsigned int *vlen)
{
	unsigned int used, voff, nlen = 0;
	const char *b = blk_of(slot, &used);

	while (name[nlen])
		nlen++;
	if (b == 0 || env_find(b, used, name, nlen, &voff, vlen) != ENV_OK)
		return 0;
	return b + voff;
}
#endif
