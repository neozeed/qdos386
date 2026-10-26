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

extern unsigned int irq_save(void);
extern void irq_restore(unsigned int flags);

static int slot_owner[APP_SLOTS];        /* job id holding each slot, or 0 */
static struct app_xfer_entry xfer_tab[XFER_MAX_IMAGES];
static int xfer_count = 0;

static unsigned int rd32(const unsigned char *p)
{
	return (unsigned int) p[0] | ((unsigned int) p[1] << 8) |
	       ((unsigned int) p[2] << 16) | ((unsigned int) p[3] << 24);
}

static void wr_bytes(unsigned char *dst, const unsigned char *src, unsigned int n)
{
	while (n-- != 0)
		*dst++ = *src++;
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

/* Does the symbol table hold "_start" with value entry?  Every name is read
 * only inside the string table. */
static int start_symbol_ok(const unsigned char *img, unsigned int len,
                           unsigned int symoff, unsigned int nsym,
                           unsigned int stroff, unsigned int strsz,
                           unsigned int entry)
{
	static const char want[] = "_start";
	unsigned int i, k;

	for (i = 0; i < nsym; i++) {
		const unsigned char *s = img + symoff + i * AOUT_SYM_BYTES;
		unsigned int strx = rd32(s), value = rd32(s + 8);
		int match = 1;
		if (strx < 4 || strx >= strsz)
			continue;
		for (k = 0; want[k] != 0; k++) {
			if (strx + k >= strsz || img[stroff + strx + k] != (unsigned char) want[k]) {
				match = 0;
				break;
			}
		}
		if (match && strx + k < strsz && img[stroff + strx + k] == 0)
			return value == entry;
	}
	(void) len;
	return 0;
}

int aout_check(const unsigned char *img, unsigned int len, struct app_image *out)
{
	unsigned int magic, text, data, bss, syms, entry, trs, drs;
	unsigned int slot, slot_size, off_sym, off_str, strsz;

	if (len < AOUT_HDR_BYTES)
		return APP_ERR_SIZE;
	magic = rd32(img);
	text  = rd32(img + 4);
	data  = rd32(img + 8);
	bss   = rd32(img + 12);
	syms  = rd32(img + 16);
	entry = rd32(img + 20);
	trs   = rd32(img + 24);
	drs   = rd32(img + 28);

	if (magic != AOUT_MAGIC)
		return APP_ERR_MAGIC;
	if (trs != 0 || drs != 0)
		return APP_ERR_RELOC;

	if (entry == APP_SLOT0_BASE)
		slot = 0;
	else if (entry == APP_SLOT1_BASE)
		slot = 1;
	else
		return APP_ERR_ENTRY;

	if (text == 0 || (text & 15u) || (data & 15u) || (bss & 15u))
		return APP_ERR_LAYOUT;
	slot_size = APP_SLOT_SIZE;
	if (text > slot_size || data > slot_size - text || bss > slot_size - text - data)
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
	strsz = rd32(img + off_str);
	if (strsz < 4)
		return APP_ERR_FORMAT;
	if (strsz > len - off_str)
		return APP_ERR_SIZE;
	if (strsz != len - off_str)           /* trailing bytes or truncation */
		return APP_ERR_SIZE;

	if (!start_symbol_ok(img, len, off_sym, syms / AOUT_SYM_BYTES,
	                     off_str, strsz, entry))
		return APP_ERR_ENTRY;

	out->img = img;
	out->len = len;
	out->slot = slot;
	out->text = text;
	out->data = data;
	out->bss = bss;
	out->entry = entry;
	return APP_OK;
}

int app_slot_owner(int slot)
{
	if (slot < 0 || slot >= APP_SLOTS)
		return 0;
	return slot_owner[slot];
}

void app_release_owner(unsigned int job_id)
{
	int i;
	for (i = 0; i < APP_SLOTS; i++)
		if (slot_owner[i] != 0 && (unsigned int) slot_owner[i] == job_id)
			slot_owner[i] = 0;
}

static void app_not_started(void)
{
	/* Placeholder entry: the job is created inactive (prio 0) and gets its
	 * real entry before it becomes runnable.  Never called. */
}

int app_load(int index)
{
	static const char *names[APP_SLOTS] = { "APP0", "APP1" };
	const unsigned char *img;
	unsigned int ilen, flags, base, i;
	struct app_image info;
	int rc, id, js;
	unsigned char *dst;

	rc = app_image_bytes(index, &img, &ilen);
	if (rc != APP_OK)
		return rc;
	rc = aout_check(img, ilen, &info);
	if (rc != APP_OK)
		return rc;
	if (slot_owner[info.slot] != 0)
		return APP_ERR_SLOTBUSY;

	/* Interrupts off from job creation to the copy, so the timer cannot run
	 * the new job before its text and data are in place. */
	flags = irq_save();
	id = job_alloc(names[info.slot], app_not_started, 0);
	if (id < 0) {
		irq_restore(flags);
		return (id == -1) ? APP_ERR_NOJOB : APP_ERR_NOMEM;
	}
	js = job_lookup((unsigned int) id);
	if (js == JOB_NONE) {
		irq_restore(flags);
		return APP_ERR_NOJOB;
	}

	base = APP_SLOT0_BASE + info.slot * APP_SLOT_SIZE;
	dst = (unsigned char *) base;
	wr_bytes(dst, img + AOUT_HDR_BYTES, info.text);
	wr_bytes(dst + info.text, img + AOUT_HDR_BYTES + info.text, info.data);
	for (i = 0; i < info.bss; i++)
		dst[info.text + info.data + i] = 0;

	slot_owner[info.slot] = id;
	jobs[js].entry = (void (*)(void)) info.entry;
	jobs[js].prio_inc = 2;
	irq_restore(flags);
	return id;
}
