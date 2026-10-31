/* QDOS/386 R13 - INDEX8 surfaces.  See gsurf.h.  Freestanding C89 (no library calls). */
#include "gsurf.h"
#ifdef QDOS_R13A
#include "fontreg.h"
#define CW(g) ((int) (g)->font->cw)          /* R13A: the surface's font sets the text cell */
#define CH(g) ((int) (g)->font->ch)
#define DRAW_CELL(g, x, y, c, i, p, a) kfont_draw_cell((g)->font, (g)->pix, (g)->w, (g)->h, (g)->stride, x, y, c, i, p, a)
#else
#include "font.h"
#define CW(g) FONT_CELL_W
#define CH(g) FONT_CELL_H
#define DRAW_CELL(g, x, y, c, i, p, a) font_draw_cell((g)->pix, (g)->w, (g)->h, (g)->stride, x, y, c, i, p, a)
#endif

unsigned long gs_block_bytes(unsigned int w, unsigned int h)
{
	unsigned long stride = ((unsigned long) w + 3ul) & ~3ul;

	return (unsigned long) sizeof(struct gsurf) + stride * (unsigned long) h;
}

/* The default palette: 16 EGA colours, a 6x6x6 colour cube at 16..231, 24 greys at 232..255. */
void gs_default_palette(unsigned char *pal)
{
	static const unsigned char ega[16][3] = {
		{ 0, 0, 0 }, { 0, 0, 170 }, { 0, 170, 0 }, { 0, 170, 170 }, { 170, 0, 0 }, { 170, 0, 170 }, { 170, 85, 0 },
		{ 170, 170, 170 }, { 85, 85, 85 }, { 85, 85, 255 }, { 85, 255, 85 }, { 85, 255, 255 }, { 255, 85, 85 },
		{ 255, 85, 255 }, { 255, 255, 85 }, { 255, 255, 255 }
	};
	static const unsigned char lv[6] = { 0, 95, 135, 175, 215, 255 };
	unsigned int i;

	for (i = 0; i < 16; i++) {
		pal[i * 3] = ega[i][0];
		pal[i * 3 + 1] = ega[i][1];
		pal[i * 3 + 2] = ega[i][2];
	}
	for (i = 0; i < 216; i++) {
		pal[(16 + i) * 3] = lv[i / 36];
		pal[(16 + i) * 3 + 1] = lv[(i / 6) % 6];
		pal[(16 + i) * 3 + 2] = lv[i % 6];
	}
	for (i = 0; i < 24; i++) {
		unsigned char v = (unsigned char) (8 + i * 10);
		pal[(232 + i) * 3] = v;
		pal[(232 + i) * 3 + 1] = v;
		pal[(232 + i) * 3 + 2] = v;
	}
}

int gs_init(struct gsurf *g, void *block, unsigned int w, unsigned int h, unsigned int owner)
{
	unsigned long i, n;

	if (w == 0 || h == 0 || w > GS_MAX_W || h > GS_MAX_H)
		return GS_ERR_ARG;
	g->magic = GS_MAGIC;
	g->w = w;
	g->h = h;
	g->stride = (w + 3u) & ~3u;
	g->pix = (unsigned char *) block + sizeof(struct gsurf);
	n = (unsigned long) g->stride * h;
	for (i = 0; i < n; i++)
		g->pix[i] = 0;
	gs_default_palette(g->pal);
	g->pal_gen = 1;
	g->gen = 1;
	g->dy0 = 0;
	g->dy1 = (int) h;
#ifdef QDOS_R13A
	g->font = kfont_default(0);
	kfont_ref(g->font);
	g->view = GS_VIEW_FIT;
	g->vx = 0;
	g->vy = 0;
#endif
	g->cols = w / CW(g);
	g->rows = h / CH(g);
	g->tcol = 0;
	g->trow = 0;
	g->ink = 15;
	g->paper = 0;
	g->tattr = 0;
	g->cursor_on = 0;
	g->owner = owner;
	g->frames = 0;
	return GS_OK;
}
#ifdef QDOS_R13A

/* R13A: give the surface font f (the surface holds a reference): the text grid follows the font's cell. */
void gs_set_font(struct gsurf *g, struct kfont *f)
{
	kfont_ref(f);
	if (g->font != 0)
		kfont_unref(g->font);
	g->font = f;
	g->cols = g->w / f->cw;
	g->rows = g->h / f->ch;
	if (g->tcol > g->cols)
		g->tcol = 0;
	if (g->trow >= g->rows)
		g->trow = 0;
	gs_dirty(g, 0, (long) g->h);
}
#endif

