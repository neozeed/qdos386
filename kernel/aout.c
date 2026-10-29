/*
 * QDOS/386 R5 - guest-side OMAGIC a.out loader.  See aout.h.
 *
 * Every check reads the image through the unsigned byte pointer and compares
 * sizes without overflow.  The file is accepted only when its length equals
 * the end of its string table, so the directory length is the file length
 * and padding is never counted.
 */
#include "aout.h"
#include "jobs.h"
#include "memory.h"
#include "region.h"

extern unsigned int irq_save(void);
extern void irq_restore(unsigned int flags);

static int slot_owner[APP_PROFILES];    /* job id holding each profile, or 0 */
static long profile_handle[APP_PROFILES];  /* R11H: region of each owned profile */
static struct region_pool app_pool;     /* R11H: application regions */
static int app_pool_ready = 0;

#if defined(QDOS_I386VM_BLOCK) && defined(QDOS_TEST_SPARE)
/* TEST ONLY (R11I).  Never compiled into a production image.  Bounded guest proof that the
 * former RAM-disk span is usable RAM held by the allocator: on the first pool build, allocate
 * the whole spare span through region_alloc, write and verify a pattern on every page of it,
 * check that the neighbouring transfer area and 0x801000..0x8FFFFF are untouched and the block
 * device still serves sector 0, then release the span.  Prints SPARE: PASS or SPARE: FAIL. */
#include "platform.h"
static unsigned long spare_sum(unsigned long lo, unsigned long hi)
{
	unsigned long s = 0, a;

	for (a = lo; a < hi; a += 4)
		s = (s << 1 | s >> 31) ^ *(volatile unsigned long *) a;
	return s;
}
static void spare_fail(const char *what, int *bad)
{
	con_puts("SPARE: FAIL ");
	con_puts(what);
	con_puts("\n");
	(*bad)++;
}
static void app_spare_selftest(void)
{
	unsigned long base = 0, end = 0, pb = 0, pe = 0, a, bytes, x0, x1, g0, g1, pages = 0;
	unsigned char sec[512];
	long h, hp;
	int bad = 0;

	x0 = spare_sum(XFER_BASE, XFER_BASE + XFER_SIZE);
	g0 = spare_sum(DEVICE_BASE + DEVICE_SIZE, SPARE_BASE);
	h = region_alloc(&app_pool, SPARE_END - SPARE_BASE, 0x7FFEu);
	if (h <= 0) {
		spare_fail("allocation", &bad);
		return;
	}
	if (region_range(&app_pool, h, &base, &end) != REGION_OK || base != SPARE_BASE ||
	    end != SPARE_END)
		spare_fail("range", &bad);
	if (!region_check(&app_pool))
		spare_fail("invariants while held", &bad);
	/* With the spare held, a further page must land outside it (in a profile span). */
	hp = region_alloc(&app_pool, 0x1000ul, 0x7FFDu);
	if (hp <= 0 || region_range(&app_pool, hp, &pb, &pe) != REGION_OK ||
	    (pb < SPARE_END && SPARE_BASE < pe))
		spare_fail("probe page", &bad);
	if (hp > 0 && region_release(&app_pool, hp, 0x7FFDu) != REGION_OK)
		spare_fail("probe release", &bad);
	for (a = base; a < end; a += 0x1000ul) {
		*(volatile unsigned long *) a = a ^ 0xA5A5A5A5ul;
		*(volatile unsigned long *) (a + 0xFFCul) = ~a;
	}
	for (a = base; a < end; a += 0x1000ul) {
		if (*(volatile unsigned long *) a != (a ^ 0xA5A5A5A5ul) ||
		    *(volatile unsigned long *) (a + 0xFFCul) != ~a) {
			spare_fail("pattern", &bad);
			break;
		}
		pages++;
	}
	x1 = spare_sum(XFER_BASE, XFER_BASE + XFER_SIZE);
	g1 = spare_sum(DEVICE_BASE + DEVICE_SIZE, SPARE_BASE);
	if (x0 != x1)
		spare_fail("transfer area changed", &bad);
	if (g0 != g1)
		spare_fail("0x801000..0x8FFFFF changed", &bad);
	if (disk_read_sector(0u, sec) != 0 || sec[510] != 0x55 || sec[511] != 0xAA)
		spare_fail("block device after the pattern", &bad);
	/* Device boundaries seen by the guest backend: last valid sector reads, the first invalid
	 * one is refused with a zeroed buffer, and three reads of one sector agree. */
	{
		unsigned long ns = disk_sector_count(), ck0 = 0, ck;
		unsigned int r, k;

		if (ns == 0ul || disk_read_sector((unsigned int) (ns - 1ul), sec) != 0)
			spare_fail("last valid sector", &bad);
		if (disk_read_sector((unsigned int) ns, sec) >= 0)
			spare_fail("first invalid sector accepted", &bad);
		for (r = 0; r < 3u; r++) {
			if (disk_read_sector(1u, sec) != 0)
				spare_fail("repeated read", &bad);
			ck = 0;
			for (k = 0; k < 512u; k++)
				ck = ck * 31ul + sec[k];
			if (r == 0)
				ck0 = ck;
			else if (ck != ck0)
				spare_fail("repeated read differs", &bad);
		}
		con_puts("BLKT: sectors ");
		con_putdec(ns);
		con_puts(" (last, first invalid, repeat x3 checked)\n");
	}
	if (region_release(&app_pool, h, 0x7FFFu) != REGION_ERR_OWNER)
		spare_fail("foreign release accepted", &bad);
	if (region_release(&app_pool, h, 0x7FFEu) != REGION_OK)
		spare_fail("release", &bad);
	if (region_release(&app_pool, h, 0x7FFEu) != REGION_ERR_HANDLE)
		spare_fail("stale handle accepted", &bad);
	if (!region_check(&app_pool) || region_live(&app_pool, &bytes) != 0 || bytes != 0)
		spare_fail("pool not empty after release", &bad);
	con_puts("SPARE: base ");
	con_putdec(base);
	con_puts(" end ");
	con_putdec(end);
	con_puts(" pages ");
	con_putdec(pages);
	con_puts(bad ? "\nSPARE: FAIL\n" : "\nSPARE: PASS\n");
}
#endif

