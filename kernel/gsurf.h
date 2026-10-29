/*
 * QDOS/386 R13 - INDEX8 surfaces: an 8-bit indexed framebuffer with a 256-entry RGB palette, dirty rows and a
 * QL-style text stream drawn with the bitmap font (kernel/font.h).  Pure memory operations: no display hardware, no
 * user pointers (kernel/surface.c checks those), no locking (callers run with interrupts off, as traps do).
 * See docs/R13-DISPLAY-ABI.md.  Freestanding C89; also built on the host by tests/r13/host-surf.c.
 */
#ifndef QDOS386_GSURF_H
#define QDOS386_GSURF_H

#define GS_MAX_W      320u
#define GS_MAX_H      200u
#define GS_MAGIC      0x46525347u          /* "GSRF" */

#define GS_OK         0
#define GS_ERR_ARG    (-4)                 /* negative width/height, bad dimensions, bad palette range */
#define GS_ERR_MEM    (-3)

/* Clipping rule (every drawing call): the rectangle (x, y, w, h) is clipped to the surface; w or h < 0 is an error
 * (GS_ERR_ARG); a rectangle wholly outside, or w or h == 0, draws nothing and returns 0.  The return value of a
 * drawing call is the number of pixels changed or read.  Coordinates are signed 32-bit; x + w and y + h are
 * computed without overflow (long arithmetic on a clamped range). */

struct gsurf {
	unsigned int magic;
	unsigned int w, h, stride;             /* stride = w rounded up to 4 */
	unsigned char *pix;                    /* stride * h bytes */
	unsigned char pal[768];                /* 256 x (R, G, B), 0..255 each */
	unsigned int pal_gen;                  /* changes on every palette write */
	unsigned int gen;                      /* changes on every pixel write */
	int dy0, dy1;                          /* dirty rows [dy0, dy1); dy0 >= dy1: nothing pending */
	/* the text stream (gs_print): cell grid of FONT_CELL_W x FONT_CELL_H */
	unsigned int cols, rows;
	unsigned int tcol, trow;               /* text cursor (cell) */
	unsigned int ink, paper, tattr;        /* colours and FONT_A_* attributes of gs_print */
	int cursor_on;                         /* non-destructive cursor: drawn by the presenter, never into pix */
	unsigned int owner;                    /* tagged job id that opened it */
	unsigned int frames;                   /* gs_frame_mark count (programs may report frames) */
};

/* Size of the single block that holds the header, the pixels and nothing else (pix points inside it). */
unsigned long gs_block_bytes(unsigned int w, unsigned int h);
/* Initialise a surface in a block of gs_block_bytes(w, h) bytes: pixels 0, the default palette, text at (0,0). */
int gs_init(struct gsurf *g, void *block, unsigned int w, unsigned int h, unsigned int owner);
void gs_default_palette(unsigned char *pal);

int gs_fill(struct gsurf *g, long x, long y, long w, long h, unsigned int colour);
/* Copy a w x h rectangle from src (row pitch sstride) to (x, y); clipped (only the visible part of src is read). */
long gs_blit(struct gsurf *g, long x, long y, long w, long h, const unsigned char *src, unsigned long sstride);
long gs_read(const struct gsurf *g, long x, long y, long w, long h, unsigned char *dst, unsigned long dstride);
int gs_scroll(struct gsurf *g, long x, long y, long w, long h, long dy, unsigned int fill);
int gs_set_palette(struct gsurf *g, unsigned int first, unsigned int count, const unsigned char *rgb);
int gs_get_palette(const struct gsurf *g, unsigned int first, unsigned int count, unsigned char *rgb);
/* Draw n bytes of text with their full cells at pixel (x, y) -> cells drawn (partly visible ones included). */
int gs_text(struct gsurf *g, long x, long y, const unsigned char *s, unsigned int n, unsigned int ink,
            unsigned int paper, unsigned int attr);
/* The text stream: CR, LF (scrolls the whole surface by one cell row at the bottom), BS, TAB (to a multiple of 8),
 * other control bytes ignored; wraps at the right edge. */
void gs_print(struct gsurf *g, const unsigned char *s, unsigned int n);
void gs_text_colours(struct gsurf *g, unsigned int ink, unsigned int paper, unsigned int attr);
int gs_cursor(struct gsurf *g, unsigned int col, unsigned int row, int visible);
/* FNV-1a over the visible pixels (w bytes per row, stride padding excluded), then the 768 palette bytes. */
unsigned int gs_checksum(const struct gsurf *g);
void gs_dirty(struct gsurf *g, long y0, long y1);
/* The clipping rule above: 1 and the visible rectangle (*x0, *y0, *cw, *ch) with its offset (*sx, *sy) inside the
 * requested one; 0 if nothing is visible; GS_ERR_ARG for a negative size. */
int gs_clip(const struct gsurf *g, long x, long y, long w, long h, long *x0, long *y0, long *cw, long *ch,
            long *sx, long *sy);

#endif