void gs_dirty(struct gsurf *g, long y0, long y1)
{
	if (y0 < 0)
		y0 = 0;
	if (y1 > (long) g->h)
		y1 = (long) g->h;
	if (y0 >= y1)
		return;
	if (g->dy0 >= g->dy1) {
		g->dy0 = (int) y0;
		g->dy1 = (int) y1;
	} else {
		if (y0 < g->dy0)
			g->dy0 = (int) y0;
		if (y1 > g->dy1)
			g->dy1 = (int) y1;
	}
	g->gen++;
}

/* Clip (x, y, w, h) to the surface.  On return *x0, *y0 are the first visible pixel, *cw, *ch the visible size and
 * *sx, *sy the offset of that pixel inside the requested rectangle.  Returns 1 if anything is visible, 0 if not,
 * GS_ERR_ARG for a negative size.  The arithmetic stays inside long: coordinates are first clamped to
 * [-2^30, 2^30] so x + w cannot overflow. */
int gs_clip(const struct gsurf *g, long x, long y, long w, long h, long *x0, long *y0, long *cw, long *ch,
                long *sx, long *sy)
{
	long x1, y1;

	if (w < 0 || h < 0)
		return GS_ERR_ARG;
	if (w == 0 || h == 0)
		return 0;
	if (x < -0x40000000L || x > 0x40000000L || y < -0x40000000L || y > 0x40000000L)
		return 0;
	if (w > 0x40000000L)
		w = 0x40000000L;
	if (h > 0x40000000L)
		h = 0x40000000L;
	x1 = x + w;
	y1 = y + h;
	*sx = x < 0 ? -x : 0;
	*sy = y < 0 ? -y : 0;
	if (x < 0)
		x = 0;
	if (y < 0)
		y = 0;
	if (x1 > (long) g->w)
		x1 = (long) g->w;
	if (y1 > (long) g->h)
		y1 = (long) g->h;
	if (x >= x1 || y >= y1)
		return 0;
	*x0 = x;
	*y0 = y;
	*cw = x1 - x;
	*ch = y1 - y;
	return 1;
}

int gs_fill(struct gsurf *g, long x, long y, long w, long h, unsigned int colour)
{
	long x0, y0, cw, ch, sx, sy, r, c;
	int v = gs_clip(g, x, y, w, h, &x0, &y0, &cw, &ch, &sx, &sy);

	if (v <= 0)
		return v;
	for (r = 0; r < ch; r++) {
		unsigned char *p = g->pix + (unsigned long) (y0 + r) * g->stride + (unsigned long) x0;
		for (c = 0; c < cw; c++)
			p[c] = (unsigned char) colour;
	}
	gs_dirty(g, y0, y0 + ch);
	return (int) (cw * ch);
}

long gs_blit(struct gsurf *g, long x, long y, long w, long h, const unsigned char *src, unsigned long sstride)
{
	long x0, y0, cw, ch, sx, sy, r, c;
	int v = gs_clip(g, x, y, w, h, &x0, &y0, &cw, &ch, &sx, &sy);

	if (v <= 0)
		return v;
	for (r = 0; r < ch; r++) {
		const unsigned char *s = src + (unsigned long) (sy + r) * sstride + (unsigned long) sx;
		unsigned char *p = g->pix + (unsigned long) (y0 + r) * g->stride + (unsigned long) x0;
		for (c = 0; c < cw; c++)
			p[c] = s[c];
	}
	gs_dirty(g, y0, y0 + ch);
	return cw * ch;
}

#ifdef QDOS_R13A
/* R13A: gs_blit, but source pixels equal to `key` are transparent (the surface keeps its pixel). */
long gs_blit_key(struct gsurf *g, long x, long y, long w, long h, const unsigned char *src, unsigned long sstride,
                 unsigned int key)
{
	long x0, y0, cw, ch, sx, sy, r, c, n = 0;
	int v = gs_clip(g, x, y, w, h, &x0, &y0, &cw, &ch, &sx, &sy);

	if (v <= 0)
		return v;
	for (r = 0; r < ch; r++) {
		const unsigned char *s = src + (unsigned long) (sy + r) * sstride + (unsigned long) sx;
		unsigned char *p = g->pix + (unsigned long) (y0 + r) * g->stride + (unsigned long) x0;
		for (c = 0; c < cw; c++)
			if (s[c] != (unsigned char) key) {
				p[c] = s[c];
				n++;
			}
	}
	gs_dirty(g, y0, y0 + ch);
	return n;
}

