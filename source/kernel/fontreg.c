/* QDOS/386 R13A - the font registry and QFNT v1 loader.  See fontreg.h and docs/R13A-FONT-FORMAT.md.  Freestanding
 * C89.  Callers run with interrupts off (traps) except kfont_poll (the SYSTEM job), which masks them itself. */
#include "fontreg.h"

#define KFONT_MAGIC 0x544E4F46u    /* "FONT" */
#define KFONT_OWNER 0x7E01u        /* heap owner of font blocks (never a job id) */
#define ZMAX        KFONT_SLOTS

#ifdef KFONT_HOST_TEST
#include <stdlib.h>
static void *kf_alloc(unsigned int n) { return malloc(n); }
static void kf_free(void *p) { free(p); }
static unsigned int kf_irq_save(void) { return 0; }
static void kf_irq_restore(unsigned int f) { (void) f; }
#else
#include "memory.h"
extern unsigned int irq_save(void);
extern void irq_restore(unsigned int flags);
static void *kf_alloc(unsigned int n)
{
	void *p = mem_alloc(n, KFONT_OWNER);
	return mem_is_error((unsigned int) p) ? 0 : p;
}
static void kf_free(void *p) { (void) mem_free(p, KFONT_OWNER); }
static unsigned int kf_irq_save(void) { return irq_save(); }
static void kf_irq_restore(unsigned int f) { irq_restore(f); }
#endif

static struct kfont builtin[2];
static struct kfont *slot[KFONT_SLOTS];
static unsigned int gen[KFONT_SLOTS];
static struct kfont *zombie[ZMAX];
static struct kfont *defs[2];      /* surface text, console */
static int inited;
static unsigned int live_n, live_bytes;

static unsigned int rd16(const unsigned char *p) { return (unsigned int) p[0] | ((unsigned int) p[1] << 8); }
static unsigned int rd32(const unsigned char *p)
{
	return (unsigned int) p[0] | ((unsigned int) p[1] << 8) | ((unsigned int) p[2] << 16) | ((unsigned int) p[3] << 24);
}

unsigned int kfont_crc32(const unsigned char *p, unsigned int n)
{
	unsigned int c = 0xFFFFFFFFu, i;
	int k;

	for (i = 0; i < n; i++) {
		c ^= p[i];
		for (k = 0; k < 8; k++)
			c = (c >> 1) ^ (0xEDB88320u & (0u - (c & 1u)));
	}
	return c ^ 0xFFFFFFFFu;
}

int kfont_parse(const unsigned char *d, unsigned int len, struct kfont *f, const char **why)
{
	unsigned int gw, gh, cw, ch, xo, yo, bo, rb, cnt, dflt, nmap, enc, moff, boff, plen, i, r, pad;
	static unsigned char used[64];   /* 512 glyph bits (traps are serialised: interrupts off) */

#define BAD(s) do { if (why) *why = (s); return KFONT_ERR_FORMAT; } while (0)
	if (len < QFNT_HDR)
		BAD("truncated header");
	if (len > QFNT_MAX_FILE)
		BAD("file too large");
	if (d[0] != 'Q' || d[1] != 'F' || d[2] != 'N' || d[3] != 'T')
		BAD("bad magic");
	if (rd16(d + 4) != QFNT_VERSION)
		BAD("unsupported version");
	if (rd16(d + 6) != QFNT_HDR)
		BAD("bad header length");
	if (kfont_crc32(d, 40u) != rd32(d + 40))
		BAD("header checksum");
	gw = d[8]; gh = d[9]; cw = d[10]; ch = d[11]; xo = d[12]; yo = d[13]; bo = d[14]; rb = d[15];
	cnt = rd16(d + 16); dflt = rd16(d + 18); nmap = rd16(d + 20); enc = rd16(d + 22);
	moff = rd32(d + 24); boff = rd32(d + 28); plen = rd32(d + 32);
	if (gw < 1 || gw > 16 || gh < 1 || gh > 32 || cw < gw || cw > 32 || ch < gh || ch > 32 || xo + gw > cw ||
	    yo + gh > ch)
		BAD("bad glyph or cell size");
	if (bo != 0 || rb != (gw + 7u) / 8u)
		BAD("bad bit order or row bytes");
	if (cnt < 1 || cnt > 512 || dflt >= cnt || nmap != 256 || enc < 1 || enc > 4)
		BAD("bad glyph count, default, map or encoding");
	if (moff != QFNT_HDR || boff != QFNT_HDR + 512u || plen != len - QFNT_HDR || plen != 512u + cnt * gh * rb)
		BAD("bad offsets or payload length");
	if (kfont_crc32(d + QFNT_HDR, plen) != rd32(d + 36))
		BAD("payload checksum");
	for (i = 0; i < 64; i++)
		used[i] = 0;
	for (i = 0; i < 256; i++) {
		unsigned int m = rd16(d + moff + 2u * i);

		f->map[i] = (unsigned short) m;
		if (m == QFNT_NONE)
			continue;
		if (m >= cnt)
			BAD("map entry beyond the glyphs");
		if (used[m >> 3] & (1u << (m & 7u)))
			BAD("duplicate mapping");
		used[m >> 3] |= (unsigned char) (1u << (m & 7u));
	}
	pad = 8u * rb - gw;
	for (i = 0; i < cnt * gh; i++) {
		const unsigned char *p = d + boff + i * rb;
		unsigned int v = rb == 1 ? p[0] : ((unsigned int) p[0] << 8) | p[1];

		if (v & ((1u << pad) - 1u))
			BAD("bits outside the glyph width");
	}
	f->gw = gw; f->gh = gh; f->cw = cw; f->ch = ch; f->xo = xo; f->yo = yo; f->rb = rb;
	f->count = cnt; f->def = dflt; f->enc = enc;
	for (r = 0; r < 19; r++)
		f->name[r] = (char) (d[44 + r] >= 32 && d[44 + r] < 127 ? d[44 + r] : (d[44 + r] ? '?' : 0));
	f->name[19] = 0;
	f->crc = rd32(d + 36);
	f->bits = d + boff;
	return 0;
#undef BAD
}

