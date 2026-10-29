/* QDOS/386 R13 - the QT_SURFACE kernel call.  See surface.h.  Empty unless QDOS_SURFACES.  Freestanding C89.
 * Runs inside traps (interrupts off), except surface_poll (the SYSTEM job). */
#ifdef QDOS_SURFACES
#include "surface.h"
#include "gsurf.h"
#include "font.h"
#include "vcon.h"
#include "jobs.h"
#include "channel.h"
#include "memory.h"
#include "present.h"

extern unsigned int irq_save(void);
extern void irq_restore(unsigned int flags);

#define ZOMBIE_MAX (SURF_GFX_MAX + 4)

static struct gsurf *zombies[ZOMBIE_MAX];
static unsigned int n_open, n_opened, n_refused, n_freed;

static unsigned int sid(int idx)
{
	return (vsess_tag(idx) << 8) | (unsigned int) idx;
}

static int my_sess(void)
{
	return vcon_job_con(job_current);
}

static unsigned int my_id(void)
{
	return jobs[job_current].id;
}

/* Resolve a surface id: 0 = the caller's session.  -> session index or SURF_ERR_STALE. */
static int resolve(unsigned int id)
{
	int idx;

	if (id == 0)
		return my_sess();
	idx = (int) (id & 0xffu);
	if (!vsess_valid(idx) || vsess_tag(idx) != (id >> 8))
		return SURF_ERR_STALE;
	return idx;
}

void surf_release(struct gsurf *g)
{
	int i;

	if (g == 0)
		return;
	g->magic = 0;
	if (n_open > 0)
		n_open--;
	for (i = 0; i < ZOMBIE_MAX; i++)
		if (zombies[i] == 0) {
			zombies[i] = g;
			return;
		}
	(void) mem_free(g, VSESS_OWNER);          /* not reached: at most SURF_GFX_MAX surfaces exist */
	n_freed++;
}

void surf_job_ended(unsigned int job_id, int idx)
{
	struct gsurf *g = vsess_gfx(idx);

	if (g != 0 && g->owner == job_id) {
		vsess_set_gfx(idx, 0);
		surf_release(g);
	}
}

/* The SYSTEM job: free released surfaces.  The presenter runs in the same job, so it never copies from one of
 * these while it is freed. */
void surface_poll(void)
{
	int i;

	for (i = 0; i < ZOMBIE_MAX; i++) {
		unsigned int irq = irq_save();
		struct gsurf *g = zombies[i];

		zombies[i] = 0;
		if (g != 0) {
			(void) mem_free(g, VSESS_OWNER);
			n_freed++;
		}
		irq_restore(irq);
	}
}

static int copy_in(void *dst, unsigned int src, unsigned int n)
{
	unsigned int i;

	if (!chan_user_range_ok(src, n))
		return SURF_ERR_RANGE;
	for (i = 0; i < n; i++)
		((unsigned char *) dst)[i] = ((const unsigned char *) src)[i];
	return 0;
}

static struct gsurf *my_gfx(int *err)
{
	struct gsurf *g = vsess_gfx(my_sess());

	*err = g != 0 ? 0 : SURF_ERR_NOGFX;
	return g;
}

/* Check that the part of a user buffer (pitch q) that the clipped rectangle reads or writes is the caller's memory. */
static int user_rect_ok(const struct gsurf *g, const struct surf_rect *r)
{
	long x0, y0, cw, ch, sx, sy;
	unsigned long start, len;
	int v = gs_clip(g, r->x, r->y, r->w, r->h, &x0, &y0, &cw, &ch, &sx, &sy);

	if (v < 0)
		return SURF_ERR_ARG;
	if (v == 0)
		return 0;
	if (r->q < (unsigned long) cw || r->q > 65536ul)
		return SURF_ERR_ARG;                       /* the pitch must cover the visible width */
	if ((unsigned long) sy > (0xFFFFFFFFul - (unsigned long) sx) / r->q)
		return SURF_ERR_RANGE;
	start = r->p + (unsigned long) sy * r->q + (unsigned long) sx;
	if (start < r->p)
		return SURF_ERR_RANGE;
	len = (unsigned long) (ch - 1) * r->q + (unsigned long) cw;
	if (!chan_user_range_ok((unsigned int) start, (unsigned int) len))
		return SURF_ERR_RANGE;
	return 1;
}

