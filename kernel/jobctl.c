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

#ifdef QDOS_TITLES
/* R12C: per-slot strings, valid while meta[slot].id is the job's id (reset when the slot is reused). */
static char t_exe[JOB_MAX][JT_EXE_MAX];       /* the launch name as given (diagnostics, JT_OP_EXE) */
static char t_base[JOB_MAX][JC2_EXE_MAX];     /* its basename */
static char t_prog[JOB_MAX][JT_TITLE_MAX];    /* compiled title (_prog_name) */
static char t_run[JOB_MAX][JT_TITLE_MAX];     /* runtime title (qd_set_title) */
unsigned long titles_set, titles_refused;
#endif

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
#ifdef QDOS_TITLES
	t_exe[slot][0] = 0;
	t_base[slot][0] = 0;
	t_prog[slot][0] = 0;
	t_run[slot][0] = 0;
#endif
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
#ifdef QDOS_TITLES
	{
		/* the file the loader actually opened (after the PATH search and the .AOU suffix), else the name as given */
		const char *img_resolved_name(void);
		const char *r = img_resolved_name();

		if (r == 0)
			r = tmp;
		for (i = 0; r[i] && i < JT_EXE_MAX - 1; i++)
			t_exe[slot][i] = r[i];
		t_exe[slot][i] = 0;
		base = t_exe[slot];
		for (s = t_exe[slot]; *s; s++)
			if (*s == '/' || *s == ':')
				base = s + 1;
		if (base == t_exe[slot] && base[0] == 'W' && base[1] == 'I' && base[2] == 'N' && base[3] == '1' && base[4] == '_')
			base += 5;
		for (n = 0, p = base; *p && n < JC2_EXE_MAX - 1; p++)
			t_base[slot][n++] = *p;
		t_base[slot][n] = 0;
	}
	{
		/* the compiled title from the image's symbol table (_prog_name): binaries linked before R12C included */
		const char *img_symbol_title(void);
		const char *c = img_symbol_title();

		for (i = 0; c[i] && i < JT_TITLE_MAX - 1; i++)
			t_prog[slot][i] = c[i];
		t_prog[slot][i] = 0;
	}
#endif
}

#ifdef QDOS_TITLES
void jobctl_note_kernel(int id, const char *exe)
{
	int slot = job_lookup((unsigned int) id);
	unsigned int i;

	if (slot == JOB_NONE)
		return;
	for (i = 0; exe[i] && i < JT_EXE_MAX - 1; i++)
		t_exe[slot][i] = exe[i];
	t_exe[slot][i] = 0;
	for (i = 0; exe[i] && i < JC2_EXE_MAX - 1; i++)
		t_base[slot][i] = exe[i];
	t_base[slot][i] = 0;
	for (i = 0; exe[i] && i < JC_NAME_MAX - 1; i++)
		meta[slot].name[i] = exe[i];
	meta[slot].name[i] = 0;
}

static int mine(int s)
{
	return s >= 0 && s < JOB_MAX && jobs[s].in_use && meta[s].id == jobs[s].id;
}

const char *jobctl_exe_base(int s)
{
	if (mine(s) && t_base[s][0])
		return t_base[s];
	if (mine(s) && meta[s].name[0])
		return meta[s].name;
	return (s >= 0 && s < JOB_MAX && jobs[s].name) ? jobs[s].name : "";
}

int jobctl_title_src(int s)
{
	if (mine(s) && t_run[s][0])
		return 2;
	if (mine(s) && t_prog[s][0])
		return 1;
	return 0;
}

const char *jobctl_title(int s)
{
	int src = jobctl_title_src(s);

	return src == 2 ? t_run[s] : src == 1 ? t_prog[s] : jobctl_exe_base(s);
}

/* Copy a title from the caller: at most JT_TITLE_MAX-1 bytes, every byte outside 0x20..0x7E stored as '?'.  Reads
 * stop at the NUL or after JT_TITLE_MAX-1 bytes (a longer string is truncated, never read beyond that).  The target is
 * changed only when every byte read was in the caller's memory. */
static int title_copy_in(unsigned int addr, char *dst)
{
	char tmp[JT_TITLE_MAX];
	unsigned int i;

	for (i = 0; i < JT_TITLE_MAX - 1; i++) {
		unsigned char c;

		if (!chan_user_range_ok(addr + i, 1)) {
			titles_refused++;
			return CHAN_ERR_RANGE;
		}
		c = *(const unsigned char *) (addr + i);
		if (c == 0)
			break;
		tmp[i] = (char) ((c < 0x20 || c > 0x7e) ? '?' : c);
	}
	tmp[i] = 0;
	{
		unsigned int k;
		for (k = 0; k <= i; k++)
			dst[k] = tmp[k];
	}
	titles_set++;
	return (int) i;
}

static int str_out(unsigned int buf, unsigned int size, const char *s)
{
	unsigned int n = 0, k, i;
	char *d;

	while (s[n])
		n++;
	if (size == 0 || !chan_user_range_ok(buf, size))
		return CHAN_ERR_RANGE;
	k = n < size - 1 ? n : size - 1;
	d = (char *) buf;
	for (i = 0; i < k; i++)
		d[i] = s[i];
	d[k] = 0;
	return (int) n;
}

