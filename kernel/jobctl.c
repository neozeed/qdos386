/* QDOS/386 R11M - native job control.  Empty unless QDOS_JOBCTL is defined.  Freestanding C89. */
#ifdef QDOS_JOBCTL
#include "jobs.h"
#include "jobctl.h"
#include "channel.h"
#ifdef QDOS_PAGING
#include "paging.h"
#endif
#ifdef QDOS_VCON
#include "vcon.h"
#endif

struct jc_meta {
	char name[JC_NAME_MAX];
	unsigned int start_tick;
	unsigned char flags;
	unsigned char killed;
	unsigned int id;                  /* id the record belongs to */
};

static struct jc_meta meta[JOB_MAX];

void jobctl_slot_reset(int slot)
{
	unsigned int i;

	if (slot < 0 || slot >= JOB_MAX)
		return;
	for (i = 0; i < JC_NAME_MAX; i++)
		meta[slot].name[i] = 0;
	meta[slot].start_tick = (unsigned int) sched_ticks;
	meta[slot].flags = 0;
	meta[slot].killed = 0;
	meta[slot].id = jobs[slot].id;
}

/* The last component of a file name, without a volume prefix ("WIN1_", "WIN1:"). */
void jobctl_note_launch(int id, unsigned int name_addr, unsigned int flags)
{
	int slot = job_lookup((unsigned int) id);
	const char *p, *s, *base;
	unsigned int i, n = 0;
	char tmp[80];

	if (slot == JOB_NONE)
		return;
	meta[slot].flags = (unsigned char) (flags | JC_F_LAUNCHED);
	for (i = 0; i < sizeof tmp - 1; i++) {
		if (!chan_user_range_ok(name_addr + i, 1))
			break;
		tmp[i] = *(const char *) (name_addr + i);
		if (tmp[i] == 0)
			break;
	}
	tmp[i] = 0;
	base = tmp;
	for (s = tmp; *s; s++)
		if (*s == '/' || *s == ':')
			base = s + 1;
	if (base == tmp && tmp[0] == 'W' && tmp[1] == 'I' && tmp[2] == 'N' && tmp[3] == '1' && tmp[4] == '_')
		base = tmp + 5;                       /* the legacy flat form WIN1_NAME */
	for (p = base; *p && n < JC_NAME_MAX - 1; p++)
		meta[slot].name[n++] = *p;
	meta[slot].name[n] = 0;
}

int jobctl_killed(unsigned int id)
{
	int i = (int) (id & 0xffffu);

	return i > 0 && i < JOB_MAX && meta[i].id == id && meta[i].killed;
}

static unsigned char state_of(int i)
{
	const struct job *j = &jobs[i];

	if (j->dead)
		return JC_S_DEAD;
	if (j->prio_inc == 0)
		return j->dispatches == 0 ? JC_S_HELD : JC_S_SLEEP;
	if (j->status != 0) {
#ifdef QDOS_VCON
		if (vcon_waiter(vcon_job_con(i)) == i)
			return JC_S_CONWAIT;
#endif
		return JC_S_SLEEP;
	}
	return JC_S_RUN;
}

static void put32(unsigned char *d, unsigned int v)
{
	d[0] = (unsigned char) v;
	d[1] = (unsigned char) (v >> 8);
	d[2] = (unsigned char) (v >> 16);
	d[3] = (unsigned char) (v >> 24);
}

int jobctl_query(unsigned int buf, unsigned int bytes, unsigned int version)
{
	unsigned char *p = (unsigned char *) buf;
	unsigned int n = 0, room, i, k;
	int s;

	if (version != JC_VERSION)
		return -34;
	if (bytes < JC_HDR_BYTES || !chan_user_range_ok(buf, bytes))
		return CHAN_ERR_RANGE;
	room = (bytes - JC_HDR_BYTES) / JC_ENTRY_BYTES;
	for (s = 1; s < JOB_MAX && n < room; s++) {
		unsigned char *e;
		const char *nm;

		if (!jobs[s].in_use)
			continue;
		e = p + JC_HDR_BYTES + n * JC_ENTRY_BYTES;
		for (k = 0; k < JC_ENTRY_BYTES; k++)
			e[k] = 0;
		put32(e, jobs[s].id);
		e[4] = (unsigned char) s;
		e[5] = state_of(s);
		e[6] = jobs[s].prio_inc;
#ifdef QDOS_VCON
		e[7] = (unsigned char) vcon_job_con(s);
#else
		e[7] = JC_NOCON;
#endif
		e[8] = (unsigned char) ((meta[s].id == jobs[s].id ? meta[s].flags : 0) | (s == job_current ? JC_F_SELF : 0));
		put32(e + 12, (unsigned int) jobs[s].dispatches);
		put32(e + 16, (unsigned int) sched_ticks - meta[s].start_tick);
		put32(e + 20, jobs[s].stack_bytes);
		nm = (meta[s].id == jobs[s].id && meta[s].name[0]) ? meta[s].name : jobs[s].name;
		for (i = 0; nm != 0 && nm[i] && i < JC_NAME_MAX - 1; i++)
			e[24 + i] = (unsigned char) nm[i];
		n++;
	}
	put32(p, JC_VERSION);
	put32(p + 4, JC_ENTRY_BYTES);
	put32(p + 8, n);
	put32(p + 12, JOB_MAX);
	return (int) n;
}

int jobctl_kill(unsigned int id)
{
	int slot = job_lookup(id);

	if (slot == JOB_NONE)
		return -2;
	if (slot == job_current)
		return JC_ERR_SELF;
	if (meta[slot].id != id || !(meta[slot].flags & JC_F_LAUNCHED))
		return JC_ERR_PROTECT;
	if (jobs[slot].dead)
		return JC_ERR_DEAD;
	meta[slot].killed = 1;
	job_mark_dead_other(slot);
	return 0;
}
#else
typedef int jobctl_off_unit;      /* an empty translation unit is not ISO C */
#endif /* QDOS_JOBCTL */
