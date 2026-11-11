/*
 * QDOS/386 R13A - the display presenter (kernel/present.h), derived from R13's platform/pc/vgapres.c (which R13
 * builds keep): the only code that changes the VGA mode, the DAC and the planes.  R13A adds: logical INDEX8 surfaces
 * larger than the screen (up to 512x256) presented scaled to fit (nearest neighbour, aspect kept, centred) or as a
 * 1:1 cropped window; and rendered text consoles: a session in VSESS_CM_GFX is drawn from its cells with a bitmap
 * font in mode 12h (640x480, 16 colours, planar), the cursor overlaid on the screen only.  Empty unless QDOS_SURFACES.  With QDOS_CON_VGA it drives text mode 3 (the BIOS's own register set,
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
#include "fontreg.h"
#include "kev.h"

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

enum { PHYS_TEXT = 0, PHYS_BUSY = 1, PHYS_GFX = 2, PHYS_G12 = 3 };

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

/* Mode 12h: 640x480, 16 colours, planar (the standard IBM VGA values); attribute palette identity 0..15. */
static const struct vregs mode12 = {
	0xe3,
	{ 0x03, 0x01, 0x0f, 0x00, 0x06 },
	{ 0x5f, 0x4f, 0x50, 0x82, 0x54, 0x80, 0x0b, 0x3e, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	  0xea, 0x8c, 0xdf, 0x28, 0x00, 0xe7, 0x04, 0xe3, 0xff },
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x0f, 0xff },
	{ 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
	  0x01, 0x00, 0x0f, 0x00, 0x00 }
};
/* The 16 text colours (6-bit DAC values of the standard EGA/VGA text palette). */
static const unsigned char ega6[16][3] = {
	{ 0, 0, 0 }, { 0, 0, 42 }, { 0, 42, 0 }, { 0, 42, 42 }, { 42, 0, 0 }, { 42, 0, 42 }, { 42, 21, 0 }, { 42, 42, 42 },
	{ 21, 21, 21 }, { 21, 21, 63 }, { 21, 63, 21 }, { 21, 63, 63 }, { 63, 21, 21 }, { 63, 21, 63 }, { 63, 63, 21 },
	{ 63, 63, 63 }
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
static unsigned int shown_view;
#ifdef QDOS_R18
static unsigned int eff_view;           /* R18: the view in use: the session override (vsess_view_ov) or the program's */
#endif

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
#ifdef QDOS_R18
	return saved ? (PRES_CAP_TEXT | PRES_CAP_MODE13 | PRES_CAP_MODE12 | PRES_CAP_G12GFX) : PRES_CAP_TEXT;
#else
	return saved ? (PRES_CAP_TEXT | PRES_CAP_MODE13 | PRES_CAP_MODE12) : PRES_CAP_TEXT;
#endif
}

void pres_focus_changed(void)
{
	want_check = 1;
	kev_refocus();
}

static struct pres_shadow shadow = { 0x44485351u, 1u };

const struct pres_shadow *pres_shadow_ptr(void)
{
	return &shadow;
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
	shadow.mode = 0;
	st_text++;
}


/* ---- mode 13h: INDEX8 surfaces, 1:1 (fits the screen) or scaled / cropped (R13A) ---- */
static unsigned int sx0, sy0, sdw, sdh;      /* the picture's place on the screen */
static int scaled;                            /* 0: 1:1 at the top left (the R13 rule), 1: fit, 2: crop */