static int info(unsigned int id, unsigned int buf)
{
	unsigned int w[SURF_INFO_WORDS], i;
	int idx = resolve(id);
	struct gsurf *g;

	if (idx < 0)
		return idx;
	if (!chan_user_range_ok(buf, SURF_INFO_WORDS * 4u))
		return SURF_ERR_RANGE;
	for (i = 0; i < SURF_INFO_WORDS; i++)
		w[i] = 0;
	g = vsess_gfx(idx);
	w[0] = SURF_VERSION;
	w[1] = sid(idx);
	if (g != 0) {
		w[2] = SURF_KIND_INDEX8;
		w[3] = g->w;
		w[4] = g->h;
		w[5] = g->stride;
		w[6] = g->cols;
		w[7] = g->rows;
		w[8] = g->tcol | (g->trow << 8) | ((unsigned int) (g->cursor_on ? 1 : 0) << 16);
		w[11] = g->owner;
		w[12] = g->pal_gen;
		w[13] = g->gen;
		w[14] = g->frames;
		if (g->owner == my_id())
			w[9] |= SURF_FL_OWNER;
	} else {
		w[2] = SURF_KIND_TEXT;
		w[3] = VCON_COLS;
		w[4] = VCON_ROWS;
		w[5] = VCON_COLS * 2u;
		w[6] = VCON_COLS;
		w[7] = VCON_ROWS;
		w[8] = (unsigned int) vcon_query(idx, VQ_CURSOR);
		w[11] = vsess_owner(idx);
	}
	if (vcon_fg() == idx)
		w[9] |= SURF_FL_SHOWN;
	if (vsess_dynamic(idx))
		w[9] |= SURF_FL_PRIVATE;
	if (vcon_overlay_on())
		w[9] |= SURF_FL_OVERLAY;
	w[10] = (unsigned int) vsess_users(idx);
	w[15] = (unsigned int) idx;
	for (i = 0; i < SURF_INFO_WORDS; i++)
		((unsigned int *) buf)[i] = w[i];
	return 0;
}

static int list(unsigned int buf, unsigned int bytes)
{
	unsigned int room, n = 0, k;
	int i;
	unsigned char *p = (unsigned char *) buf;

	if (bytes < 16u || !chan_user_range_ok(buf, bytes))
		return SURF_ERR_RANGE;
	room = (bytes - 16u) / SURF_LIST_ENTRY;
	for (i = 0; i < VSESS_MAX && n < room; i++) {
		unsigned int *e;
		struct gsurf *g;

		if (!vsess_valid(i))
			continue;
		e = (unsigned int *) (p + 16u + n * SURF_LIST_ENTRY);
		for (k = 0; k < SURF_LIST_ENTRY / 4u; k++)
			e[k] = 0;
		g = vsess_gfx(i);
		e[0] = sid(i);
		e[1] = (unsigned int) i;
		e[2] = g ? SURF_KIND_INDEX8 : SURF_KIND_TEXT;
		e[3] = g ? g->w : VCON_COLS;
		e[4] = g ? g->h : VCON_ROWS;
		e[5] = (unsigned int) vsess_users(i);
		e[6] = g ? g->owner : vsess_owner(i);
		e[7] = (vcon_fg() == i ? SURF_FL_SHOWN : 0u) | (vsess_dynamic(i) ? SURF_FL_PRIVATE : 0u);
		n++;
	}
	((unsigned int *) buf)[0] = SURF_VERSION;
	((unsigned int *) buf)[1] = SURF_LIST_ENTRY;
	((unsigned int *) buf)[2] = n;
	((unsigned int *) buf)[3] = VSESS_MAX;
	return (int) n;
}

