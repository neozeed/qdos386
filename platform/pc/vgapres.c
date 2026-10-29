/*
 * QDOS/386 R13 - the display presenter (kernel/present.h): the only code that changes the VGA mode, the DAC and
 * the planes.  Empty unless QDOS_SURFACES.  With QDOS_CON_VGA it drives text mode 3 (the BIOS's own register set,
 * read back at boot) and mode 13h (320x200x256, chain-4, standard register values) by direct register programming -
 * there is no BIOS call in protected mode.  Without QDOS_CON_VGA (the headless Tiny386 build) every function is a
 * stub: surfaces exist only in memory.
 *
 * Context rules: pres_focus_changed may run in the keyboard IRQ; it only sets a flag.  pres_poll runs in the SYSTEM
 * job every tick with interrupts on: while it reprograms the card, pres_text_ok() is 0, so no trap touches the card
 * (vdisp_* in vga.c check it); the register sequences and frame copies are done with interrupts on.  The text is
 * redrawn from the console cells (vcon_present_text) with interrupts off, as R12C's console switch did.
 * Freestanding C89.
 */
#ifdef QDOS_SURFACES
#include "present.h"
#include "vcon.h"
#include "gsurf.h"
#include "font.h"

extern unsigned int irq_save(void);
extern void irq_restore(unsigned int flags);

#ifdef QDOS_CON_VGA
extern void outb(unsigned int port, unsigned int val);
extern unsigned int inb(unsigned int port);
void vdisp_reset_cache(void);           /* vga.c: forget the cursor registers it last wrote */

#define P_AC     0x3c0
#define P_AC_R   0x3c1
#define P_MISC_W 0x3c2
#define P_SEQ    0x3c4
#define P_DAC_RI 0x3c7
#define P_DAC_WI 0x3c8
#define P_DAC_D  0x3c9
#define P_MISC_R 0x3cc
#define P_GC     0x3ce
#define P_CRTC   0x3d4
#define P_ISR1   0x3da

#define VRAM ((volatile unsigned char *) 0xA0000u)

enum { PHYS_TEXT = 0, PHYS_BUSY = 1, PHYS_GFX = 2 };

struct vregs {
	unsigned char misc, seq[5], crtc[25], gc[9], ac[21];
};

/* Mode 13h: 320x200, 256 colours, chain-4 (the standard IBM VGA values). */
static const struct vregs mode13 = {
	0x63,
	{ 0x03, 0x01, 0x0f, 0x00, 0x0e },
	{ 0x5f, 0x4f, 0x50, 0x82, 0x54, 0x80, 0xbf, 0x1f, 0x00, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	  0x9c, 0x0e, 0x8f, 0x28, 0x40, 0x96, 0xb9, 0xa3, 0xff },
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x05, 0x0f, 0xff },
	{ 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
	  0x41, 0x00, 0x0f, 0x00, 0x00 }
};

static struct vregs text_regs;          /* the BIOS's mode 3, read back at boot */
static unsigned char text_dac[768];
static unsigned char text_font[8192];   /* plane 2: 256 characters x 32 bytes */
static int saved;
static volatile int phys = PHYS_TEXT;
static volatile int want_check = 1;
static struct gsurf *shown_g;
static unsigned int shown_tag, shown_pal_gen;
static int shown_idx = -1;
static unsigned int st_text, st_gfx, st_full, st_rows, st_pal;
static unsigned int cur_col, cur_row, cur_vis;     /* the cursor last drawn into VRAM */

static void read_regs(struct vregs *r)
{
	unsigned int i;

	r->misc = (unsigned char) inb(P_MISC_R);
	for (i = 0; i < 5; i++) {
		outb(P_SEQ, i);
		r->seq[i] = (unsigned char) inb(P_SEQ + 1);
	}
	for (i = 0; i < 25; i++) {
		outb(P_CRTC, i);
		r->crtc[i] = (unsigned char) inb(P_CRTC + 1);
	}
	for (i = 0; i < 9; i++) {
		outb(P_GC, i);
		r->gc[i] = (unsigned char) inb(P_GC + 1);
	}
	for (i = 0; i < 21; i++) {
		(void) inb(P_ISR1);
		outb(P_AC, i);
		r->ac[i] = (unsigned char) inb(P_AC_R);
	}
	(void) inb(P_ISR1);
	outb(P_AC, 0x20);                   /* palette address source: the display runs again */
}

