/*
 * QDOS/386 R17 Apollo 13 - firmware memory map (see memmap.h, docs/APOLLO13-MEMORY-MAP.md).
 */
#include "memmap.h"
#include "platform.h"

#define MM_BOOT_TAG   (0x7C00u + 0x1CAu)    /* the QBOOT3 boot sector is still at 0x7C00 when the kernel starts */
#define MM_END_PTR    0x0500u
#define MM_FIRST      0x0504u
#define MM_RECSZ      24u
#define MM_PAGES      (MM_CEILING / 4096u)  /* 65,536 */
#define MM_LOW_PAGES  256u                  /* below 1 MiB: never pool */

static struct mm_rec raw[MM_MAX];
static int nraw = 0, source = MM_SRC_NONE, captured = 0;
static unsigned char usable[MM_PAGES / 8u];
static struct mm_summary sum;

#define BIT(i)   (usable[(i) >> 3] & (unsigned char) (1u << ((i) & 7u)))
#define SETB(i)  (usable[(i) >> 3] |= (unsigned char) (1u << ((i) & 7u)))
#define CLRB(i)  (usable[(i) >> 3] &= (unsigned char) ~(1u << ((i) & 7u)))

void memmap_capture(void)
{
	const unsigned char *tag = (const unsigned char *) MM_BOOT_TAG;
	unsigned int end, a, i, k;
	const unsigned int *w;

	nraw = 0;
	captured = 0;
	if (tag[0] != 'Q' || tag[1] != 'B' || tag[2] != 'O' || tag[3] != 'O' || tag[4] != 'T' || tag[5] != '3')
		return;                                    /* not the QBOOT3 loader: 0x500 is not ours */
	end = *(const unsigned short *) MM_END_PTR;
	if (end < MM_FIRST || end > MM_FIRST + MM_MAX * MM_RECSZ || (end - MM_FIRST) % MM_RECSZ != 0)
		return;
	captured = 1;
	for (a = MM_FIRST, i = 0; a < end; a += MM_RECSZ, i++) {
		w = (const unsigned int *) a;
		raw[i].base_lo = w[0];
		raw[i].base_hi = w[1];
		raw[i].len_lo = w[2];
		raw[i].len_hi = w[3];
		raw[i].type = w[4];
	}
	nraw = (int) i;
	for (k = 0; k < MM_PAGES / 8u; k++)
		usable[k] = 0;
}

/* page range of a record, clamped to [MM_LOW_PAGES, MM_PAGES): inward (usable) or outward (reserved).  Returns 0 if
 * the record is unusable for this purpose (zero length, entirely at or above 4 GiB). */
static int rec_pages(const struct mm_rec *r, int inward, unsigned int *pf, unsigned int *pl, int *overflow)
{
	unsigned int end_lo, end_hi, first, last;

	*overflow = 0;
	if (r->len_lo == 0 && r->len_hi == 0)
		return 0;
	if (r->base_hi != 0)
		return 0;
	end_lo = r->base_lo + r->len_lo;
	end_hi = r->len_hi + (end_lo < r->base_lo ? 1u : 0u);
	if (end_hi != 0) {                                  /* ends at or beyond 4 GiB */
		*overflow = 1;
		last = 0x100000u;                               /* page number of 4 GiB */
	} else
		last = inward ? (end_lo >> 12) : ((end_lo >> 12) + ((end_lo & 0xFFFu) ? 1u : 0u));
	if (inward)
		first = (r->base_lo >> 12) + ((r->base_lo & 0xFFFu) ? 1u : 0u);
	else
		first = r->base_lo >> 12;
	if (first < MM_LOW_PAGES)
		first = MM_LOW_PAGES;
	if (last > MM_PAGES)
		last = MM_PAGES;
	*pf = first;
	*pl = last;
	return 1;
}