/* R13A: copy the rectangle (sx, sy, w, h) of the surface to (dx, dy), as memmove (overlap is safe).  Both rectangles
 * are clipped to the surface; only the part whose source and destination are both inside is copied. */
long gs_copy(struct gsurf *g, long sx, long sy, long w, long h, long dx, long dy)
{
	long a0, b0, cw, ch, ox, oy, r, c;
	int v;

	if (w < 0 || h < 0)
		return GS_ERR_ARG;
	v = gs_clip(g, sx, sy, w, h, &a0, &b0, &cw, &ch, &ox, &oy);      /* the visible source */
	if (v <= 0)
		return v;
	dx += ox;
	dy += oy;
	v = gs_clip(g, dx, dy, cw, ch, &dx, &dy, &cw, &ch, &ox, &oy);     /* ... and its visible destination */
	if (v <= 0)
		return v;
	a0 += ox;
	b0 += oy;
	if (dy > b0) {
		for (r = ch - 1; r >= 0; r--) {
			unsigned char *d = g->pix + (unsigned long) (dy + r) * g->stride + (unsigned long) dx;
			const unsigned char *s = g->pix + (unsigned long) (b0 + r) * g->stride + (unsigned long) a0;
			if (dx > a0)
				for (c = cw - 1; c >= 0; c--)
					d[c] = s[c];
			else
				for (c = 0; c < cw; c++)
					d[c] = s[c];
		}
	} else {
		for (r = 0; r < ch; r++) {
			unsigned char *d = g->pix + (unsigned long) (dy + r) * g->stride + (unsigned long) dx;
			const unsigned char *s = g->pix + (unsigned long) (b0 + r) * g->stride + (unsigned long) a0;
			if (dx > a0 && dy == b0)
				for (c = cw - 1; c >= 0; c--)
					d[c] = s[c];
			else
				for (c = 0; c < cw; c++)
					d[c] = s[c];
		}
	}
	gs_dirty(g, dy, dy + ch);
	return cw * ch;
}
#endif

long gs_read(const struct gsurf *g, long x, long y, long w, long h, unsigned char *dst, unsigned long dstride)
{
	long x0, y0, cw, ch, sx, sy, r, c;
	int v = gs_clip(g, x, y, w, h, &x0, &y0, &cw, &ch, &sx, &sy);

	if (v <= 0)
		return v;
	for (r = 0; r < ch; r++) {
		const unsigned char *p = g->pix + (unsigned long) (y0 + r) * g->stride + (unsigned long) x0;
		unsigned char *d = dst + (unsigned long) (sy + r) * dstride + (unsigned long) sx;
		for (c = 0; c < cw; c++)
			d[c] = p[c];
	}
	return cw * ch;
}

int gs_scroll(struct gsurf *g, long x, long y, long w, long h, long dy, unsigned int fill)
{
	long x0, y0, cw, ch, sx, sy, r, c;
	int v = gs_clip(g, x, y, w, h, &x0, &y0, &cw, &ch, &sx, &sy);

	if (v <= 0)
		return v;
	if (dy > ch)
		dy = ch;
	if (dy < -ch)
		dy = -ch;
	if (dy > 0) {                                  /* content moves up */
		for (r = 0; r < ch - dy; r++) {
			unsigned char *d = g->pix + (unsigned long) (y0 + r) * g->stride + (unsigned long) x0;
			const unsigned char *s = d + (unsigned long) dy * g->stride;
			for (c = 0; c < cw; c++)
				d[c] = s[c];
		}
		for (r = ch - dy; r < ch; r++) {
			unsigned char *d = g->pix + (unsigned long) (y0 + r) * g->stride + (unsigned long) x0;
			for (c = 0; c < cw; c++)
				d[c] = (unsigned char) fill;
		}
	} else if (dy < 0) {                           /* content moves down */
		long n = -dy;
		for (r = ch - 1; r >= n; r--) {
			unsigned char *d = g->pix + (unsigned long) (y0 + r) * g->stride + (unsigned long) x0;
			const unsigned char *s = d - (unsigned long) n * g->stride;
			for (c = 0; c < cw; c++)
				d[c] = s[c];
		}
		for (r = 0; r < n; r++) {
			unsigned char *d = g->pix + (unsigned long) (y0 + r) * g->stride + (unsigned long) x0;
			for (c = 0; c < cw; c++)
				d[c] = (unsigned char) fill;
		}
	}
	gs_dirty(g, y0, y0 + ch);
	return (int) (cw * ch);
}

