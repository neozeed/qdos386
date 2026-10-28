/*
 * QDOS/386 R4 - native channel manager (see channel.h).
 *
 * Table: CHAN_MAX slots.  A slot holds a pointer to its open channel object
 * (NULL when free) and the last tag issued for it.  Opening takes the lowest
 * free slot and the next tag.  Closing frees the object and leaves the slot
 * free with its tag unchanged, so the next open of that slot issues a new
 * tag, and an old id no longer matches.
 *
 * Validation for every call, in this order: index in range, slot in use,
 * tag equal, owner equal.  A failed check changes nothing.
 *
 * Interrupts: every entry point is reached from a trap, so IF is clear.  The
 * memory calls protect themselves with irq_save/irq_restore as usual.
 */
#include "channel.h"
#include "cin.h"
#include "memory.h"
#include "platform.h"
#ifdef QDOS_PAGING
#include "paging.h"
#endif
#ifdef QDOS_VCON
#ifndef QDOS_FS_WRITE
#error "QDOS_VCON needs QDOS_FS_WRITE (the channel control trap)"
#endif
#include "vcon.h"
extern int job_current;              /* jobs.c: slot of the running job */
#endif

struct chan_slot {
	struct chan_object *obj;
	unsigned int tag;                /* last tag issued */
};

static struct chan_slot slots[CHAN_MAX];
static unsigned long live = 0;
volatile unsigned long chan_opened = 0;
volatile unsigned long chan_closed = 0;
volatile unsigned long chan_closed_on_exit = 0;
volatile unsigned long chan_rejected = 0;

/* ---- console driver: the only R4 backend --------------------------------- */

static int con_drv_open(struct chan_object *ch, const char *name)
{
	(void) name;
#ifdef QDOS_VCON
	ch->con = vcon_job_con(job_current);          /* the opener's console: its session */
#else
	(void) ch;
#endif
	return 0;
}

static void con_drv_close(struct chan_object *ch)
{
	(void) ch;
}

static int con_drv_write(struct chan_object *ch, const unsigned char *buf, unsigned int n)
{
#ifdef QDOS_VCON
	vcon_write(ch->con, buf, n);
	return (int) n;
#else
	unsigned int i;
	(void) ch;
	for (i = 0; i < n; i++)
		con_putc(buf[i]);
	return (int) n;
#endif
}

/* CON read (R7): takes what is waiting, never blocks.  0 at end of input,
 * CHAN_ERR_AGAIN when nothing is waiting yet. */
static int con_drv_read(struct chan_object *ch, unsigned char *buf, unsigned int n)
{
	unsigned int got = 0;
	unsigned char c;
#ifdef QDOS_VCON
	while (got < n && vcon_get(ch->con, &c))
		buf[got++] = c;
	if (got != 0)
		return (int) got;
	if (vcon_at_eof(ch->con))
		return 0;
	return CHAN_ERR_AGAIN;
#else
	(void) ch;
	while (got < n && cin_get(&c))
		buf[got++] = c;
	if (got != 0)
		return (int) got;
	if (cin_at_eof())
		return 0;
	return CHAN_ERR_AGAIN;
#endif
}

#ifdef QDOS_VCON
/* Console controls.  ch->con is the console of the session that opened the channel. */
static int con_drv_ctl(struct chan_object *ch, unsigned int op, unsigned long arg)
{
	int con = ch->con;

	switch (op) {
	case CHAN_CTL_FLUSH:              /* chan_close flushes every channel whose driver has a ctl */
		return 0;
	case CHAN_CON_GOTO:
		return vcon_goto(con, (unsigned int) arg & 0xffu, (unsigned int) (arg >> 8) & 0xffu);
	case CHAN_CON_CLEAR:
		if (arg > VCLR_EOS)
			return CHAN_ERR_RANGE;
		vcon_clear(con, (unsigned int) arg);
		return 0;
	case CHAN_CON_ATTR:
		return (int) vcon_set_attr(con, (unsigned int) arg);
	case CHAN_CON_QUERY:
		return (int) vcon_query(con, (unsigned int) arg);
	case CHAN_CON_MODE:
		return (int) vcon_set_mode(con, (unsigned int) arg);
	case CHAN_CON_CURSOR:
		vcon_set_cursor_visible(con, arg != 0);
		return 0;
	case CHAN_CON_REGION:
		return vcon_set_region(con, (unsigned int) arg & 0xffu, (unsigned int) (arg >> 8) & 0xffu);
	case CHAN_CON_FOCUS:
		return vcon_switch((int) arg);
	case CHAN_CON_CHILD:
		return vcon_set_child(job_current, arg == 0xffu ? -1 : (int) arg);
	case CHAN_CON_SNAP:
		{
			struct chan_con_snap *s = (struct chan_con_snap *) arg;
			int c, r;

			if (!chan_user_range_ok((unsigned int) arg, (unsigned int) sizeof(struct chan_con_snap)))
				return CHAN_ERR_RANGE;
			c = s->con == 0xffu ? con : (int) s->con;
			r = vcon_snap_row(c, s->row, s->text, (unsigned int) sizeof s->text);
			return r;
		}
	case CHAN_CON_DIAG:
		if (arg != 0xffu)
			vcon_set_diag((int) arg);
		return vcon_get_diag();
	}
	return CHAN_ERR_NOTSUP;
}
#endif

