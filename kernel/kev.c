/* QDOS/386 R13A - native keyboard events.  See kev.h. */
#include "kev.h"
#include "vcon.h"
#include "present.h"

struct kevq {
	int on;
	unsigned int h, t;                 /* write, read */
	unsigned int ev[KEV_QSIZE];
	unsigned char down[32];            /* 256 keys: code | ext << 7 */
};

static struct kevq q[VSESS_MAX];
static int last = -1;                  /* the session that receives events now (-1: none: the selector is up) */
static unsigned char hot_down[32];     /* keys whose down was a hotkey: their up is consumed too */
static unsigned int st_deliv, st_drop, st_synth, st_hot;

static int target(void)
{
	if (vcon_overlay_on())
		return -1;
	return vcon_fg();
}

static void put(int idx, unsigned int e)
{
	struct kevq *k = &q[idx];

	if (!k->on)
		return;
	if (((k->h + 1u) % KEV_QSIZE) == k->t) {
		st_drop++;
		return;
	}
	k->ev[k->h] = e;
	k->h = (k->h + 1u) % KEV_QSIZE;
	st_deliv++;
}

static void release_all(int idx)
{
	unsigned int key;
	struct kevq *k;

	if (idx < 0 || idx >= VSESS_MAX)
		return;
	k = &q[idx];
	for (key = 0; key < 256; key++)
		if (k->down[key >> 3] & (1u << (key & 7u))) {
			k->down[key >> 3] &= (unsigned char) ~(1u << (key & 7u));
			put(idx, (key & 0x7fu) | ((key & 0x80u) ? KEV_EXT : 0u) | KEV_SYNTH);
			st_synth++;
		}
}

void kev_refocus(void)
{
	int t = target();

	if (t != last) {
		release_all(last);
		last = t;
	}
}

void kev_scancode(unsigned int sc, int ext, int hot, unsigned int ch, unsigned int mods)
{
	unsigned int code = sc & 0x7fu, key = code | (ext ? 0x80u : 0u), bit = 1u << (key & 7u);
	int down = (sc & 0x80u) == 0, t;

	if (sc == 0xe0u || sc == 0xe1u)
		return;
	if (hot) {                          /* the hotkey's own down (Fn / digit / Esc) */
		hot_down[key >> 3] |= (unsigned char) bit;
		st_hot++;
		kev_refocus();
		return;
	}
	if (!down && (hot_down[key >> 3] & bit)) {
		hot_down[key >> 3] &= (unsigned char) ~bit;
		st_hot++;
		return;
	}
	kev_refocus();
	t = last;
	if (t < 0 || t >= VSESS_MAX)
		return;
	if (down) {
		q[t].down[key >> 3] |= (unsigned char) bit;
		put(t, code | (ext ? KEV_EXT : 0u) | KEV_DOWN | mods | ((ch & 0xffu) << 16));
	} else {
		if (!(q[t].down[key >> 3] & bit))
			return;                     /* its down went to another session (or to a hotkey) */
		q[t].down[key >> 3] &= (unsigned char) ~bit;
		put(t, code | (ext ? KEV_EXT : 0u) | mods);
	}
}

int kev_enable(int idx, int on)
{
	unsigned int i;

	if (idx < 0 || idx >= VSESS_MAX)
		return -4;
	q[idx].on = on != 0;
	q[idx].h = q[idx].t = 0;
	for (i = 0; i < 32; i++)
		q[idx].down[i] = 0;
	return 0;
}

int kev_read(int idx, unsigned int *out, int max)
{
	int n = 0;
	struct kevq *k;

	if (idx < 0 || idx >= VSESS_MAX)
		return 0;
	k = &q[idx];
	while (n < max && k->t != k->h) {
		out[n++] = k->ev[k->t];
		k->t = (k->t + 1u) % KEV_QSIZE;
	}
	return n;
}

void kev_session_gone(int idx)
{
	if (idx >= 0 && idx < VSESS_MAX)
		(void) kev_enable(idx, 0);
	if (last == idx)
		last = -2;                      /* nobody: the next refocus does not synthesise for a reused index */
}

void kev_stats(unsigned int *o)
{
	o[0] = st_deliv;
	o[1] = st_drop;
	o[2] = st_synth;
	o[3] = st_hot;
}