static void layout(const struct gsurf *g)
{
#ifdef QDOS_R18
	if (eff_view == GS_VIEW_CROP) {
#else
	if (g->view == GS_VIEW_CROP) {
#endif
		scaled = 2;
		sdw = g->w < 320u ? g->w : 320u;
		sdh = g->h < 200u ? g->h : 200u;
		sx0 = sy0 = 0;
	} else if (g->w <= 320u && g->h <= 200u) {
		scaled = 0;
		sdw = g->w;
		sdh = g->h;
		sx0 = sy0 = 0;
	} else {
		scaled = 1;
		if ((unsigned long) g->w * 200ul >= (unsigned long) g->h * 320ul) {
			sdw = 320u;
			sdh = (unsigned int) ((unsigned long) g->h * 320ul / g->w);
		} else {
			sdh = 200u;
			sdw = (unsigned int) ((unsigned long) g->w * 200ul / g->h);
		}
		sx0 = (320u - sdw) / 2u;
		sy0 = (200u - sdh) / 2u;
	}
}

/* the source row shown on screen row d of the picture */
static unsigned int src_row(const struct gsurf *g, unsigned int d)
{
	if (scaled == 1)
		return (unsigned int) ((unsigned long) d * g->h / sdh);
	if (scaled == 2)
		return g->vy + d;
	return d;
}

static void draw_screen_row(const struct gsurf *g, unsigned int y)      /* screen row y (0..199) */
{
	volatile unsigned char *d = VRAM + y * 320u;
	unsigned int x;

	if (y < sy0 || y >= sy0 + sdh) {
		for (x = 0; x < 320u; x++)
			d[x] = 0;
		return;
	}
	{
		unsigned int sy = src_row(g, y - sy0);
		const unsigned char *s = g->pix + sy * g->stride;

		if (sy >= g->h) {
			for (x = 0; x < 320u; x++)
				d[x] = 0;
			return;
		}
		for (x = 0; x < sx0; x++)
			d[x] = 0;
		if (scaled == 0 && (g->w & 3u) == 0) {
			const unsigned int *s4 = (const unsigned int *) s;
			volatile unsigned int *d4 = (volatile unsigned int *) d;
			for (x = 0; x < g->w / 4u; x++)
				d4[x] = s4[x];
		} else if (scaled == 1) {
			for (x = 0; x < sdw; x++)
				d[sx0 + x] = s[(unsigned long) x * g->w / sdw];
		} else {
			unsigned int ox = scaled == 2 ? g->vx : 0u;
			for (x = 0; x < sdw; x++)
				d[sx0 + x] = (ox + x) < g->w ? s[ox + x] : 0;
		}
		for (x = sx0 + sdw; x < 320u; x++)
			d[x] = 0;
	}
	st_rows++;
}

/* the screen rows that show source rows [y0, y1) */
static void copy_rows(const struct gsurf *g, int y0, int y1)
{
	unsigned int d;

	if (scaled == 0) {
		for (d = (unsigned int) y0; d < (unsigned int) y1 && d < 200u; d++)
			draw_screen_row(g, d);
		return;
	}
	for (d = 0; d < sdh; d++) {
		unsigned int sy = src_row(g, d);
		if ((int) sy >= y0 && (int) sy < y1)
			draw_screen_row(g, sy0 + d);
	}
}

/* The text-stream cursor of a 1:1 surface: an underline in the last two rows of its cell, drawn into VRAM only. */
static void draw_cursor(const struct gsurf *g)
{
	unsigned int r, k, x0, y0, cw = g->font->cw, ch = g->font->ch;

	cur_vis = scaled == 0 && g->cursor_on && g->tcol < g->cols && g->trow < g->rows;
	cur_col = g->tcol;
	cur_row = g->trow;
	if (!cur_vis)
		return;
	x0 = cur_col * cw;
	y0 = cur_row * ch;
	for (r = ch - 2u; r < ch; r++)
		for (k = 0; k < cw; k++)
			if (y0 + r < 200u && x0 + k < 320u)
				VRAM[(y0 + r) * 320u + x0 + k] = (unsigned char) g->ink;
}

static void to_gfx(struct gsurf *g, int idx, unsigned int tag)
{
	unsigned int irq, y;

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
	layout(g);
	for (y = 0; y < 200u; y++)
		draw_screen_row(g, y);
	draw_cursor(g);
	shown_g = g;
	shown_idx = idx;
	shown_tag = tag;
#ifdef QDOS_R18
	shown_view = eff_view | (g->vx << 2) | (g->vy << 16);
#else
	shown_view = g->view | (g->vx << 2) | (g->vy << 16);
#endif
	phys = PHYS_GFX;
	shadow.mode = 2;
	shadow.session = (unsigned int) idx;
	st_full++;
}

/* ---- mode 12h: rendered text consoles (R13A) ---- */
static unsigned char line[640];
static unsigned int t_cw, t_ch, t_ox, t_oy, t_font, t_cur;

static const struct kfont *console_font(int idx)
{
	const struct kfont *f = vsess_font(idx);

	if (f == 0 || f->cw * 80u > 640u || f->ch * 25u > 480u)
		f = kfont_default(1);
	if (f == 0 || f->cw * 80u > 640u || f->ch * 25u > 480u)
		f = kfont_get(KFONT_RECOVERY);
	return f;
}

#ifdef QDOS_R18
/* R18: the same planes, gathered in one pass: spread[v] has bit p of v at bit 8p, so after 8 pixels w holds the four
 * plane bytes; then each plane's 80 bytes are written with its map mask (no VRAM reads, latches unused). */
static const unsigned int spread[16] = {
	0x00000000u, 0x00000001u, 0x00000100u, 0x00000101u, 0x00010000u, 0x00010001u, 0x00010100u, 0x00010101u,
	0x01000000u, 0x01000001u, 0x01000100u, 0x01000101u, 0x01010000u, 0x01010001u, 0x01010100u, 0x01010101u };
static unsigned char pl[4][80];
static void put_line(unsigned int y)
{
	unsigned int b, w, p;
	const unsigned char *s = line;
	volatile unsigned char *d = VRAM + y * 80u;

	for (b = 0; b < 80u; b++, s += 8) {
		w = spread[s[0] & 15u];
		w = (w << 1) | spread[s[1] & 15u];
		w = (w << 1) | spread[s[2] & 15u];
		w = (w << 1) | spread[s[3] & 15u];
		w = (w << 1) | spread[s[4] & 15u];
		w = (w << 1) | spread[s[5] & 15u];
		w = (w << 1) | spread[s[6] & 15u];
		w = (w << 1) | spread[s[7] & 15u];
		pl[0][b] = (unsigned char) w;
		pl[1][b] = (unsigned char) (w >> 8);
		pl[2][b] = (unsigned char) (w >> 16);
		pl[3][b] = (unsigned char) (w >> 24);
	}
	for (p = 0; p < 4u; p++) {
		const unsigned int *q = (const unsigned int *) pl[p];
		volatile unsigned int *d4 = (volatile unsigned int *) d;
		outb(P_SEQ, 2);
		outb(P_SEQ + 1, 1u << p);
		for (b = 0; b < 20u; b++)
			d4[b] = q[b];
	}
	outb(P_SEQ, 2);
	outb(P_SEQ + 1, 0x0f);
}
#else
static void put_line(unsigned int y)          /* line[0..639] (colours 0..15) to screen row y, plane by plane */
{
	unsigned int p, b, k;
	volatile unsigned char *d = VRAM + y * 80u;

	for (p = 0; p < 4; p++) {
		outb(P_SEQ, 2);
		outb(P_SEQ + 1, 1u << p);
		for (b = 0; b < 80u; b++) {
			unsigned int v = 0;
			const unsigned char *s = line + b * 8u;
			for (k = 0; k < 8; k++)
				v = (v << 1) | ((s[k] >> p) & 1u);
			d[b] = (unsigned char) v;
		}
	}
	outb(P_SEQ, 2);
	outb(P_SEQ + 1, 0x0f);
}
#endif

/* Render text row r of the session (cells) into screen rows; the cursor is drawn if it is on this row. */
static void text_row(const struct kfont *f, const unsigned short *cells, unsigned int r, unsigned int cur)
{
	unsigned int y, c, k;
	int curon = (cur >> 16) & 1u, curcol = (int) (cur & 0xffu), currow = (int) ((cur >> 8) & 0xffu);

	for (y = 0; y < f->ch; y++) {
		for (k = 0; k < 640u; k++)
			line[k] = 0;
		for (c = 0; c < 80u; c++) {
			unsigned short cell = cells[r * 80u + c];
			unsigned int fgc = (cell >> 8) & 0x0fu, bgc = (cell >> 12) & 0x07u;
			unsigned int bits = kfont_cell_row(f, cell & 0xffu, y, 0);
			unsigned char *o = line + t_ox + c * f->cw;

			if (curon && currow == (int) r && curcol == (int) c && y + 2u >= f->ch)
				bits = 0xFFFFFFFFu;             /* the cursor: the last two rows in the ink colour */
			for (k = 0; k < f->cw; k++)
				o[k] = (unsigned char) ((bits >> (f->cw - 1u - k)) & 1u ? fgc : bgc);
		}
		put_line(t_oy + r * f->ch + y);
	}
	st_rows += f->ch;
}

static void blank_rows(unsigned int y0, unsigned int y1)
{
	unsigned int k;

	for (k = 0; k < 640u; k++)
		line[k] = 0;
	for (; y0 < y1; y0++)
		put_line(y0);
}

static void to_gtext(int idx, unsigned int tag)
{
	const struct kfont *f = console_font(idx);
	const unsigned short *cells;
	unsigned int cur = 0, r, i, irq;

	if (phys != PHYS_G12) {
		write_regs(&mode12);
		outb(P_DAC_WI, 0);
		for (i = 0; i < 16; i++) {
			outb(P_DAC_D, ega6[i][0]);
			outb(P_DAC_D, ega6[i][1]);
			outb(P_DAC_D, ega6[i][2]);
		}
		st_gfx++;
	}
	t_cw = f->cw;
	t_ch = f->ch;
	t_ox = (640u - 80u * f->cw) / 2u;
	t_oy = (480u - 25u * f->ch) / 2u;
	t_font = f->id;
	irq = irq_save();
	cells = vsess_cells(idx, &cur);
	if (cells != 0)
		for (i = 0; i < 2000u; i++)
			shadow.cell[i] = cells[i];
	irq_restore(irq);
	blank_rows(0, t_oy);
	for (r = 0; r < 25u; r++)
		text_row(f, shadow.cell, r, cur);
	blank_rows(t_oy + 25u * f->ch, 480u);
	t_cur = cur;
	shown_g = 0;
	shown_idx = idx;
	shown_tag = tag;
	phys = PHYS_G12;
	shadow.mode = 3;
	shadow.session = (unsigned int) idx;
	shadow.cursor = cur;
	shadow.font_id = f->id;
	shadow.cell_w = f->cw;
	shadow.cell_h = f->ch;
	shadow.ox = t_ox;
	shadow.oy = t_oy;
	shadow.frames++;
	st_full++;
}

static void gtext_update(int idx)
{
	const struct kfont *f = console_font(idx);
	static unsigned short now[2000];
	const unsigned short *cells;
	unsigned int cur = 0, r, i, irq, changed = 0;

	if (f->id != t_font) {
		to_gtext(idx, shown_tag);
		return;
	}
	irq = irq_save();
	cells = vsess_cells(idx, &cur);
	if (cells != 0)
		for (i = 0; i < 2000u; i++)
			now[i] = cells[i];
	irq_restore(irq);
	if (cells == 0)
		return;
	for (r = 0; r < 25u; r++) {
		int diff = 0;
		for (i = r * 80u; i < r * 80u + 80u; i++)
			if (now[i] != shadow.cell[i]) {
				diff = 1;
				break;
			}
		if (cur != t_cur && (((cur >> 8) & 0xffu) == r || ((t_cur >> 8) & 0xffu) == r))
			diff = 1;
		if (diff) {
			for (i = r * 80u; i < r * 80u + 80u; i++)
				shadow.cell[i] = now[i];
			text_row(f, shadow.cell, r, cur);
			changed = 1;
		}
	}
	t_cur = cur;
	shadow.cursor = cur;
	if (changed)
		shadow.frames++;
}

#ifdef QDOS_R18
/* ---- mode 12h: INDEX8 surfaces presented 1:1 (R18 Apollo 14, docs/APOLLO14-PLANAR-VGA.md) ----
 * The surface (at most 512x256) is placed centred in 640x480 without scaling rows: GS_VIEW_VGA12 1:1,
 * GS_VIEW_VGA12W each pixel doubled horizontally (if 2w <= 640).  Colour: pixel & 15 through palette entries 0..15
 * (the DAC's first 16 registers; the attribute controller keeps the identity map of mode12).  The border is colour 0.
 * Each screen row is built in line[] and written plane by plane (put_line: write mode 0, map mask 1 << p). */
#define PHYS_G12G 4
static unsigned int g12_ox, g12_oy, g12_xs;

static void g12_layout(const struct gsurf *g)
{
	g12_xs = (eff_view == GS_VIEW_VGA12W && g->w * 2u <= 640u) ? 2u : 1u;
	g12_ox = (640u - g->w * g12_xs) / 2u;
	g12_oy = (480u - g->h) / 2u;
}

static void g12_dac(const struct gsurf *g)
{
	unsigned int i;

	outb(P_DAC_WI, 0);
	for (i = 0; i < 48u; i++)
		outb(P_DAC_D, (unsigned int) (g->pal[i] >> 2));
	st_pal++;
}

/* source row sy (0..h-1) to screen row g12_oy + sy, with the text cursor if it covers this row */
static void g12_row(const struct gsurf *g, unsigned int sy)
{
	const unsigned char *s = g->pix + sy * g->stride;
	unsigned char *o = line + g12_ox;
	unsigned int x, k;

	for (k = 0; k < 640u; k++)
		line[k] = 0;
	if (g12_xs == 1u)
		for (x = 0; x < g->w; x++)
			o[x] = (unsigned char) (s[x] & 15u);
	else
		for (x = 0; x < g->w; x++)
			o[2u * x] = o[2u * x + 1u] = (unsigned char) (s[x] & 15u);
	if (cur_vis && g->font != 0) {
		unsigned int cw = g->font->cw, ch = g->font->ch, cy = cur_row * ch, cx = cur_col * cw;
		if (sy >= cy + ch - 2u && sy < cy + ch)
			for (k = 0; k < cw && cx + k < g->w; k++) {
				if (g12_xs == 1u)
					o[cx + k] = (unsigned char) (g->ink & 15u);
				else
					o[2u * (cx + k)] = o[2u * (cx + k) + 1u] = (unsigned char) (g->ink & 15u);
			}
	}
	put_line(g12_oy + sy);
	st_rows++;
}

static void g12_cursor_state(const struct gsurf *g)
{
	cur_vis = g->cursor_on && g->font != 0 && g->tcol < g->cols && g->trow < g->rows;
	cur_col = g->tcol;
	cur_row = g->trow;
}

static void g12_cell_rows(const struct gsurf *g, unsigned int row)   /* redraw the rows of one text cell row */
{
	unsigned int ch = g->font != 0 ? g->font->ch : 0u, y;

	for (y = row * ch; y < row * ch + ch && y < g->h; y++)
		g12_row(g, y);
}

/* Work per pres_poll call is bounded (the call holds the BKL with interrupts off): at most G12_ROWS rows, border rows
 * first after a full redraw, then the dirty surface rows; what is left stays pending for the next tick (R18). */
#ifndef G12_ROWS
#define G12_ROWS 64u
#endif
static unsigned int g12_bord = 480u;    /* next screen row to check for border blanking; 480: border done */

static void g12_work(struct gsurf *g)
{
	unsigned int budget = G12_ROWS, irq, y;
	int y0, y1, r0, r1;

	while (g12_bord < 480u && budget > 0u) {
		if (g12_bord >= g12_oy && g12_bord < g12_oy + g->h) {
			g12_bord = g12_oy + g->h;
			continue;
		}
		blank_rows(g12_bord, g12_bord + 1u);
		g12_bord++;
		budget--;
	}
	irq = irq_save();
	y0 = g->dy0;
	y1 = g->dy1;
	g->dy0 = g->dy1 = 0;
	irq_restore(irq);
	if (y0 < 0)
		y0 = 0;
	if (y1 > (int) g->h)
		y1 = (int) g->h;
	if (y0 >= y1)
		return;
	r0 = y0 + (int) budget;
	r1 = y1;
	for (y = (unsigned int) y0; (int) y < y1 && (int) y < r0; y++)
		g12_row(g, y);
	if (r0 < r1) {                                   /* the rest: back into the dirty range for the next tick */
		irq = irq_save();
		if (g->dy0 < g->dy1) {
			if (r0 < g->dy0)
				g->dy0 = r0;
			if (r1 > g->dy1)
				g->dy1 = r1;
		} else {
			g->dy0 = r0;
			g->dy1 = r1;
		}
		irq_restore(irq);
	}
	shadow.frames++;
}

static void to_g12gfx(struct gsurf *g, int idx, unsigned int tag)
{
	unsigned int irq;

	if (phys != PHYS_G12 && phys != PHYS_G12G) {
		write_regs(&mode12);
		st_gfx++;
	}
	irq = irq_save();
	g->dy0 = 0;                                      /* the whole surface is drawn, in bounded steps */
	g->dy1 = (int) g->h;
	shown_pal_gen = g->pal_gen;
	irq_restore(irq);
	g12_dac(g);
	g12_layout(g);
	g12_cursor_state(g);
	g12_bord = 0;
	shown_g = g;
	shown_idx = idx;
	shown_tag = tag;
	shown_view = eff_view | (g->vx << 2) | (g->vy << 16);
	phys = PHYS_G12G;
	shadow.mode = 4;
	shadow.session = (unsigned int) idx;
	shadow.ox = g12_ox;
	shadow.oy = g12_oy;
	shadow.cell_w = g12_xs;
	st_full++;
	g12_work(g);
}

static void g12_update(struct gsurf *g)
{
	unsigned int irq, pgen, ocol = cur_col, orow = cur_row, ovis = cur_vis;

	irq = irq_save();
	pgen = g->pal_gen;
	irq_restore(irq);
	if (pgen != shown_pal_gen) {
		shown_pal_gen = pgen;
		g12_dac(g);
	}
	g12_cursor_state(g);
	g12_work(g);
	if (ovis != cur_vis || ocol != cur_col || orow != cur_row) {
		if (ovis)
			g12_cell_rows(g, orow);
		if (cur_vis && (!ovis || orow != cur_row))
			g12_cell_rows(g, cur_row);
	}
}
#endif

#ifdef QDOS_R18
/* R18: pres_poll runs in the SYSTEM job with the BKL held and interrupts off (kernel/shellboot.c): its cost is the
 * presenter's share of lock time.  Measured with the TSC around every call; read by SURF_OP_PRES_TIME. */
static unsigned int pt_calls, pt_busy, pt_max, pt_tot_lo, pt_tot_hi, pt_maxrows;
static unsigned int rdtsc_lo(void)
{
	unsigned int lo, hi;
	__asm__ volatile (".byte 0x0f, 0x31" : "=a" (lo), "=d" (hi));
	return lo;
}
void pres_time(unsigned int *o)
{
	o[0] = pt_calls; o[1] = pt_busy; o[2] = pt_max; o[3] = pt_tot_lo; o[4] = pt_tot_hi; o[5] = pt_maxrows;
	o[6] = st_full; o[7] = st_rows;
}
static void pres_poll_body(void);
void pres_poll(void)
{
	unsigned int t0, dt, f0 = st_full, r0 = st_rows;

	t0 = rdtsc_lo();
	pres_poll_body();
	dt = rdtsc_lo() - t0;
	pt_calls++;
	if (st_full != f0 || st_rows != r0) {           /* a call that drew something */
		pt_busy++;
		if (dt > pt_max)
			pt_max = dt;
		if (st_rows - r0 > pt_maxrows)
			pt_maxrows = st_rows - r0;
		pt_tot_lo += dt;
		if (pt_tot_lo < dt)
			pt_tot_hi++;
	}
}
static void pres_poll_body(void)
#else
void pres_poll(void)
#endif
{
	unsigned int irq;
	struct gsurf *g;
	int idx, y0, y1, recheck, over, gt;
	unsigned int tag, pgen, view;

	if (!saved)
		return;
	irq = irq_save();
	recheck = want_check;
	want_check = 0;
	over = vcon_overlay_on();
	idx = vcon_fg();
	g = over ? 0 : vsess_gfx(idx);
	gt = !over && g == 0 && vsess_cmode(idx) == VSESS_CM_GFX;
	tag = vsess_tag(idx);
	if (phys == PHYS_TEXT && g == 0 && !gt) {
		irq_restore(irq);
		return;                          /* text shown, text wanted: the console layer draws it directly */
	}
	if (phys == PHYS_TEXT)
		phys = PHYS_BUSY;                /* from now on no trap writes to the card */
	irq_restore(irq);
	if (g == 0 && !gt) {
		to_text();
		return;
	}
	if (gt) {
		if (phys != PHYS_G12 || recheck || idx != shown_idx || tag != shown_tag)
			to_gtext(idx, tag);
		else
			gtext_update(idx);
		return;
	}
	view = g->view | (g->vx << 2) | (g->vy << 16);
#ifdef QDOS_R18
	{
		unsigned int ov = vsess_view_ov(idx);
		eff_view = ov ? ov - 1u : g->view;
	}
	view = eff_view | (g->vx << 2) | (g->vy << 16);
	if (eff_view == GS_VIEW_VGA12 || eff_view == GS_VIEW_VGA12W) {
		if (phys != PHYS_G12G || recheck || g != shown_g || idx != shown_idx || tag != shown_tag || view != shown_view)
			to_g12gfx(g, idx, tag);
		else
			g12_update(g);
		return;
	}
#endif
	if (phys != PHYS_GFX || recheck || g != shown_g || idx != shown_idx || tag != shown_tag || view != shown_view) {
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
		int cy = (int) (cur_row * g->font->ch);  /* erase the old cursor: copy its cell row again */
		if (cy < (int) g->h)
			copy_rows(g, cy, cy + (int) g->font->ch > (int) g->h ? (int) g->h : cy + (int) g->font->ch);
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
	kev_refocus();
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
#ifdef QDOS_R18
void pres_time(unsigned int *o)
{
	unsigned int i;
	for (i = 0; i < 8u; i++)
		o[i] = 0;
}
#endif

static struct pres_shadow shadow = { 0x44485351u, 1u };

const struct pres_shadow *pres_shadow_ptr(void)
{
	return &shadow;
}
#endif /* QDOS_CON_VGA */
#else
typedef int vgapres_off_unit;        /* an empty translation unit is not ISO C */
#endif /* QDOS_SURFACES */