/* R11H: the application-region pool, built once from the profiles.  FREE spans
 * are exactly the profile ranges, so only a profile origin can be reserved.
 * Everything else (kernel, heap, transfer area, stacks, RAM disk) stays outside
 * the pool and cannot be claimed. */
static int app_pool_init(void)
{
#ifdef QDOS_I386VM_BLOCK
	struct region_span sp[APP_PROFILES + 1];
#else
	struct region_span sp[APP_PROFILES];
#endif
	int n = 0, i, rc;

	if (app_pool_ready)
		return APP_OK;
	/* Slots 0 and 1 are one contiguous FREE span; profiles 2 and 3 (R9 only)
	 * are separate spans, and only when their size is not zero. */
	sp[n].base = APP_SLOT0_BASE;
	sp[n].end = APP_SLOT1_BASE + APP_SLOT1_SIZE;
	sp[n].kind = REGION_SPAN_FREE;
	n++;
	for (i = 2; i < APP_PROFILES; i++) {
		if (APP_PROFILE_SIZE(i) == 0)
			continue;
		sp[n].base = APP_PROFILE_BASE(i);
		sp[n].end = APP_PROFILE_BASE(i) + APP_PROFILE_SIZE(i);
		sp[n].kind = REGION_SPAN_FREE;
		n++;
	}
#ifdef QDOS_I386VM_BLOCK
	/* R11I: the former RAM-disk span is spare RAM, a FREE span with no profile origin in it. */
	sp[n].base = SPARE_BASE;
	sp[n].end = SPARE_END;
	sp[n].kind = REGION_SPAN_FREE;
	n++;
#endif
	rc = region_init(&app_pool, sp, n);
	if (rc != REGION_OK)
		return APP_ERR_LAYOUT;
	app_pool_ready = 1;
#if defined(QDOS_I386VM_BLOCK) && defined(QDOS_TEST_SPARE)
	app_spare_selftest();
#endif
	return APP_OK;
}