extern struct chan_driver win1_driver;   /* win1.c: WIN1_ file channels */

static struct chan_driver con_driver;
#ifdef QDOS_FS_WRITE
extern struct chan_driver win1n_driver;  /* win1.c: the "WIN1:" native path prefix (R11M) */
static struct chan_driver *drivers[4];
#else
static struct chan_driver *drivers[3];
#endif

static void drivers_init(void)
{
	con_driver.name = "CON";
	con_driver.prefix = 0;
	con_driver.read = con_drv_read;
	con_driver.seek = 0;
	con_driver.open = con_drv_open;
	con_driver.close = con_drv_close;
	con_driver.write = con_drv_write;
#ifdef QDOS_VCON
	con_driver.ctl = con_drv_ctl;
#endif
	drivers[0] = &con_driver;
	drivers[1] = &win1_driver;
#ifdef QDOS_FS_WRITE
	drivers[2] = &win1n_driver;
	drivers[3] = 0;
#else
	drivers[2] = 0;
#endif
}

static int drivers_ready = 0;

/* ---- user buffers ---------------------------------------------------------- */

int chan_user_range_ok(unsigned int addr, unsigned int n)
{
#ifdef QDOS_PAGING
	/* R11J: a paged caller may name only its own window and the shared kernel heap. */
	{
		int pr = pg_user_range_ok(addr, n);
		if (pr >= 0)
			return pr;
	}
#endif
	/* R11E: the lower bound is the heap arena base.  Before this, only the
	 * upper bound was checked, so a user buffer could name kernel memory. */
	if (addr == 0 && n != 0)
		return 0;
	if (mem_arena_base() != 0 && addr < mem_arena_base())
		return 0;
	if (addr + n < addr)
		return 0;                    /* wraps */
	return addr + n <= USER_RANGE_END;
}

/* Copy a driver name (NUL-terminated, at most CHAN_NAME_MAX-1 characters)
 * into out.  Returns 0 on success, CHAN_ERR_RANGE if the name is not in
 * range or is not terminated within the limit. */
static int copy_name(unsigned int addr, char *out)
{
	unsigned int i;
	for (i = 0; i < CHAN_NAME_MAX; i++) {
		if (!chan_user_range_ok(addr + i, 1))
			return CHAN_ERR_RANGE;
		out[i] = *(char *) (addr + i);
		if (out[i] == 0)
			return 0;
	}
	return CHAN_ERR_RANGE;
}

static int name_eq(const char *a, const char *b);

/* Does the driver serve this name?  Exact match, or prefix match for a
 * prefix driver (its name is the prefix, followed by the file name). */
static int name_match(const struct chan_driver *d, const char *name)
{
	unsigned int k = 0;
	if (!d->prefix)
		return name_eq(d->name, name);
	while (d->name[k] != 0 && d->name[k] == name[k])
		k++;
	/* R7: the bare prefix is allowed; the driver reads it as its root listing */
	return d->name[k] == 0;
}

/* Exact match of two NUL-terminated names. */
static int name_eq(const char *a, const char *b)
{
	unsigned int k = 0;
	while (a[k] != 0 && a[k] == b[k])
		k++;
	return a[k] == b[k];
}

/* ---- lookup and validation -------------------------------------------------- */

static int check(unsigned int id, unsigned int owner, struct chan_object **out)
{
	unsigned int index = id & 0xffffu;
	unsigned int tag = id >> 16;
	struct chan_object *ch;

	if (index >= CHAN_MAX || slots[index].obj == 0 || slots[index].tag != tag) {
		chan_rejected++;
		return CHAN_ERR_BADID;
	}
	ch = slots[index].obj;
	if (ch->owner != owner) {
		chan_rejected++;
		return CHAN_ERR_OWNER;
	}
	*out = ch;
	return 0;
}