static unsigned int mkid(int s)
{
	return (gen[s] << 8) | (unsigned int) (s + 1);
}

void kfont_init(void)
{
	int s;

	if (inited)
		return;
	inited = 1;
	for (s = 0; s < KFONT_SLOTS; s++) {
		slot[s] = 0;
		gen[s] = 0;
	}
	if (kfont_parse(font_recovery, font_recovery_len, &builtin[0], 0) == 0) {
		builtin[0].magic = KFONT_MAGIC;
		builtin[0].builtin = 1;
		builtin[0].refs = 1;
		builtin[0].id = KFONT_RECOVERY;
		slot[0] = &builtin[0];
	}
	if (kfont_parse(font_console, font_console_len, &builtin[1], 0) == 0) {
		builtin[1].magic = KFONT_MAGIC;
		builtin[1].builtin = 1;
		builtin[1].refs = 1;
		builtin[1].id = KFONT_CONSOLE;
		slot[1] = &builtin[1];
	}
	defs[0] = slot[0];
	defs[1] = slot[1] ? slot[1] : slot[0];
}

int kfont_load(const unsigned char *data, unsigned int len, unsigned int owner, const char **why)
{
	struct kfont tmp, *f;
	unsigned char *blk;
	unsigned int i, need;
	int s, rc;

	kfont_init();
	rc = kfont_parse(data, len, &tmp, why);
	if (rc != 0)
		return rc;
	for (s = 2; s < KFONT_SLOTS; s++)
		if (slot[s] == 0)
			break;
	if (s >= KFONT_SLOTS) {
		if (why)
			*why = "font table full";
		return KFONT_ERR_FULL;
	}
	need = (unsigned int) sizeof(struct kfont) + tmp.count * tmp.gh * tmp.rb;
	blk = (unsigned char *) kf_alloc(need);
	if (blk == 0) {
		if (why)
			*why = "no kernel heap";
		return KFONT_ERR_MEM;
	}
	f = (struct kfont *) blk;
	for (i = 0; i < (unsigned int) sizeof(struct kfont); i++)   /* no struct assignment: it calls memcpy */
		blk[i] = ((const unsigned char *) &tmp)[i];
	for (i = 0; i < tmp.count * tmp.gh * tmp.rb; i++)
		blk[sizeof(struct kfont) + i] = tmp.bits[i];
	f->bits = blk + sizeof(struct kfont);
	f->magic = KFONT_MAGIC;
	f->builtin = 0;
	f->refs = 1;                    /* the loader's */
	f->owner = owner;
	gen[s] = (gen[s] + 1u) & 0x7FFFFFu;
	if (gen[s] == 0)
		gen[s] = 1;
	f->id = mkid(s);
	slot[s] = f;
	live_n++;
	live_bytes += need;
	return (int) f->id;
}

struct kfont *kfont_get(unsigned int id)
{
	int s = (int) (id & 0xffu) - 1;

	kfont_init();
	if (s < 0 || s >= KFONT_SLOTS || slot[s] == 0 || slot[s]->id != id || slot[s]->magic != KFONT_MAGIC)
		return 0;
	return slot[s];
}

void kfont_ref(struct kfont *f)
{
	if (f != 0 && !f->builtin)
		f->refs++;
}