/* The profile whose base is entry, or -1. */
static int profile_of(unsigned int entry)
{
	int n;

	for (n = 0; n < APP_PROFILES; n++)
		if (entry == APP_PROFILE_BASE(n))
			return n;
	return -1;
}
static struct app_xfer_entry xfer_tab[XFER_MAX_IMAGES];
static int xfer_count = 0;

static void wr32(unsigned char *p, unsigned int v)
{
	p[0] = (unsigned char) v;
	p[1] = (unsigned char) (v >> 8);
	p[2] = (unsigned char) (v >> 16);
	p[3] = (unsigned char) (v >> 24);
}

static unsigned int rd32(const unsigned char *p)
{
	return (unsigned int) p[0] | ((unsigned int) p[1] << 8) |
	       ((unsigned int) p[2] << 16) | ((unsigned int) p[3] << 24);
}

int xfer_parse(const unsigned char *base, unsigned int size,
               struct app_xfer_entry *entries, unsigned int max)
{
	unsigned int count, i, prev_end = XFER_DIR_BYTES;

	if (size < XFER_DIR_BYTES)
		return APP_ERR_XFER;
	if (rd32(base) != XFER_MAGIC || rd32(base + 4) != XFER_VERSION)
		return APP_ERR_XFER;
	count = rd32(base + 8);
	if (count == 0 || count > XFER_MAX_IMAGES || count > max)
		return APP_ERR_XFER;
	for (i = 0; i < count; i++) {
		const unsigned char *e = base + 16 + 16 * i;
		unsigned int off = rd32(e), len = rd32(e + 4);
		if (off < prev_end || (off & 15u) != 0)
			return APP_ERR_XFER;
		if (len == 0 || len > size || off > size - len)
			return APP_ERR_XFER;
		entries[i].offset = off;
		entries[i].length = len;
		entries[i].name[0] = (char) e[8];
		entries[i].name[1] = (char) e[9];
		entries[i].name[2] = (char) e[10];
		entries[i].name[3] = (char) e[11];
		entries[i].name[4] = 0;
		prev_end = (off + len + 15u) & ~15u;
	}
	return (int) count;
}

int app_xfer_init(void)
{
	xfer_count = xfer_parse((const unsigned char *) XFER_BASE, XFER_SIZE,
	                        xfer_tab, XFER_MAX_IMAGES);
	return xfer_count;
}

int app_image_count(void)
{
	return xfer_count;
}

int app_image_bytes(int index, const unsigned char **img, unsigned int *len)
{
	if (xfer_count <= 0)
		return APP_ERR_XFER;
	if (index < 0 || index >= xfer_count)
		return APP_ERR_NOIMAGE;
	*img = (const unsigned char *) (XFER_BASE + xfer_tab[index].offset);
	*len = xfer_tab[index].length;
	return APP_OK;
}

/* ---- validation and loading, independent of the byte source (R8) ------- */

struct mem_src {
	const unsigned char *img;
	unsigned int len;
};

static int mem_rd(void *ctx, unsigned int off, unsigned char *dst, unsigned int n)
{
	struct mem_src *m = (struct mem_src *) ctx;
	unsigned int i;

	if (off > m->len || n > m->len - off)
		return APP_ERR_SIZE;
	for (i = 0; i < n; i++)
		dst[i] = m->img[off + i];
	return APP_OK;
}

static int rd_u32(app_rd_fn rd, void *ctx, unsigned int off, unsigned int *v)
{
	unsigned char b[4];
	int rc = rd(ctx, off, b, 4);

	if (rc != APP_OK)
		return rc;
	*v = (unsigned int) b[0] | ((unsigned int) b[1] << 8) |
	     ((unsigned int) b[2] << 16) | ((unsigned int) b[3] << 24);
	return APP_OK;
}

/* Does the symbol table hold "_start" with value entry?  R8: the table is read
 * forward, in chunks of START_CHUNK symbols, so a file-backed source never seeks
 * backwards during the scan (a backward seek restarts the FAT chain walk).  A
 * name is read from the string table only when the symbol's value equals the
 * entry point, and at most START_CANDIDATES such names are examined.  Result:
 * APP_OK if "_start" with value entry exists, APP_ERR_ENTRY if not,
 * APP_ERR_FORMAT if the candidate bound is exceeded. */