int gs_set_palette(struct gsurf *g, unsigned int first, unsigned int count, const unsigned char *rgb)
{
	unsigned int i;

	if (count == 0 || first > 255u || count > 256u - first)
		return GS_ERR_ARG;
	for (i = 0; i < count * 3u; i++)
		g->pal[first * 3u + i] = rgb[i];
	g->pal_gen++;
	return (int) count;
}

int gs_get_palette(const struct gsurf *g, unsigned int first, unsigned int count, unsigned char *rgb)
{
	unsigned int i;

	if (count == 0 || first > 255u || count > 256u - first)
		return GS_ERR_ARG;
	for (i = 0; i < count * 3u; i++)
		rgb[i] = g->pal[first * 3u + i];
	return (int) count;
}

int gs_text(struct gsurf *g, long x, long y, const unsigned char *s, unsigned int n, unsigned int ink,
            unsigned int paper, unsigned int attr)
{
	unsigned int i;
	int drawn = 0;

	if (y < -CH(g) || y >= (long) g->h || x >= (long) g->w || x < -0x40000000L)
		return 0;
	for (i = 0; i < n; i++) {
		long cx = x + (long) i * CW(g);

		if (cx >= (long) g->w)
			break;
		if (cx + CW(g) <= 0)
			continue;
		drawn += DRAW_CELL(g, (int) cx, (int) y, s[i], ink, paper, attr);
	}
	if (drawn)
		gs_dirty(g, y, y + CH(g));
	return drawn;
}

void gs_text_colours(struct gsurf *g, unsigned int ink, unsigned int paper, unsigned int attr)
{
	g->ink = ink & 0xffu;
	g->paper = paper & 0xffu;
	g->tattr = attr & (FONT_A_UNDER | FONT_A_INVERSE | FONT_A_BOLD | FONT_A_NOPAPER);
}

static void text_newline(struct gsurf *g)
{
	g->tcol = 0;
	if (g->trow + 1 < g->rows) {
		g->trow++;
		return;
	}
	(void) gs_scroll(g, 0, 0, (long) g->w, (long) (g->rows * CH(g)), CH(g), g->paper);
}

void gs_print(struct gsurf *g, const unsigned char *s, unsigned int n)
{
	unsigned int i;

	if (g->cols == 0 || g->rows == 0)
		return;
	for (i = 0; i < n; i++) {
		unsigned char c = s[i];

		switch (c) {
		case '\r':
			g->tcol = 0;
			break;
		case '\n':
			text_newline(g);
			break;
		case '\b':
			if (g->tcol > 0)
				g->tcol--;
			break;
		case '\t':
			g->tcol = (g->tcol / 8u + 1u) * 8u;
			if (g->tcol >= g->cols)
				text_newline(g);
			break;
		default:
			if (c < 32)
				break;
			if (g->tcol >= g->cols)
				text_newline(g);
			(void) DRAW_CELL(g, (int) (g->tcol * CW(g)),
			                      (int) (g->trow * CH(g)), c, g->ink, g->paper, g->tattr);
			gs_dirty(g, (long) (g->trow * CH(g)), (long) ((g->trow + 1u) * CH(g)));
			g->tcol++;
			break;
		}
	}
	gs_dirty(g, (long) (g->trow * CH(g)), (long) ((g->trow + 1u) * CH(g)));   /* the cursor's row */
}

int gs_cursor(struct gsurf *g, unsigned int col, unsigned int row, int visible)
{
	long oldy = (long) (g->trow * CH(g));

	if (col > g->cols || row >= g->rows)
		return GS_ERR_ARG;
	g->tcol = col;
	g->trow = row;
	g->cursor_on = visible != 0;
	gs_dirty(g, oldy, oldy + CH(g));
	gs_dirty(g, (long) (row * CH(g)), (long) ((row + 1u) * CH(g)));
	return GS_OK;
}

unsigned int gs_checksum(const struct gsurf *g)
{
	unsigned int h = 2166136261u, r, c;

	for (r = 0; r < g->h; r++)
		for (c = 0; c < g->w; c++) {
			h ^= g->pix[r * g->stride + c];
			h *= 16777619u;
		}
	for (r = 0; r < 768u; r++) {
		h ^= g->pal[r];
		h *= 16777619u;
	}
	return h;
}