static int gfx_open(unsigned int b, unsigned int c)
{
	unsigned int w = b & 0xffffu, h = b >> 16;
	int sess = my_sess(), n;
	void *blk;
	struct gsurf *g;

	if (w == 0 && h == 0) {
		w = GS_MAX_W;
		h = GS_MAX_H;
	}
	if (w == 0 || h == 0 || w > GS_MAX_W || h > GS_MAX_H || (c & ~(SURF_GF_FOREGROUND | SURF_GF_SHARE)) != 0)
		return SURF_ERR_ARG;
	if (vsess_gfx(sess) != 0)
		return SURF_ERR_BUSY;
	if (n_open >= SURF_GFX_MAX) {
		n_refused++;
		return SURF_ERR_FULL;
	}
	blk = mem_alloc((unsigned int) gs_block_bytes(w, h), VSESS_OWNER);
	if (mem_is_error((unsigned int) blk)) {
		n_refused++;
		return SURF_ERR_MEM;
	}
	if (!(c & SURF_GF_SHARE) && (!vsess_dynamic(sess) || vsess_users(sess) > 1)) {
		n = vsess_new(my_id());                    /* a private session first: the caller's console is shared */
		if (n < 0) {
			(void) mem_free(blk, VSESS_OWNER);
			n_refused++;
			return n;
		}
		(void) vsess_move(job_current, n);
		sess = n;
	}
	g = (struct gsurf *) blk;
	(void) gs_init(g, blk, w, h, my_id());
	vsess_set_gfx(sess, g);
	n_open++;
	n_opened++;
	if (c & SURF_GF_FOREGROUND)
		(void) vcon_switch(sess);
	return (int) sid(sess);
}