#define START_CHUNK       64u
#define START_CANDIDATES 64u

static unsigned char start_buf[AOUT_SYM_BYTES * START_CHUNK];

static int start_symbol_src(app_rd_fn rd, void *ctx, unsigned int symoff,
                            unsigned int nsym, unsigned int stroff,
                            unsigned int strsz, unsigned int entry, int *found)
{
	static const char want[] = "_start";
	unsigned char name[8];
	unsigned int i, k, n, c, strx, value, cand = 0;
	int rc;

	*found = 0;
	for (i = 0; i < nsym; i += START_CHUNK) {
		n = nsym - i;
		if (n > START_CHUNK)
			n = START_CHUNK;
		rc = rd(ctx, symoff + i * AOUT_SYM_BYTES, start_buf, n * AOUT_SYM_BYTES);
		if (rc != APP_OK)
			return rc;
		for (c = 0; c < n; c++) {
			unsigned char *sym = start_buf + c * AOUT_SYM_BYTES;

			strx = (unsigned int) sym[0] | ((unsigned int) sym[1] << 8) |
			       ((unsigned int) sym[2] << 16) | ((unsigned int) sym[3] << 24);
			value = (unsigned int) sym[8] | ((unsigned int) sym[9] << 8) |
			        ((unsigned int) sym[10] << 16) | ((unsigned int) sym[11] << 24);
			if (value != entry)
				continue;
			if (strx < 4 || strx >= strsz || strsz - strx < 7)
				continue;
			if (++cand > START_CANDIDATES)
				return APP_ERR_FORMAT;
			rc = rd(ctx, stroff + strx, name, 7);
			if (rc != APP_OK)
				return rc;
			for (k = 0; k < 6; k++)
				if (name[k] != (unsigned char) want[k])
					break;
			if (k == 6 && name[6] == 0) {
				*found = 1;
				return APP_OK;
			}
		}
	}
	return APP_OK;
}

/* The pure check, on any source.  Reads the header, the layout and the
 * string-table size, and the _start symbol; changes nothing. */
int aout_check_src(app_rd_fn rd, void *ctx, unsigned int len, struct app_image *out)
{
	unsigned char hb[AOUT_HDR_BYTES];
	unsigned int magic, text, data, bss, syms, entry, trs, drs;
	unsigned int slot, off_sym, off_str, strsz, lim;
	int found;
	int rc;

	if (len < AOUT_HDR_BYTES)
		return APP_ERR_SIZE;
	rc = rd(ctx, 0, hb, AOUT_HDR_BYTES);
	if (rc != APP_OK)
		return rc;
	magic = rd32(hb);
	text  = rd32(hb + 4);
	data  = rd32(hb + 8);
	bss   = rd32(hb + 12);
	syms  = rd32(hb + 16);
	entry = rd32(hb + 20);
	trs   = rd32(hb + 24);
	drs   = rd32(hb + 28);

	if (magic != AOUT_MAGIC)
		return APP_ERR_MAGIC;
	if (trs != 0 || drs != 0)
		return APP_ERR_RELOC;

	{
		int prof = profile_of(entry);
		if (prof < 0)
			return APP_ERR_ENTRY;
		if (APP_PROFILE_SIZE(prof) == 0)
			return APP_ERR_NOPROFILE;   /* a profile this memory map does not have */
		slot = (unsigned int) prof;
	}

	if (text == 0 || (text & 15u) || (data & 15u) || (bss & 15u))
		return APP_ERR_LAYOUT;
	lim = APP_IMAGE_LIMIT_OF(slot);
	if (text > lim || data > lim - text || bss > lim - text - data)
		return APP_ERR_LAYOUT;

	if (syms % AOUT_SYM_BYTES != 0)
		return APP_ERR_FORMAT;

	/* File layout without overflow: header, text, data, symbols, strings. */
	if (text > len - AOUT_HDR_BYTES)
		return APP_ERR_SIZE;
	if (data > len - AOUT_HDR_BYTES - text)
		return APP_ERR_SIZE;
	if (syms > len - AOUT_HDR_BYTES - text - data)
		return APP_ERR_SIZE;
	off_sym = AOUT_HDR_BYTES + text + data;
	off_str = off_sym + syms;
	if (len - off_str < 4)
		return APP_ERR_SIZE;
	rc = rd_u32(rd, ctx, off_str, &strsz);
	if (rc != APP_OK)
		return rc;
	if (strsz < 4)
		return APP_ERR_FORMAT;
	if (strsz > len - off_str)
		return APP_ERR_SIZE;
	if (strsz != len - off_str)           /* trailing bytes or truncation */
		return APP_ERR_SIZE;

	rc = start_symbol_src(rd, ctx, off_sym, syms / AOUT_SYM_BYTES,
	                      off_str, strsz, entry, &found);
	if (rc != APP_OK)
		return rc;
	if (!found)
		return APP_ERR_ENTRY;

	out->img = 0;
	out->len = len;
	out->slot = slot;
	out->text = text;
	out->data = data;
	out->bss = bss;
	out->entry = entry;
	return APP_OK;
}

