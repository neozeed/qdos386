/* R13 demos: a program-side model of its own 320x200 INDEX8 surface.  Every drawing call the demo makes through
 * libqdos is also applied to this copy; at the end the kernel's surface (qd_gfx_read, qd_gfx_get_palette and
 * qd_gfx_checksum) must equal the model exactly - whatever was shown, switched or hidden meanwhile.  Strict C89. */
#ifndef R13_GFXMODEL_H
#define R13_GFXMODEL_H
#include <string.h>
#include "qdos386.h"

#define GM_W 320
#define GM_H 200
static unsigned char gm_pix[GM_W * GM_H], gm_pal[768], gm_back[GM_W * GM_H], gm_bpal[768];

static void gm_fill(long x, long y, long w, long h, unsigned int c)
{
	long i, j;

	qd_gfx_fill(x, y, w, h, c);
	for (j = y < 0 ? 0 : y; j < y + h && j < GM_H; j++)
		for (i = x < 0 ? 0 : x; i < x + w && i < GM_W; i++)
			gm_pix[j * GM_W + i] = (unsigned char) c;
}

static void gm_blit(long x, long y, long w, long h, const unsigned char *src, unsigned long stride)
{
	long i, j;

	qd_gfx_blit(x, y, w, h, src, stride);
	for (j = 0; j < h; j++)
		for (i = 0; i < w; i++)
			if (x + i >= 0 && x + i < GM_W && y + j >= 0 && y + j < GM_H)
				gm_pix[(y + j) * GM_W + x + i] = src[j * (long) stride + i];
}

static void gm_palette(unsigned int first, unsigned int count, const unsigned char *rgb)
{
	qd_gfx_palette(first, count, rgb);
	memcpy(gm_pal + 3 * first, rgb, 3 * count);
}

/* text is drawn by the kernel's font; the model takes the drawn area back once (a static title) */
static void gm_text_static(long x, long y, const char *s, unsigned int ink, unsigned int paper)
{
	long n = (long) strlen(s), j;

	qd_gfx_text(x, y, s, (unsigned int) n, ink, paper, 0);
	for (j = 0; j < 10; j++)
		qd_gfx_read(x, y + j, n * 6, 1, gm_pix + (y + j) * GM_W + x, (unsigned long) (n * 6));
}

/* a 32-bit frame counter as 32 small squares (lit 15 / dark 8) at (x, y): drawn with fills only */
static void gm_counter(long x, long y, unsigned long v)
{
	int b;

	for (b = 0; b < 32; b++)
		gm_fill(x + (31 - b) * 5, y, 4, 4, (v >> b) & 1u ? 15u : 8u);
}

static unsigned long gm_sum(void)
{
	unsigned long h = 2166136261UL;
	unsigned int i;

	for (i = 0; i < GM_W * GM_H; i++)
		h = ((h ^ gm_pix[i]) * 16777619UL) & 0xffffffffUL;
	for (i = 0; i < 768; i++)
		h = ((h ^ gm_pal[i]) * 16777619UL) & 0xffffffffUL;
	return h & 0x7fffffffUL;
}

/* 1 if the kernel's surface equals the model: pixels read back, palette read back, checksum */
static int gm_verify(long *ksum, unsigned long *msum)
{
	int ok;

	memset(gm_back, 0xEE, sizeof gm_back);
	ok = qd_gfx_read(0, 0, GM_W, GM_H, gm_back, GM_W) == (long) GM_W * GM_H && memcmp(gm_back, gm_pix, sizeof gm_pix) == 0;
	ok = ok && qd_gfx_get_palette(0, 256, gm_bpal) == 256 && memcmp(gm_bpal, gm_pal, 768) == 0;
	*ksum = qd_gfx_checksum();
	*msum = gm_sum();
	return ok && (unsigned long) *ksum == *msum;
}

#endif