int surface_trap(unsigned int op, unsigned int b, unsigned int c)
{
	struct surf_rect r;
	struct surf_text t;
	struct surf_cells ce;
	struct gsurf *g;
	int rc, sess;

	switch (op) {
	case SURF_OP_CAPS: {
		unsigned int pc = pres_caps(), caps = SURF_CAP_TEXT | SURF_CAP_INDEX8;

		if (pc & PRES_CAP_TEXT)
			caps |= SURF_CAP_SHOW_TEXT;
		if (pc & PRES_CAP_MODE13)
			caps |= SURF_CAP_SHOW_GFX;
		return (int) ((SURF_VERSION << 16) | caps);
	}
	case SURF_OP_INFO:
		return info(b, c);
	case SURF_OP_NEW:
		sess = my_sess();
		if (vsess_gfx(sess) != 0 && vsess_gfx(sess)->owner == my_id())
			return SURF_ERR_BUSY;                  /* the graphics surface stays with its session */
		rc = vsess_new(my_id());
		if (rc < 0)
			return rc;
		(void) vsess_move(job_current, rc);
		return (int) sid(rc);
	case SURF_OP_GFX_OPEN:
		return gfx_open(b, c);
	case SURF_OP_GFX_CLOSE:
		sess = my_sess();
		if ((g = my_gfx(&rc)) == 0)
			return rc;
		if (g->owner != my_id())
			return SURF_ERR_OWNER;
		vsess_set_gfx(sess, 0);
		surf_release(g);
		return 0;
	case SURF_OP_FILL:
		if ((g = my_gfx(&rc)) == 0)
			return rc;
		if ((rc = copy_in(&r, b, (unsigned int) sizeof r)) != 0)
			return rc;
		rc = gs_fill(g, r.x, r.y, r.w, r.h, (unsigned int) r.p & 0xffu);
		return rc == GS_ERR_ARG ? SURF_ERR_ARG : rc;
	case SURF_OP_BLIT:
	case SURF_OP_READ:
		if ((g = my_gfx(&rc)) == 0)
			return rc;
		if ((rc = copy_in(&r, b, (unsigned int) sizeof r)) != 0)
			return rc;
		rc = user_rect_ok(g, &r);
		if (rc <= 0)
			return rc;
		if (op == SURF_OP_BLIT)
			return (int) gs_blit(g, r.x, r.y, r.w, r.h, (const unsigned char *) r.p, r.q);
		return (int) gs_read(g, r.x, r.y, r.w, r.h, (unsigned char *) r.p, r.q);
	case SURF_OP_SCROLL:
		if ((g = my_gfx(&rc)) == 0)
			return rc;
		if ((rc = copy_in(&r, b, (unsigned int) sizeof r)) != 0)
			return rc;
		rc = gs_scroll(g, r.x, r.y, r.w, r.h, (long) r.p, (unsigned int) r.q & 0xffu);
		return rc == GS_ERR_ARG ? SURF_ERR_ARG : rc;
	case SURF_OP_PAL_SET:
	case SURF_OP_PAL_GET: {
		unsigned int first = b & 0xffffu, count = b >> 16;

		if ((g = my_gfx(&rc)) == 0)
			return rc;
		if (count == 0 || first > 255u || count > 256u - first)
			return SURF_ERR_ARG;
		if (!chan_user_range_ok(c, count * 3u))
			return SURF_ERR_RANGE;
		if (op == SURF_OP_PAL_SET)
			return gs_set_palette(g, first, count, (const unsigned char *) c);
		return gs_get_palette(g, first, count, (unsigned char *) c);
	}
	case SURF_OP_TEXT:
		if ((g = my_gfx(&rc)) == 0)
			return rc;
		if ((rc = copy_in(&t, b, (unsigned int) sizeof t)) != 0)
			return rc;
		if (t.len > 256u)
			return SURF_ERR_ARG;
		if (!chan_user_range_ok((unsigned int) t.str, (unsigned int) t.len))
			return SURF_ERR_RANGE;
		return gs_text(g, t.x, t.y, (const unsigned char *) t.str, (unsigned int) t.len, (unsigned int) t.ink & 0xffu,
		               (unsigned int) t.paper & 0xffu, (unsigned int) t.attr & 0x0fu);
	case SURF_OP_PRINT:
		if ((g = my_gfx(&rc)) == 0)
			return rc;
		if (c > 256u)
			return SURF_ERR_ARG;
		if (!chan_user_range_ok(b, c))
			return SURF_ERR_RANGE;
		gs_print(g, (const unsigned char *) b, c);
		return (int) c;
	case SURF_OP_COLOURS:
		if ((g = my_gfx(&rc)) == 0)
			return rc;
		gs_text_colours(g, b & 0xffu, (b >> 8) & 0xffu, (b >> 16) & 0x0fu);
		return 0;
	case SURF_OP_CURSOR:
		if ((g = my_gfx(&rc)) == 0)
			return rc;
		if (b == 0xFFFFFFFFu)
			return (int) (g->tcol | (g->trow << 8) | ((unsigned int) (g->cursor_on ? 1 : 0) << 16));
		rc = gs_cursor(g, b & 0xffu, (b >> 8) & 0xffu, (int) ((b >> 16) & 1u));
		return rc == GS_ERR_ARG ? SURF_ERR_ARG : 0;
	case SURF_OP_FOREGROUND:
		sess = resolve(b);
		if (sess < 0)
			return sess;
		if (sess != my_sess())
			return SURF_ERR_OWNER;                 /* a program shows only its own session */
		return vcon_switch(sess) < 0 ? SURF_ERR_STALE : 0;
	case SURF_OP_FOCUS:
		return vcon_fg() == my_sess() && !vcon_overlay_on();
	case SURF_OP_SUM:
		if ((g = my_gfx(&rc)) == 0)
			return rc;
		return (int) (gs_checksum(g) & 0x7fffffffu);   /* 31 bits: never mistaken for an error code */
	case SURF_OP_FRAME:
		if ((g = my_gfx(&rc)) == 0)
			return rc;
		return (int) ++g->frames;
	case SURF_OP_LIST:
		return list(b, c);
	case SURF_OP_CELLS:
		if ((rc = copy_in(&ce, b, (unsigned int) sizeof ce)) != 0)
			return rc;
		if (ce.n > VCON_CELLS)
			return SURF_ERR_ARG;
		if (!chan_user_range_ok((unsigned int) ce.cells, (unsigned int) ce.n * 2u))
			return SURF_ERR_RANGE;
		rc = vcon_write_cells(my_sess(), (unsigned int) ce.row, (unsigned int) ce.col,
		                      (const unsigned short *) ce.cells, (unsigned int) ce.n);
		return rc < 0 ? SURF_ERR_ARG : rc;
	case SURF_OP_STATS: {
		unsigned int ps[5], i;

		if (!chan_user_range_ok(b, 32u))
			return SURF_ERR_RANGE;
		pres_stats(ps);
		((unsigned int *) b)[0] = n_open;
		((unsigned int *) b)[1] = n_opened;
		((unsigned int *) b)[2] = n_refused;
		((unsigned int *) b)[3] = n_freed;
		for (i = 0; i < 4; i++)
			((unsigned int *) b)[4 + i] = ps[i];
		return 0;
	}
	default:
		return SURF_ERR_ARG;
	}
}
#else
typedef int surface_off_unit;        /* an empty translation unit is not ISO C */
#endif /* QDOS_SURFACES */