int aout_check(const unsigned char *img, unsigned int len, struct app_image *out)
{
	struct mem_src m;
	int rc;

	m.img = img;
	m.len = len;
	rc = aout_check_src(mem_rd, &m, len, out);
	if (rc == APP_OK)
		out->img = img;
	return rc;
}

int app_slot_owner(int slot)
{
	if (slot < 0 || slot >= APP_PROFILES)
		return 0;
	return slot_owner[slot];
}

void app_release_owner(unsigned int job_id)
{
	int i;
	for (i = 0; i < APP_PROFILES; i++)
		if (slot_owner[i] != 0 && (unsigned int) slot_owner[i] == job_id) {
			if (profile_handle[i] > 0)
				region_release(&app_pool, profile_handle[i], job_id);
			profile_handle[i] = 0;
			slot_owner[i] = 0;
		}
}

static void app_not_started(void)
{
	/* Placeholder entry: the job is created inactive (prio 0) and gets its
	 * real entry before it becomes runnable.  Never called. */
}

int app_load(int index)
{
	const unsigned char *img;
	unsigned int ilen;
	int rc;

	rc = app_image_bytes(index, &img, &ilen);
	if (rc != APP_OK)
		return rc;
	return app_load_bytes(img, ilen);
}

unsigned int app_args_of(int job_slot)
{
	if (job_slot < 0 || job_slot >= JOB_MAX)
		return 0;
	return jobs[job_slot].args;
}

#ifdef QDOS_TEST_FAULT_LAUNCH
/* TEST ONLY.  Never compiled into a production image.  The QDOS_TEST_FAULT_LAUNCH-th
 * image load fails at its QDOS_TEST_FAULT_READ-th read with FAT_ERR_IO, after the
 * region is reserved and the job created.  Used by sdk/examples/r11h/fault.c to
 * check the rollback path (docs/R11H-FIX1-RUNTIME-REPORT.md). */
#ifndef QDOS_TEST_FAULT_READ
#define QDOS_TEST_FAULT_READ 2
#endif
#define FAULT_RC_IO (-40)   /* FAT_ERR_IO */
static int fault_launches;  /* image loads seen since boot */
static int fault_read_no;   /* read index to fail in the current load, 0 = none */
static int fault_reads;     /* reads seen in the current load */
#endif