/* ---- service entry points -------------------------------------------------- */

#ifdef QDOS_FS_WRITE
int chan_open(unsigned int owner, unsigned int name_addr)
{
	return chan_open_ex(owner, name_addr, CHAN_O_READ);
}

int chan_open_ex(unsigned int owner, unsigned int name_addr, unsigned int flags)
#else
int chan_open(unsigned int owner, unsigned int name_addr)
#endif
{
	char name[CHAN_NAME_MAX];
	struct chan_object *ch;
	unsigned int slot, newtag;
	int rc;
	int d;
	void *p;

	if (!drivers_ready) {
		drivers_init();
		drivers_ready = 1;
	}
	rc = copy_name(name_addr, name);
	if (rc != 0)
		return rc;

	for (d = 0; drivers[d] != 0; d++)
		if (name_match(drivers[d], name))
			break;
	if (drivers[d] == 0)
		return CHAN_ERR_NAME;

	for (slot = 0; slot < CHAN_MAX; slot++)
		if (slots[slot].obj == 0)
			break;
	if (slot >= CHAN_MAX)
		return CHAN_ERR_NOSLOT;

	p = mem_alloc(sizeof(struct chan_object), owner);
	if (mem_is_error((int) (unsigned int) p))
		return (int) (unsigned int) p;
	ch = (struct chan_object *) p;

	newtag = slots[slot].tag + 1;
	if (newtag >= CHAN_TAG_MAX)
		newtag = 1;

	ch->owner = owner;
	ch->index = slot;
	ch->tag = newtag;
	ch->drv = drivers[d];
	ch->bytes_out = 0;
#ifdef QDOS_FS_WRITE
	ch->mode = flags;
#endif

	rc = ch->drv->open(ch, name);
	if (rc != 0) {
		mem_free(p, owner);
		return rc;
	}

	slots[slot].obj = ch;
	slots[slot].tag = newtag;
	live++;
	chan_opened++;
	return (int) ((newtag << 16) | slot);
}

int chan_close(unsigned int id, unsigned int owner)
{
	struct chan_object *ch;
	int rc = check(id, owner, &ch);
	if (rc != 0)
		return rc;
#ifdef QDOS_FS_WRITE
	/* R11L: the directory entry is written before the channel goes; the channel is closed whatever
	 * the result, and a failed flush is reported to the caller. */
	if (ch->drv->ctl != 0)
		rc = ch->drv->ctl(ch, CHAN_CTL_FLUSH, 0);
#endif
	ch->drv->close(ch);
	slots[ch->index].obj = 0;
	live--;
	chan_closed++;
	if (mem_free(ch, owner) != MEM_OK)
		return CHAN_ERR_BADID;       /* cannot happen: the block is ours */
#ifdef QDOS_FS_WRITE
	return rc;
#else
	return 0;
#endif
}

int chan_write(unsigned int id, unsigned int owner, unsigned int buf_addr, unsigned int n)
{
	struct chan_object *ch;
	int rc = check(id, owner, &ch);
	if (rc != 0)
		return rc;
	if (n > CHAN_MAX_WRITE) {
		chan_rejected++;
		return CHAN_ERR_RANGE;
	}
	if (n == 0)
		return 0;
	if (!chan_user_range_ok(buf_addr, n)) {
		chan_rejected++;
		return CHAN_ERR_RANGE;
	}
	if (ch->drv->write == 0)
		return CHAN_ERR_NOTSUP;
	rc = ch->drv->write(ch, (const unsigned char *) buf_addr, n);
	if (rc > 0)
		ch->bytes_out += (unsigned long) rc;
	return rc;
}

int chan_putc(unsigned int id, unsigned int owner, unsigned int c)
{
	struct chan_object *ch;
	unsigned char b;
	int rc = check(id, owner, &ch);
	if (rc != 0)
		return rc;
	b = (unsigned char) (c & 0xff);
	if (ch->drv->write == 0)
		return CHAN_ERR_NOTSUP;
	rc = ch->drv->write(ch, &b, 1);
	if (rc > 0)
		ch->bytes_out += (unsigned long) rc;
	return rc;
}