static void write_regs(const struct vregs *r)
{
	unsigned int i;
	unsigned char c;

	outb(P_MISC_W, r->misc);
	outb(P_SEQ, 0);
	outb(P_SEQ + 1, 0x01);              /* synchronous reset while the clocking changes */
	for (i = 1; i < 5; i++) {
		outb(P_SEQ, i);
		outb(P_SEQ + 1, r->seq[i]);
	}
	outb(P_SEQ, 0);
	outb(P_SEQ + 1, r->seq[0]);
	outb(P_CRTC, 0x03);                 /* unlock CRTC 0..7 */
	outb(P_CRTC + 1, inb(P_CRTC + 1) | 0x80);
	outb(P_CRTC, 0x11);
	outb(P_CRTC + 1, inb(P_CRTC + 1) & 0x7f);
	for (i = 0; i < 25; i++) {
		c = r->crtc[i];
		if (i == 0x03)
			c |= 0x80;
		if (i == 0x11)
			c &= 0x7f;
		outb(P_CRTC, i);
		outb(P_CRTC + 1, c);
	}
	for (i = 0; i < 9; i++) {
		outb(P_GC, i);
		outb(P_GC + 1, r->gc[i]);
	}
	for (i = 0; i < 21; i++) {
		(void) inb(P_ISR1);
		outb(P_AC, i);
		outb(P_AC, r->ac[i]);
	}
	(void) inb(P_ISR1);
	outb(P_AC, 0x20);
}

/* Planar access to plane 2 at 0xA0000 (font load/save in text mode), then back to the text values. */
static void plane2(int on)
{
	if (on) {
		outb(P_SEQ, 2); outb(P_SEQ + 1, 0x04);       /* write plane 2 only */
		outb(P_SEQ, 4); outb(P_SEQ + 1, 0x06);       /* sequential addressing, extended memory */
		outb(P_GC, 4); outb(P_GC + 1, 0x02);         /* read plane 2 */
		outb(P_GC, 5); outb(P_GC + 1, 0x00);         /* write mode 0, no odd/even */
		outb(P_GC, 6); outb(P_GC + 1, 0x04);         /* 0xA0000, 64 KiB, no chaining */
	} else {
		outb(P_SEQ, 2); outb(P_SEQ + 1, text_regs.seq[2]);
		outb(P_SEQ, 4); outb(P_SEQ + 1, text_regs.seq[4]);
		outb(P_GC, 4); outb(P_GC + 1, text_regs.gc[4]);
		outb(P_GC, 5); outb(P_GC + 1, text_regs.gc[5]);
		outb(P_GC, 6); outb(P_GC + 1, text_regs.gc[6]);
	}
}

static void dac_read(unsigned char *d)
{
	unsigned int i;

	outb(P_DAC_RI, 0);
	for (i = 0; i < 768; i++)
		d[i] = (unsigned char) (inb(P_DAC_D) & 0x3f);
}

static void dac_write6(const unsigned char *d)
{
	unsigned int i;

	outb(P_DAC_WI, 0);
	for (i = 0; i < 768; i++)
		outb(P_DAC_D, d[i] & 0x3f);
}

static void dac_write8(const unsigned char *rgb)          /* 0..255 per channel -> the DAC's 6 bits */
{
	unsigned int i;

	outb(P_DAC_WI, 0);
	for (i = 0; i < 768; i++)
		outb(P_DAC_D, (unsigned int) (rgb[i] >> 2));
}

void pres_init(void)
{
	unsigned int i;

	read_regs(&text_regs);
	dac_read(text_dac);
	plane2(1);
	for (i = 0; i < 8192; i++)
		text_font[i] = VRAM[i];
	plane2(0);
	saved = 1;
	phys = PHYS_TEXT;
}

unsigned int pres_caps(void)
{
	return saved ? (PRES_CAP_TEXT | PRES_CAP_MODE13) : PRES_CAP_TEXT;
}

void pres_focus_changed(void)
{
	want_check = 1;
}

int pres_text_ok(void)
{
	return phys == PHYS_TEXT;
}

void pres_stats(unsigned int *o)
{
	o[0] = st_text;
	o[1] = st_gfx;
	o[2] = st_full;
	o[3] = st_rows;
	o[4] = st_pal;
}

static void to_text(void)
{
	unsigned int i, irq;

	write_regs(&text_regs);
	plane2(1);
	for (i = 0; i < 8192; i++)
		VRAM[i] = text_font[i];
	plane2(0);
	dac_write6(text_dac);
	vdisp_reset_cache();
	irq = irq_save();
	phys = PHYS_TEXT;
	vcon_present_text();                 /* the foreground cells, or nothing while the selector redraws itself */
	irq_restore(irq);
	shown_g = 0;
	shown_idx = -1;
	st_text++;
}

