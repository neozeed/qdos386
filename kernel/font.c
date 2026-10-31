/* QDOS/386 R13 - bitmap font renderer.  See font.h.  Freestanding C89, no display hardware. */
#include "font.h"

#ifdef QDOS_FONT_QDOS5X9
const char font_name[] = "QDOS5x9";

const unsigned char *font_glyph(unsigned int c)
{
	if (c < FONT_FIRST || c >= FONT_MISSING)
		c = FONT_MISSING;
	return font5x9[c - FONT_FIRST];
}
#else
const char font_name[] = "Minerva 1.98";

const unsigned char *font_glyph(unsigned int c)
{
	return font_minerva[c & 0xffu];
}
#endif

int font_draw_cell(unsigned char *buf, unsigned int w, unsigned int h, unsigned int stride, int x, int y,
                   unsigned int c, unsigned int ink, unsigned int paper, unsigned int attr)
{
	const unsigned char *g = font_glyph(c);
	int r, k, any = 0;

	if (attr & FONT_A_INVERSE) {
		unsigned int t = ink;
		ink = paper;
		paper = t;
	}
	for (r = 0; r < FONT_CELL_H; r++) {
		int py = y + r;
		unsigned int bits;

		if (py < 0 || py >= (int) h)
			continue;
		bits = r < FONT_GLYPH_H ? g[r] : 0u;
		if (attr & FONT_A_BOLD)
			bits |= bits >> 1;
		if ((attr & FONT_A_UNDER) && r == FONT_CELL_H - 1)
			bits = 0xfcu;                                 /* all six columns */
		for (k = 0; k < FONT_CELL_W; k++) {
			int px = x + k;
			int on = (bits & (0x80u >> k)) != 0;

			if (px < 0 || px >= (int) w)
				continue;
			any = 1;
			if (on)
				buf[(unsigned int) py * stride + (unsigned int) px] = (unsigned char) ink;
			else if (!(attr & FONT_A_NOPAPER))
				buf[(unsigned int) py * stride + (unsigned int) px] = (unsigned char) paper;
		}
	}
	return any;
}