int chan_read(unsigned int id, unsigned int owner, unsigned int buf_addr, unsigned int n)
{
	struct chan_object *ch;
	int rc = check(id, owner, &ch);
	if (rc != 0)
		return rc;
	if (n > CHAN_MAX_WRITE) {
		chan_rejected++;
		return CHAN_ERR_RANGE;
	}
	if (n == 0)
		return 0;
	if (!chan_user_range_ok(buf_addr, n)) {
		chan_rejected++;
		return CHAN_ERR_RANGE;
	}
	if (ch->drv->read == 0)
		return CHAN_ERR_NOTSUP;
	return ch->drv->read(ch, (unsigned char *) buf_addr, n);
}

int chan_seek(unsigned int id, unsigned int owner, unsigned int pos)
{
	struct chan_object *ch;
	int rc = check(id, owner, &ch);
	if (rc != 0)
		return rc;
	if (ch->drv->seek == 0)
		return CHAN_ERR_NOTSUP;
	rc = ch->drv->seek(ch, pos);
	return rc;
}

#ifdef QDOS_FS_WRITE
int chan_ctl(unsigned int id, unsigned int owner, unsigned int op, unsigned int arg)
{
	struct chan_object *ch;
	int rc = check(id, owner, &ch);
	if (rc != 0)
		return rc;
	if (ch->drv->ctl == 0)
		return CHAN_ERR_NOTSUP;
	return ch->drv->ctl(ch, op, arg);
}

/* Name operations: the driver is chosen by the first name.  Only prefix drivers with an fsop.
 * R11M: CHAN_FS_GETCWD takes a buffer (a) and size (b) instead of names, and CHAN_FS_STAT a result
 * buffer in b; both ranges are checked here and handed to the driver as raw values. */
int chan_fsop(unsigned int op, unsigned int a_addr, unsigned int b_addr)
{
	char na[CHAN_NAME_MAX], nb[CHAN_NAME_MAX];
	int rc, d;

	if (!drivers_ready) {
		drivers_init();
		drivers_ready = 1;
	}
	if (op == CHAN_FS_GETCWD) {
		if (b_addr == 0 || b_addr > 256 || !chan_user_range_ok(a_addr, b_addr))
			return CHAN_ERR_RANGE;
		return win1_driver.fsop(op, (const char *) a_addr, (const char *) b_addr);
	}
	if (op == CHAN_FS_FREE || op == CHAN_FS_CLUSTER || op == CHAN_FS_TOTAL) {
		/* any WIN1 name selects the volume */
	}
	rc = copy_name(a_addr, na);
	if (rc != 0)
		return rc;
	nb[0] = 0;
	if (op == CHAN_FS_RENAME) {
		rc = copy_name(b_addr, nb);
		if (rc != 0)
			return rc;
	}
	if (op == CHAN_FS_STAT && b_addr != 0 && !chan_user_range_ok(b_addr, 8))
		return CHAN_ERR_RANGE;
	for (d = 0; drivers[d] != 0; d++)
		if (name_match(drivers[d], na))
			break;
	if (drivers[d] == 0)
		return CHAN_ERR_NAME;
	if (drivers[d]->fsop == 0)
		return CHAN_ERR_NOTSUP;
	if (op == CHAN_FS_RENAME) {
		/* both names must belong to the same volume (either WIN1 prefix) */
		int d2;
		for (d2 = 0; drivers[d2] != 0; d2++)
			if (name_match(drivers[d2], nb))
				break;
		if (drivers[d2] == 0 || drivers[d2]->fsop != drivers[d]->fsop)
			return CHAN_ERR_NAME;
	}
	if (op == CHAN_FS_STAT)
		return drivers[d]->fsop(op, na, (const char *) b_addr);
	return drivers[d]->fsop(op, na, nb);
}
#endif

unsigned int chan_close_owner(unsigned int owner)
{
	unsigned int slot;
	unsigned int n = 0;
	for (slot = 0; slot < CHAN_MAX; slot++) {
		struct chan_object *ch = slots[slot].obj;
		if (ch == 0 || ch->owner != owner)
			continue;
#ifdef QDOS_FS_WRITE
		if (ch->drv->ctl != 0)
			(void) ch->drv->ctl(ch, CHAN_CTL_FLUSH, 0);   /* best effort at job exit */
#endif
		ch->drv->close(ch);
		slots[slot].obj = 0;
		live--;
		chan_closed++;
		chan_closed_on_exit++;
		mem_free(ch, owner);
		n++;
	}
	return n;
}

unsigned long chan_objects_live(void)
{
	return live;
}