/* Copy text, data and zeroed BSS into dst, reading in fixed chunks. */
static int copy_src(app_rd_fn rd, void *ctx, const struct app_image *info,
                    unsigned char *dst)
{
	unsigned char buf[512];
	unsigned int off, rem, n, i;
	int rc;

	off = AOUT_HDR_BYTES;
	rem = info->text;
	while (rem != 0) {
		n = rem < sizeof buf ? rem : (unsigned int) sizeof buf;
#ifdef QDOS_TEST_FAULT_LAUNCH
		if (fault_read_no != 0 && ++fault_reads == fault_read_no)
			return FAULT_RC_IO;
#endif
		rc = rd(ctx, off, buf, n);
		if (rc != APP_OK)
			return rc;
		for (i = 0; i < n; i++)
			dst[i] = buf[i];
		dst += n;
		off += n;
		rem -= n;
	}
	rem = info->data;
	while (rem != 0) {
		n = rem < sizeof buf ? rem : (unsigned int) sizeof buf;
#ifdef QDOS_TEST_FAULT_LAUNCH
		if (fault_read_no != 0 && ++fault_reads == fault_read_no)
			return FAULT_RC_IO;
#endif
		rc = rd(ctx, off, buf, n);
		if (rc != APP_OK)
			return rc;
		for (i = 0; i < n; i++)
			dst[i] = buf[i];
		dst += n;
		off += n;
		rem -= n;
	}
	for (i = 0; i < info->bss; i++)
		dst[i] = 0;
	return APP_OK;
}

/* Argument block at blk: argc, the address of the pointer array, the pointer
 * array (argc entries and a NULL), then the strings.  packed is validated by
 * the caller: plen bytes, argc NUL-terminated strings. */
static void args_write(unsigned char *blk, unsigned int argc,
                       const unsigned char *packed, unsigned int plen)
{
	unsigned int i, p, k;
	unsigned int base = (unsigned int) blk;
	unsigned int ptr;

	for (i = 0; i < APP_ARG_BYTES; i++)
		blk[i] = 0;
	wr32(blk, argc);
	wr32(blk + 4, base + 8);
	k = 0;
	p = 0;
	for (i = 0; i < argc; i++) {
		ptr = base + APP_ARG_STR_OFF + k;
		wr32(blk + 8 + 4 * i, ptr);
		while (packed[p] != 0) {
			blk[APP_ARG_STR_OFF + k++] = packed[p++];
		}
		blk[APP_ARG_STR_OFF + k++] = 0;
		p++;
	}
	wr32(blk + 8 + 4 * argc, 0);
	(void) plen;
}

/* Count the strings in a packed block; -1 if malformed or too large. */
static int args_check(const unsigned char *packed, unsigned int plen)
{
	unsigned int i, argc = 0;

	if (plen == 0)
		return 0;
	if (plen > APP_ARG_STR_MAX || packed[plen - 1] != 0)
		return -1;
	for (i = 0; i < plen; i++)
		if (packed[i] == 0) {
			if (argc == APP_ARG_MAX)
				return -1;
			argc++;
		}
	return (int) argc;
}