void kfont_unref(struct kfont *f)
{
	int s, z;

	if (f == 0 || f->builtin || f->refs <= 0)
		return;
	if (--f->refs > 0)
		return;
	s = (int) (f->id & 0xffu) - 1;
	if (s >= 0 && s < KFONT_SLOTS && slot[s] == f)
		slot[s] = 0;                /* no new references; the id is stale from now on */
	f->magic = 0;
	for (z = 0; z < ZMAX; z++)
		if (zombie[z] == 0) {
			zombie[z] = f;
			return;
		}
}

int kfont_release(unsigned int id, unsigned int owner)
{
	struct kfont *f = kfont_get(id);

	if (f == 0 || f->builtin)
		return KFONT_ERR_STALE;
	if (f->owner != owner)
		return -8;                  /* SURF_ERR_OWNER */
	f->owner = 0;
	kfont_unref(f);
	return 0;
}

void kfont_job_ended(unsigned int job_id)
{
	int s;

	for (s = 2; s < KFONT_SLOTS; s++)
		if (slot[s] != 0 && slot[s]->owner == job_id) {
			slot[s]->owner = 0;
			kfont_unref(slot[s]);
		}
}

void kfont_poll(void)
{
	int z;

	for (z = 0; z < ZMAX; z++) {
		unsigned int irq = kf_irq_save();
		struct kfont *f = zombie[z];

		zombie[z] = 0;
		if (f != 0) {
			live_n--;
			live_bytes -= (unsigned int) sizeof(struct kfont) + f->count * f->gh * f->rb;
			kf_free(f);
		}
		kf_irq_restore(irq);
	}
}

int kfont_list(unsigned int *ids, int max)
{
	int s, n = 0;

	kfont_init();
	for (s = 0; s < KFONT_SLOTS && n < max; s++)
		if (slot[s] != 0)
			ids[n++] = slot[s]->id;
	return n;
}

unsigned int kfont_live(void) { return live_n; }
unsigned int kfont_bytes(void) { return live_bytes; }

struct kfont *kfont_default(int console)
{
	kfont_init();
	return defs[console ? 1 : 0];
}

int kfont_set_default(int console, unsigned int id)
{
	struct kfont *f = kfont_get(id);

	if (f == 0)
		return KFONT_ERR_STALE;
	kfont_ref(f);
	kfont_unref(defs[console ? 1 : 0]);
	defs[console ? 1 : 0] = f;
	return 0;
}

unsigned int kfont_row(const struct kfont *f, unsigned int c, unsigned int r)
{
	unsigned int g = f->map[c & 0xffu];
	const unsigned char *p;
	unsigned int v;

	if (g == QFNT_NONE)
		g = f->def;
	if (r >= f->gh)
		return 0;
	p = f->bits + (g * f->gh + r) * f->rb;
	v = f->rb == 1 ? p[0] : ((unsigned int) p[0] << 8) | p[1];
	return v >> (8u * f->rb - f->gw);
}

unsigned int kfont_cell_row(const struct kfont *f, unsigned int c, unsigned int r, unsigned int attr)
{
	unsigned int bits = 0, mask = f->cw >= 32 ? 0xFFFFFFFFu : (1u << f->cw) - 1u;

	if (r >= f->yo && r < f->yo + f->gh)
		bits = kfont_row(f, c, r - f->yo) << (f->cw - f->xo - f->gw);
	if (attr & FONT_A_BOLD)
		bits |= bits >> 1;
	if ((attr & FONT_A_UNDER) && r == f->ch - 1u)
		bits = mask;
	return bits & mask;
}

int kfont_draw_cell(const struct kfont *f, unsigned char *buf, unsigned int w, unsigned int h, unsigned int stride,
                    int x, int y, unsigned int c, unsigned int ink, unsigned int paper, unsigned int attr)
{
	unsigned int r, k;
	int any = 0;

	if (attr & FONT_A_INVERSE) {
		unsigned int t = ink;
		ink = paper;
		paper = t;
	}
	for (r = 0; r < f->ch; r++) {
		int py = y + (int) r;
		unsigned int bits;

		if (py < 0 || py >= (int) h)
			continue;
		bits = kfont_cell_row(f, c, r, attr);
		for (k = 0; k < f->cw; k++) {
			int px = x + (int) k;

			if (px < 0 || px >= (int) w)
				continue;
			any = 1;
			if (bits & (1u << (f->cw - 1u - k)))
				buf[(unsigned int) py * stride + (unsigned int) px] = (unsigned char) ink;
			else if (!(attr & FONT_A_NOPAPER))
				buf[(unsigned int) py * stride + (unsigned int) px] = (unsigned char) paper;
		}
	}
	return any;
}