int jobctl_title_trap(unsigned int op, unsigned int b, unsigned int c)
{
	int s = job_current;

	if (s < 0 || s >= JOB_MAX)
		return -2;
	if (meta[s].id != jobs[s].id)
		jobctl_slot_reset(s);
	switch (op) {
	case JT_OP_SET:
		if (b == 0) {
			t_run[s][0] = 0;
			titles_set++;
			return 0;
		}
		return title_copy_in(b, t_run[s]);
	case JT_OP_SET_PROG:
		if (b == 0) {
			t_prog[s][0] = 0;
			return 0;
		}
		return title_copy_in(b, t_prog[s]);
	case JT_OP_GET:
		return str_out(b, c, jobctl_title(s));
	case JT_OP_EXE:
		return str_out(b, c, t_exe[s][0] ? t_exe[s] : jobctl_exe_base(s));
	default:
		return -15;
	}
}
#endif

int jobctl_killed(unsigned int id)
{
	int i = (int) (id & 0xffffu);

	return i > 0 && i < JOB_MAX && meta[i].id == id && meta[i].killed;
}

#ifdef QDOS_TITLES
unsigned char jobctl_state(int i);
#define state_of jobctl_state
unsigned char jobctl_state(int i)
#else
static unsigned char state_of(int i)
#endif
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

#ifdef QDOS_TITLES
#ifdef QDOS_DYNPROC
unsigned int proc_parent_of(int slot);           /* proc.c */
#endif

unsigned int jobctl_flags(int s)
{
	unsigned int f = (meta[s].id == jobs[s].id ? meta[s].flags : 0) | (s == job_current ? JC_F_SELF : 0);

	if (!(f & JC_F_LAUNCHED))
		f |= JC_F_PROTECTED;
#ifdef QDOS_DYNPROC
	if (s == 0 || s == job_idle || proc_parent_of(s) == 0xFFFFFFFFu)
		f |= JC_F_KERNEL;
#endif
	return f;
}

unsigned int jobctl_start_tick(int slot)
{
	return meta[slot].id == jobs[slot].id ? meta[slot].start_tick : 0u;
}

/* Jobs on console con that are not kernel jobs (the "shared console" count shown by the task lists). */
int jobctl_con_programs(int con)
{
	int s, n = 0;

	for (s = 0; s < JOB_MAX; s++)
		if (jobs[s].in_use && !jobs[s].dead && !(jobctl_flags(s) & JC_F_KERNEL)
#ifdef QDOS_VCON
		    && vcon_job_con(s) == con
#endif
		    )
			n++;
	return n;
}

static void put_str(unsigned char *d, const char *s, unsigned int max)
{
	unsigned int i;

	for (i = 0; s[i] && i < max - 1; i++)
		d[i] = (unsigned char) s[i];
	d[i] = 0;
}

/* v2: 128-byte records with the parent, the console's job count, the executable basename and the effective title. */
static int query_v2(unsigned int buf, unsigned int bytes)
{
	unsigned char *p = (unsigned char *) buf;
	unsigned int n = 0, room, k;
	int s;

	if (bytes < JC_HDR_BYTES || !chan_user_range_ok(buf, bytes))
		return CHAN_ERR_RANGE;
	room = (bytes - JC_HDR_BYTES) / JC2_ENTRY_BYTES;
	for (s = 0; s < JOB_MAX && n < room; s++) {
		unsigned char *e;
		unsigned int par = 0;

		if (!jobs[s].in_use)
			continue;
		e = p + JC_HDR_BYTES + n * JC2_ENTRY_BYTES;
		for (k = 0; k < JC2_ENTRY_BYTES; k++)
			e[k] = 0;
		put32(e, jobs[s].id);
		e[4] = (unsigned char) s;
		e[5] = state_of(s);
		e[6] = jobs[s].prio_inc;
#ifdef QDOS_VCON
		e[7] = (unsigned char) vcon_job_con(s);
		e[9] = (unsigned char) jobctl_con_programs(vcon_job_con(s));
#else
		e[7] = JC_NOCON;
#endif
		e[8] = (unsigned char) jobctl_flags(s);
		e[10] = (unsigned char) jobctl_title_src(s);
#ifdef QDOS_DYNPROC
		par = proc_parent_of(s);
		if (par == 0xFFFFFFFFu)
			par = 0;
#endif
		put32(e + 12, par);
		put32(e + 16, (unsigned int) jobs[s].dispatches);
		put32(e + 20, (unsigned int) sched_ticks - (meta[s].id == jobs[s].id ? meta[s].start_tick : 0u));
		put_str(e + 24, jobctl_exe_base(s), JC2_EXE_MAX);
		put_str(e + 56, jobctl_title(s), JT_TITLE_MAX);
		n++;
	}
	put32(p, JC_VERSION2);
	put32(p + 4, JC2_ENTRY_BYTES);
	put32(p + 8, n);
	put32(p + 12, JOB_MAX);
	return (int) n;
}
#endif

int jobctl_query(unsigned int buf, unsigned int bytes, unsigned int version)
{
	unsigned char *p = (unsigned char *) buf;
	unsigned int n = 0, room, i, k;
	int s;

#ifdef QDOS_TITLES
	if (version == JC_VERSION2)
		return query_v2(buf, bytes);
#endif
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