int app_load_src(app_rd_fn rd, void *ctx, unsigned int len, int want_slot,
                 const unsigned char *packed, unsigned int plen)
{
	static const char *names[APP_PROFILES] = { "APP0", "APP1", "APP2", "APP3" };
	unsigned int flags, base, argc_n;
	struct app_image info;
	int rc, id, js, argc;
	long h;
	unsigned char *dst;

	/* 1. Validate.  Nothing below changes memory until this has passed. */
	rc = aout_check_src(rd, ctx, len, &info);
	if (rc != APP_OK)
		return rc;
	if (want_slot >= 0 && (unsigned int) want_slot != info.slot)
		return (want_slot == 1) ? APP_ERR_NOTSLOT1 :
		       (want_slot == 0) ? APP_ERR_NOTSLOT0 : APP_ERR_NOPROFILE;
	argc = args_check(packed, plen);
	if (argc < 0)
		return APP_ERR_ARGS;
	argc_n = (unsigned int) argc;

	/* 2. Create the job first, inactive (prio 0), so a failed allocation leaves
	 * the slot untouched.  Then claim the slot and copy.  A failed copy cancels
	 * the job and releases the slot, so nothing is left behind. */
	if (slot_owner[info.slot] != 0)
		return APP_ERR_SLOTBUSY;
	rc = app_pool_init();
	if (rc != APP_OK)
		return rc;
	flags = irq_save();
	base = APP_PROFILE_BASE(info.slot);
	dst = (unsigned char *) base;
	id = job_alloc_sized(names[info.slot], app_not_started, 0, APP_STACK_BYTES);
	if (id < 0) {
		irq_restore(flags);
		return (id == -1) ? APP_ERR_NOJOB : APP_ERR_NOMEM;
	}
	js = job_lookup((unsigned int) id);
	if (js == JOB_NONE) {
		job_cancel((unsigned int) id);
		irq_restore(flags);
		return APP_ERR_NOJOB;
	}
	/* The region is owned by the job, so it is taken after the job exists. */
	h = region_reserve_at(&app_pool, base, APP_PROFILE_SIZE(info.slot), (unsigned int) id);
	if (h < 0) {
		job_cancel((unsigned int) id);
		irq_restore(flags);
		return (h == REGION_ERR_BUSY) ? APP_ERR_ADDROCC : APP_ERR_NOMEM;
	}
	slot_owner[info.slot] = APP_RESERVED;
#ifdef QDOS_TEST_FAULT_LAUNCH
	fault_launches++;
	fault_read_no = (fault_launches == QDOS_TEST_FAULT_LAUNCH) ? QDOS_TEST_FAULT_READ : 0;
	fault_reads = 0;
#endif
	rc = copy_src(rd, ctx, &info, dst);
	if (rc != APP_OK) {
		slot_owner[info.slot] = 0;
		region_release(&app_pool, h, (unsigned int) id);
		job_cancel((unsigned int) id);
		irq_restore(flags);
		return rc;
	}
	args_write(dst + APP_PROFILE_SIZE(info.slot) - APP_ARG_BYTES, argc_n, packed, plen);

	jobs[js].entry = (void (*)(void)) info.entry;
	jobs[js].args = base + APP_PROFILE_SIZE(info.slot) - APP_ARG_BYTES;
	jobs[js].prio_inc = 2;
	profile_handle[info.slot] = h;
	slot_owner[info.slot] = id;
	irq_restore(flags);
	return id;
}

int app_load_bytes(const unsigned char *img, unsigned int ilen)
{
	struct mem_src m;

	m.img = img;
	m.len = ilen;
	return app_load_src(mem_rd, &m, ilen, -1, 0, 0);
}

#if defined(QDOS_PAGING) && !defined(QDOS_DYNPROC)
/* ---- R11J paging proof of concept (docs/R11J-ADDRESS-SPACE-ABI.md) ------------------
 * Frames for a paged process are one physically contiguous run taken from the region pool
 * inside [lo, hi).  Contiguity matters on the i386VM harness: its CPU core sends a
 * multi-byte access that crosses a page boundary above 0x800000 to the harness with the
 * first physical address only, so the two pages must be physically adjacent. */
#include "paging.h"

long app_frames_alloc(unsigned int lo, unsigned int hi, unsigned int bytes,
                      unsigned int owner, unsigned int *base)
{
	unsigned int a;
	long h;

	if (app_pool_init() != APP_OK)
		return REGION_ERR_NOMEM;
	if (bytes == 0 || hi < lo || bytes > hi - lo)
		return REGION_ERR_NOMEM;
	for (a = lo; a <= hi - bytes; a += PG_PAGE) {
		h = region_reserve_at(&app_pool, a, bytes, owner);
		if (h > 0) {
			*base = a;
			return h;
		}
	}
	return REGION_ERR_NOMEM;
}

int app_frames_release(long handle, unsigned int owner)
{
	return region_release(&app_pool, handle, owner);
}

void app_pool_stats(unsigned int *count, unsigned int *bytes)
{
	unsigned long b = 0;

	*count = 0;
	*bytes = 0;
	if (!app_pool_ready)
		return;
	*count = (unsigned int) region_live(&app_pool, &b);
	*bytes = (unsigned int) b;
}

/* args_write with the block written at blk but its pointers built for vbase. */
static void args_write_at(unsigned char *blk, unsigned int vbase, unsigned int argc,
                          const unsigned char *packed)
{
	unsigned int i, p, k;

	for (i = 0; i < APP_ARG_BYTES; i++)
		blk[i] = 0;
	wr32(blk, argc);
	wr32(blk + 4, vbase + 8);
	k = 0;
	p = 0;
	for (i = 0; i < argc; i++) {
		wr32(blk + 8 + 4 * i, vbase + APP_ARG_STR_OFF + k);
		while (packed[p] != 0)
			blk[APP_ARG_STR_OFF + k++] = packed[p++];
		blk[APP_ARG_STR_OFF + k++] = 0;
		p++;
	}
	wr32(blk + 8 + 4 * argc, 0);
}

