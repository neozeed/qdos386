/* QDOS/386 R13A - libqdos: fonts, rendered text consoles, logical surface views, native key events, transparent blit
 * and copy (QT_SURFACE ops 22..36).  A separate member: programs that do not call these link as before.  Kernels
 * without R13A answer -4 (bad op) or -1; test (qd_surface_caps() & QD_SURF_CAP_R13A) first.
 * See docs/R13A-GRAPHICS-ABI.md. */
#include "qdos386.h"

#define QD_TRAP_SURFACE 0x3Bu

struct qd_rect_arg2 { long x, y, w, h; unsigned long p, q; };
struct qd_copy_arg { long sx, sy, w, h, dx, dy; };

static long st(unsigned int op, unsigned int b, unsigned int c)
{
	return (long) (int) qtrap(QD_TRAP_SURFACE, op, b, c);
}

long qd_font_open(const char *path)              { return st(36u, (unsigned int) path, 0u); }
long qd_font_load(const void *image, unsigned int bytes) { return st(22u, (unsigned int) image, bytes); }
int qd_font_release(long id)                     { return (int) st(23u, (unsigned int) id, 0u); }
int qd_font_select(long id)                      { return (int) st(24u, (unsigned int) id, 0u); }
int qd_font_info(long id, struct qd_font_info *out) { return (int) st(25u, (unsigned int) id, (unsigned int) out); }
int qd_font_default(int console, long id)        { return (int) st(26u, console ? 1u : 0u, (unsigned int) id); }
int qd_font_list(long *ids, unsigned int max)    { return (int) st(27u, (unsigned int) ids, max); }
int qd_console_mode(unsigned int mode, int all)  { return (int) st(28u, mode, all ? 1u : 0u); }
int qd_console_mode_get(void)                    { return (int) st(28u, 0xFFFFFFFFu, 0u); }
long qd_text_checksum(long id)                   { return st(29u, (unsigned int) id, 0u); }
int qd_gfx_view(unsigned int mode, unsigned int vx, unsigned int vy)
{
	return (int) st(30u, mode, (vx & 0xffffu) | (vy << 16));
}
int qd_key_events(int on)                        { return (int) st(31u, on ? 1u : 0u, 0u); }
int qd_key_read(unsigned long *ev, unsigned int max) { return (int) st(32u, (unsigned int) ev, max); }

long qd_gfx_blit_key(long x, long y, long w, long h, const void *src, unsigned long stride, unsigned int key)
{
	struct qd_rect_arg2 r;

	r.x = x;
	r.y = y;
	r.w = w;
	r.h = h;
	r.p = (unsigned long) src;
	r.q = stride;
	return st(33u, (unsigned int) &r, key);
}

long qd_gfx_copy(long sx, long sy, long w, long h, long dx, long dy)
{
	struct qd_copy_arg a;

	a.sx = sx;
	a.sy = sy;
	a.w = w;
	a.h = h;
	a.dx = dx;
	a.dy = dy;
	return st(34u, (unsigned int) &a, 0u);
}

int qd_gfx_memstats(unsigned long *w8)           { return (int) st(35u, (unsigned int) w8, 0u); }