int memmap_build(void)
{
	int i, any = 0, ovf;
	unsigned int f, l, p, top = 0;

#ifdef QDOS_R17_TEST_RESERVE
	/* TEST ONLY (make qdos386-r17-pc R17_EXTRA=-DQDOS_R17_TEST_RESERVE): inject a reserved record over usable RAM, as a
	 * firmware might, so the run can prove the region never enters the pool and its contents survive. */
	if (captured && nraw < MM_MAX) {
		raw[nraw].base_lo = MM_TEST_BASE; raw[nraw].base_hi = 0;
		raw[nraw].len_lo = MM_TEST_LEN; raw[nraw].len_hi = 0;
		raw[nraw].type = 2u;
		nraw++;
	}
#endif
	sum.nraw = (unsigned int) nraw;
	sum.nused_recs = sum.nreserved_recs = sum.nignored = 0;
	sum.usable_kib = sum.reserved_kib = sum.above_ceiling_kib = 0;
	source = MM_SRC_NONE;
	if (!captured || nraw == 0)
		return source;
	for (i = 0; i < nraw; i++) {                        /* pass 1: usable records */
		if (raw[i].type != 1u)
			continue;
		sum.nused_recs++;
		if (!rec_pages(&raw[i], 1, &f, &l, &ovf)) {
			sum.nignored++;
			continue;
		}
		if (raw[i].base_hi == 0 && raw[i].len_hi == 0 && !ovf) {
			unsigned int e = raw[i].base_lo + raw[i].len_lo;
			if (e > MM_CEILING)
				sum.above_ceiling_kib += (e - (raw[i].base_lo > MM_CEILING ? raw[i].base_lo : MM_CEILING)) >> 10;
		} else
			sum.above_ceiling_kib = 0xFFFFFFFFu;        /* "more than the counter holds": reported as such */
		for (p = f; p < l; p++)
			SETB(p);
	}
	for (i = 0; i < nraw; i++) {                        /* pass 2: anything else wins over usable */
		if (raw[i].type == 1u)
			continue;
		sum.nreserved_recs++;
		if (!rec_pages(&raw[i], 0, &f, &l, &ovf))
			continue;
		if (l > f)
			sum.reserved_kib += (l - f) * 4u;
		for (p = f; p < l; p++)
			CLRB(p);
	}
	for (p = MM_LOW_PAGES; p < MM_PAGES; p++)
		if (BIT(p)) {
			any = 1;
			sum.usable_kib += 4u;
			top = (p + 1u) * 4096u;
		}
	sum.top = top;
	if (any)
		source = MM_SRC_E820;
	return source;
}

int memmap_source(void) { return source; }
unsigned int memmap_top(void) { return source == MM_SRC_E820 ? sum.top : 0u; }

int memmap_page_usable(unsigned int pa)
{
	unsigned int p = pa >> 12;
	if (source != MM_SRC_E820 || pa >= MM_CEILING || p < MM_LOW_PAGES)
		return 0;
	return BIT(p) != 0;
}

void memmap_summary(struct mm_summary *s) { *s = sum; }
const struct mm_rec *memmap_raw(int *n) { *n = nraw; return raw; }

static void hex8(unsigned int v)
{
	static const char hx[] = "0123456789abcdef";
	int s;
	for (s = 28; s >= 0; s -= 4)
		con_putc((unsigned char) hx[(v >> s) & 15u]);
}

void memmap_print(void)
{
	int i;

	if (source != MM_SRC_E820) {
		con_puts(captured ? "MEMMAP: E820 map unusable, CMOS fallback (16 MiB rule)\n"
		                  : "MEMMAP: no E820 map from the boot loader, CMOS fallback (16 MiB rule)\n");
		return;
	}
	con_puts("MEMMAP: E820 ");
	con_putdec((unsigned long) nraw);
	con_puts(" records\n");
	for (i = 0; i < nraw; i++) {
		con_puts("  ");
		hex8(raw[i].base_hi); hex8(raw[i].base_lo);
		con_puts(" +");
		hex8(raw[i].len_hi); hex8(raw[i].len_lo);
		con_puts(" type ");
		con_putdec((unsigned long) raw[i].type);
		con_puts("\n");
	}
	con_puts("MEMMAP: usable ");
	con_putdec((unsigned long) sum.usable_kib);
	con_puts(" KiB below the 256 MiB ceiling, top ");
	hex8(sum.top);
	if (sum.above_ceiling_kib) {
		con_puts(", ");
		con_putdec((unsigned long) sum.above_ceiling_kib);
		con_puts(" KiB above it not used");
	}
	con_puts("\n");
}

#ifdef QDOS_R17_TEST_RESERVE
/* TEST ONLY: poison the injected region (template active, paging on) and count mismatching words later. */
unsigned int pg_enter_template(void);
void pg_leave_template(unsigned int was);
void memmap_poison_write(void)
{
	volatile unsigned int *w = (volatile unsigned int *) MM_TEST_BASE;
	unsigned int i;
	for (i = 0; i < MM_TEST_LEN / 4u; i++)
		w[i] = 0xBAD00000u ^ (MM_TEST_BASE + 4u * i);
}
unsigned int memmap_poison_check(void)
{
	volatile unsigned int *w = (volatile unsigned int *) MM_TEST_BASE;
	unsigned int i, bad = 0, was = pg_enter_template();
	for (i = 0; i < MM_TEST_LEN / 4u; i++)
		if (w[i] != (0xBAD00000u ^ (MM_TEST_BASE + 4u * i)))
			bad++;
	pg_leave_template(was);
	return bad;
}
#endif