/*
 * Load an image linked for slot 1 into a new address space.  Order: validate (reads only),
 * then record, frames, job; each failure undoes what came before it.  The window is zeroed,
 * the image copied and the argument block written through the frames' physical addresses:
 * the caller is never a paged job (jobs.c refuses QT_PG_START from one), so the template
 * directory, which maps the pool, is active.  Interrupts are off for the whole load, as in
 * app_load_src.
 */
int app_load_paged(app_rd_fn rd, void *ctx, unsigned int len,
                   const unsigned char *packed, unsigned int plen, int held)
{
	struct app_image info;
	unsigned int flags, frames = 0, argc_n, i, *w;
	int rc, rec, id, js, argc;
	long h;

	if (pg_slot_paged(job_current))
		return PG_ERR_REFUSED;
	rc = aout_check_src(rd, ctx, len, &info);
	if (rc != APP_OK)
		return rc;
	if (info.slot != 1 || info.entry != PG_VBASE)
		return PG_ERR_ORIGIN;
	argc = args_check(packed, plen);
	if (argc < 0)
		return APP_ERR_ARGS;
	argc_n = (unsigned int) argc;
	rc = app_pool_init();
	if (rc != APP_OK)
		return rc;

	flags = irq_save();
	rec = pg_rec_alloc();
	if (rec < 0) {
		irq_restore(flags);
		return PG_ERR_NOREC;
	}
	h = app_frames_alloc(PG_POOL_LO, PG_POOL_HI, PG_VSIZE, PG_OWNER_BASE + (unsigned int) rec,
	                     &frames);
	if (h <= 0) {
		pg_rec_free(rec);
		irq_restore(flags);
		return PG_ERR_NOFRAMES;
	}
	id = job_alloc_sized("APPV", app_not_started, 0, APP_STACK_BYTES);
	if (id < 0) {
		region_release(&app_pool, h, PG_OWNER_BASE + (unsigned int) rec);
		pg_rec_free(rec);
		irq_restore(flags);
		return (id == -1) ? APP_ERR_NOJOB : APP_ERR_NOMEM;
	}
	js = job_lookup((unsigned int) id);
	if (js == JOB_NONE) {
		job_cancel((unsigned int) id);
		region_release(&app_pool, h, PG_OWNER_BASE + (unsigned int) rec);
		pg_rec_free(rec);
		irq_restore(flags);
		return APP_ERR_NOJOB;
	}
	w = (unsigned int *) frames;
	for (i = 0; i < PG_VSIZE / 4u; i++)
		w[i] = 0;
#ifdef QDOS_TEST_FAULT_LAUNCH
	fault_launches++;
	fault_read_no = (fault_launches == QDOS_TEST_FAULT_LAUNCH) ? QDOS_TEST_FAULT_READ : 0;
	fault_reads = 0;
#endif
	rc = copy_src(rd, ctx, &info, (unsigned char *) frames);
	if (rc != APP_OK) {
		job_cancel((unsigned int) id);
		region_release(&app_pool, h, PG_OWNER_BASE + (unsigned int) rec);
		pg_rec_free(rec);
		irq_restore(flags);
		return rc;
	}
	args_write_at((unsigned char *) (frames + PG_VSIZE - APP_ARG_BYTES),
	              PG_VBASE + PG_VSIZE - APP_ARG_BYTES, argc_n, packed);
	pg_rec_build(rec, frames);
	jobs[js].entry = (void (*)(void)) info.entry;
	jobs[js].args = PG_VBASE + PG_VSIZE - APP_ARG_BYTES;
	pg_rec_bind(rec, js, (unsigned int) id, h, frames);
	jobs[js].prio_inc = held ? 0 : 2;
	irq_restore(flags);
	return id;
}
#endif
