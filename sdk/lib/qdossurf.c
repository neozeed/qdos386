/* QDOS/386 R13 - libqdos: display sessions and INDEX8 surfaces (QT_SURFACE).  A separate member: programs that do
 * not call these link exactly as before.  Kernels without surfaces answer -1.  See docs/R13-DISPLAY-ABI.md. */
#include "qdos386.h"

#define QD_TRAP_SURFACE 0x3Bu

struct qd_rect_arg { long x, y, w, h; unsigned long p, q; };
struct qd_text_arg { long x, y; unsigned long ink, paper, attr, str, len; };
struct qd_cells_arg { unsigned long row, col, n, cells; };

static long st(unsigned int op, unsigned int b, unsigned int c)
{
	return (long) (int) qtrap(QD_TRAP_SURFACE, op, b, c);
}

static long rect(unsigned int op, long x, long y, long w, long h, unsigned long p, unsigned long q)
{
	struct qd_rect_arg r;

	r.x = x;
	r.y = y;
	r.w = w;
	r.h = h;
	r.p = p;
	r.q = q;
	return st(op, (unsigned int) &r, 0u);
}

int qd_surface_caps(void)
{
	return (int) st(0u, 0u, 0u);
}

int qd_surface_info(long id, struct qd_surface_info *out)
{
	return (int) st(1u, (unsigned int) id, (unsigned int) out);
}

long qd_surface_new(void)
{
	return st(2u, 0u, 0u);
}

long qd_gfx_open(unsigned int w, unsigned int h, unsigned int flags)
{
	if (w > 0xffffu || h > 0xffffu)
		return QD_SURF_ERR_ARG;
	return st(3u, w | (h << 16), flags);
}

int qd_gfx_close(void)
{
	return (int) st(4u, 0u, 0u);
}

long qd_gfx_fill(long x, long y, long w, long h, unsigned int colour)
{
	return rect(5u, x, y, w, h, (unsigned long) colour, 0ul);
}

long qd_gfx_blit(long x, long y, long w, long h, const void *src, unsigned long stride)
{
	return rect(6u, x, y, w, h, (unsigned long) src, stride);
}

long qd_gfx_read(long x, long y, long w, long h, void *dst, unsigned long stride)
{
	return rect(7u, x, y, w, h, (unsigned long) dst, stride);
}

long qd_gfx_scroll(long x, long y, long w, long h, long dy, unsigned int fill)
{
	return rect(13u, x, y, w, h, (unsigned long) dy, (unsigned long) fill);
}

int qd_gfx_palette(unsigned int first, unsigned int count, const unsigned char *rgb)
{
	if (first > 0xffffu || count > 0xffffu)
		return QD_SURF_ERR_ARG;
	return (int) st(8u, first | (count << 16), (unsigned int) rgb);
}

int qd_gfx_get_palette(unsigned int first, unsigned int count, unsigned char *rgb)
{
	if (first > 0xffffu || count > 0xffffu)
		return QD_SURF_ERR_ARG;
	return (int) st(9u, first | (count << 16), (unsigned int) rgb);
}

long qd_gfx_text(long x, long y, const char *s, unsigned int len, unsigned int ink, unsigned int paper, unsigned int attr)
{
	struct qd_text_arg t;

	t.x = x;
	t.y = y;
	t.ink = ink;
	t.paper = paper;
	t.attr = attr;
	t.str = (unsigned long) s;
	t.len = len;
	return st(10u, (unsigned int) &t, 0u);
}

int qd_gfx_print(const char *s, unsigned int len)
{
	return (int) st(11u, (unsigned int) s, len);
}

int qd_gfx_colours(unsigned int ink, unsigned int paper, unsigned int attr)
{
	return (int) st(19u, (ink & 0xffu) | ((paper & 0xffu) << 8) | ((attr & 0xffu) << 16), 0u);
}

int qd_gfx_cursor(unsigned int col, unsigned int row, int visible)
{
	if (col > 0xffu || row > 0xffu)
		return QD_SURF_ERR_ARG;
	return (int) st(12u, col | (row << 8) | ((visible ? 1u : 0u) << 16), 0u);
}

long qd_gfx_cursor_get(void)
{
	return st(12u, 0xFFFFFFFFu, 0u);
}

int qd_surface_foreground(long id)
{
	return (int) st(14u, (unsigned int) id, 0u);
}

int qd_surface_focus(void)
{
	return (int) st(15u, 0u, 0u);
}

long qd_gfx_checksum(void)
{
	return st(16u, 0u, 0u);
}

long qd_gfx_frame(void)
{
	return st(20u, 0u, 0u);
}

int qd_surface_list(void *buf, unsigned int bytes)
{
	return (int) st(17u, (unsigned int) buf, bytes);
}

int qd_text_cells(unsigned int row, unsigned int col, const unsigned short *cells, unsigned int n)
{
	struct qd_cells_arg a;

	a.row = row;
	a.col = col;
	a.n = n;
	a.cells = (unsigned long) cells;
	return (int) st(18u, (unsigned int) &a, 0u);
}

int qd_surface_stats(unsigned long *w8)
{
	return (int) st(21u, (unsigned int) w8, 0u);
}