static void copy_rows(const struct gsurf *g, int y0, int y1)
{
	int y;

	for (y = y0; y < y1; y++) {
		const unsigned char *s = g->pix + (unsigned int) y * g->stride;
		volatile unsigned char *d = VRAM + (unsigned int) y * 320u;
		unsigned int x;

		if ((g->w & 3u) == 0) {
			const unsigned int *s4 = (const unsigned int *) s;
			volatile unsigned int *d4 = (volatile unsigned int *) d;
			for (x = 0; x < g->w / 4u; x++)
				d4[x] = s4[x];
		} else {
			for (x = 0; x < g->w; x++)
				d[x] = s[x];
		}
		for (x = g->w; x < 320u; x++)
			d[x] = 0;
	}
	st_rows += (unsigned int) (y1 - y0);
}

/* The text-stream cursor: an underline in rows 8..9 of its cell, drawn into VRAM only (never into the pixels). */
static void draw_cursor(const struct gsurf *g)
{
	unsigned int r, k, x0, y0;

	cur_vis = g->cursor_on && g->tcol < g->cols && g->trow < g->rows;
	cur_col = g->tcol;
	cur_row = g->trow;
	if (!cur_vis)
		return;
	x0 = cur_col * FONT_CELL_W;
	y0 = cur_row * FONT_CELL_H;
	for (r = 8; r < FONT_CELL_H; r++)
		for (k = 0; k < FONT_CELL_W; k++)
			VRAM[(y0 + r) * 320u + x0 + k] = (unsigned char) g->ink;
}

static void to_gfx(struct gsurf *g, int idx, unsigned int tag)
{
	unsigned int irq;
	int y;

	if (phys != PHYS_GFX) {
		write_regs(&mode13);
		st_gfx++;
	}
	irq = irq_save();
	g->dy0 = g->dy1 = 0;
	shown_pal_gen = g->pal_gen;
	irq_restore(irq);
	dac_write8(g->pal);
	st_pal++;
	copy_rows(g, 0, (int) g->h);
	for (y = (int) g->h; y < 200; y++) {
		unsigned int x;
		for (x = 0; x < 320u; x++)
			VRAM[(unsigned int) y * 320u + x] = 0;
	}
	draw_cursor(g);
	shown_g = g;
	shown_idx = idx;
	shown_tag = tag;
	phys = PHYS_GFX;
	st_full++;
}

void pres_poll(void)
{
	unsigned int irq;
	struct gsurf *g;
	int idx, y0, y1, recheck, over;
	unsigned int tag, pgen;

	if (!saved)
		return;
	irq = irq_save();
	recheck = want_check;
	want_check = 0;
	over = vcon_overlay_on();
	idx = vcon_fg();
	g = over ? 0 : vsess_gfx(idx);
	tag = vsess_tag(idx);
	if (phys == PHYS_TEXT && g == 0) {
		irq_restore(irq);
		return;                          /* text shown, text wanted: the console layer draws it directly */
	}
	if (phys == PHYS_TEXT)
		phys = PHYS_BUSY;                /* from now on no trap writes to the card */
	irq_restore(irq);
	if (g == 0) {
		to_text();
		return;
	}
	if (phys != PHYS_GFX || recheck || g != shown_g || idx != shown_idx || tag != shown_tag) {
		to_gfx(g, idx, tag);
		return;
	}
	irq = irq_save();
	y0 = g->dy0;
	y1 = g->dy1;
	g->dy0 = g->dy1 = 0;
	pgen = g->pal_gen;
	irq_restore(irq);
	if (pgen != shown_pal_gen) {
		shown_pal_gen = pgen;
		dac_write8(g->pal);
		st_pal++;
	}
	if (cur_vis && (cur_col != g->tcol || cur_row != g->trow || !g->cursor_on)) {
		int cy = (int) (cur_row * FONT_CELL_H);   /* erase the old cursor: copy its cell row again */
		if (cy < (int) g->h)
			copy_rows(g, cy, cy + FONT_CELL_H > (int) g->h ? (int) g->h : cy + FONT_CELL_H);
	}
	if (y0 < y1)
		copy_rows(g, y0, y1);
	if (y0 < y1 || g->cursor_on != (int) cur_vis || cur_col != g->tcol || cur_row != g->trow)
		draw_cursor(g);
}

#else /* no display: surfaces exist in memory only */

void pres_init(void)
{
}

unsigned int pres_caps(void)
{
	return 0;
}

void pres_focus_changed(void)
{
}

int pres_text_ok(void)
{
	return 1;
}

void pres_poll(void)
{
}

void pres_stats(unsigned int *o)
{
	o[0] = o[1] = o[2] = o[3] = o[4] = 0;
}
#endif /* QDOS_CON_VGA */
#else
typedef int vgapres_off_unit;        /* an empty translation unit is not ISO C */
#endif /* QDOS_SURFACES */
