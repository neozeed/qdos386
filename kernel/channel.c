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
#include "memory.h"
#include "platform.h"

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

static int con_drv_open(struct chan_object *ch)
{
	(void) ch;
	return 0;
}

static void con_drv_close(struct chan_object *ch)
{
	(void) ch;
}

static int con_drv_write(struct chan_object *ch, const unsigned char *buf, unsigned int n)
{
	unsigned int i;
	(void) ch;
	for (i = 0; i < n; i++)
		con_putc(buf[i]);
	return (int) n;
}

static struct chan_driver con_driver;
static struct chan_driver *drivers[2];

static void drivers_init(void)
{
	con_driver.name = "CON";
	con_driver.open = con_drv_open;
	con_driver.close = con_drv_close;
	con_driver.write = con_drv_write;
	drivers[0] = &con_driver;
	drivers[1] = 0;
}

static int drivers_ready = 0;

/* ---- user buffers ---------------------------------------------------------- */

int chan_user_range_ok(unsigned int addr, unsigned int n)
{
	if (addr == 0 && n != 0)
		return 0;
	if (addr + n < addr)
		return 0;                    /* wraps */
	return addr + n <= MEM_ARENA_END;
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

int chan_open(unsigned int owner, unsigned int name_addr)
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
		if (name_eq(drivers[d]->name, name))
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

	rc = ch->drv->open(ch);
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
	ch->drv->close(ch);
	slots[ch->index].obj = 0;
	live--;
	chan_closed++;
	if (mem_free(ch, owner) != MEM_OK)
		return CHAN_ERR_BADID;       /* cannot happen: the block is ours */
	return 0;
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
	rc = ch->drv->write(ch, &b, 1);
	if (rc > 0)
		ch->bytes_out += (unsigned long) rc;
	return rc;
}

unsigned int chan_close_owner(unsigned int owner)
{
	unsigned int slot;
	unsigned int n = 0;
	for (slot = 0; slot < CHAN_MAX; slot++) {
		struct chan_object *ch = slots[slot].obj;
		if (ch == 0 || ch->owner != owner)
			continue;
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
