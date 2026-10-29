/*
 * QDOS/386 R12C - SYSTASK: the global task selector (Ctrl+Esc, shell TASKS) and the optional periodic SYNC.
 * Opt-in: -DQDOS_TASKMGR (needs QDOS_VCON, QDOS_JOBCTL, QDOS_TITLES).  See docs/R12C-TASK-MANAGER-ABI.md.
 *
 * SYSTASK is not a job of its own: taskmgr_poll() runs in the SYSTEM job (slot 0), whose loop wakes every tick.  The
 * keyboard IRQ (Ctrl+Esc) or the QT_TASKMGR trap opens the selector at once: the console layer stops drawing
 * (vcon_overlay), keys go to SYSTASK's own queue instead of a console, and the SYSTEM job is woken.  SYSTASK draws the job list over the whole 80x25 display from a snapshot taken with interrupts off, acts on
 * keys, and on close the foreground console is drawn again from its own buffer: no application screen is changed.
 * Jobs keep running throughout.  Actions use the tagged job id selected, never a list index, so a job that ended or a
 * slot that was reused is detected (job_lookup / QT_JOB_KILL refuse stale ids).
 *
 * The same job runs the periodic SYNC (QDOS_SYNC): a timed sleep, then sync_now() with interrupts off, as a trap
 * would - never in the timer interrupt.  Freestanding C89.
 */
#ifdef QDOS_TASKMGR
#include "jobs.h"
#include "jobctl.h"
#include "vcon.h"
#include "taskmgr.h"
#ifdef QDOS_SURFACES
#include "gsurf.h"
#endif
#ifdef QDOS_SYNC
#include "sync.h"
#endif

extern unsigned int irq_save(void);
extern void irq_restore(unsigned int flags);

#define TM_ROWS      25
#define TM_COLS      80
#define TM_LIST_TOP  3
#define TM_LIST_ROWS 18                 /* rows 3..20 */
#define TM_Q         32
#define TM_REFRESH   25                 /* ticks between redraws while open (state changes) */

#define A_BAR   0x1f                    /* white on blue */
#define A_LIST  0x17                    /* grey on blue */
#define A_SEL   0x70                    /* black on grey */
#define A_HEAD  0x1e                    /* yellow on blue */
#define A_WARN  0x4f                    /* white on red */

#define K_UP    0x80
#define K_DOWN  0x81
#define K_HOME  0x84
#define K_END   0x85
#define K_PGUP  0x86
#define K_PGDN  0x87
#define K_F1    0x90

struct tm_row {
	unsigned int id, parent, start;
	int slot, con, users;
	unsigned int flags;
	unsigned char state;
#ifdef QDOS_SURFACES
	unsigned int gw, gh;               /* R13: its session's INDEX8 size, 0 for a text session */
#endif
	char title[JT_TITLE_MAX];
	char exe[JC2_EXE_MAX];
};

static volatile int tm_open;
static int tm_slot = -1;                /* SYSTASK's slot */
static int tm_prev_fg;                  /* the foreground console when the selector opened */
static unsigned char tm_q[TM_Q];
static volatile unsigned int tm_qh, tm_qt;
static volatile int tm_fresh;           /* opened since SYSTASK last looked */
static unsigned int tm_sel_id;          /* the selected job, by tagged id */
static int tm_top;                      /* first list row shown */
static unsigned int tm_confirm_id;      /* F1 asked to end this job; waiting for Y */
static int tm_gone_said;                /* the "selected job has ended" message was shown */
static char tm_msg[TM_COLS + 1];
static struct tm_row rows[JOB_MAX];
static int nrows;
static unsigned short scr[TM_ROWS * TM_COLS];
static unsigned long tm_last_draw;
unsigned long tm_opens, tm_switches, tm_kills, tm_keys, tm_draws;

/* ---- called with interrupts off (keyboard IRQ or a trap) ----------------------------------------------------- */

static void wake_systask(void)
{
	if (tm_slot >= 0)
		job_wake(tm_slot);
}

int taskmgr_open(void)
{
	if (tm_slot < 0)
		return -1;
	if (tm_open)
		return 0;
	tm_prev_fg = vcon_fg();
	tm_qh = tm_qt = 0;
	tm_fresh = 1;
	tm_open = 1;
	tm_opens++;
	vcon_overlay(1);
	wake_systask();
	return 0;
}

static void tm_close(int restore)
{
	if (!tm_open)
		return;
	if (restore && (tm_prev_fg == 0 || vcon_users(tm_prev_fg) > 0))
		(void) vcon_switch(tm_prev_fg);
	tm_open = 0;
	tm_confirm_id = 0;
	vcon_overlay(0);
}

int taskmgr_key(unsigned char c)
{
	unsigned int next;

	if (!tm_open)
		return 0;
	tm_keys++;
	next = (tm_qh + 1) % TM_Q;
	if (next != tm_qt) {
		tm_q[tm_qh] = c;
		tm_qh = next;
	}
	wake_systask();
	return 1;                            /* taken, even if the queue was full: never passed to a program */
}

void taskmgr_hotkey(void)
{
	if (tm_open)
		(void) taskmgr_key(0x1b);        /* Ctrl+Esc again: like Esc */
	else
		(void) taskmgr_open();
}

void taskmgr_console_key(int con)
{
	if (tm_open) {
		tm_open = 0;
		tm_confirm_id = 0;
		(void) vcon_switch(con);         /* the overlay is still on: only the focus moves */
		vcon_overlay(0);
		return;
	}
	(void) vcon_switch(con);
}

int taskmgr_is_open(void)
{
	return tm_open;
}

int taskmgr_trap(unsigned int op)
{
	switch (op) {
	case TM_OP_OPEN:
		return taskmgr_open();
	case TM_OP_STATE:
		return tm_open ? 1 : 0;
	case TM_OP_OPENS:
		return (int) tm_opens;
	default:
		return -15;
	}
}

/* ---- SYSTASK ---------------------------------------------------------------------------------------------------- */

static int pop_key(void)
{
	unsigned int irq = irq_save();
	int c = -1;

	if (tm_qt != tm_qh) {
		c = tm_q[tm_qt];
		tm_qt = (tm_qt + 1) % TM_Q;
	}
	irq_restore(irq);
	return c;
}

static void copy_str(char *d, const char *s, unsigned int max)
{
	unsigned int i;

	for (i = 0; s[i] && i < max - 1; i++)
		d[i] = s[i];
	d[i] = 0;
}

/* Snapshot the job table (interrupts off). */
static struct tm_row tmp_row;

static void row_copy(struct tm_row *d, const struct tm_row *s)
{
	unsigned char *dp = (unsigned char *) d;
	const unsigned char *sp = (const unsigned char *) s;
	unsigned int i;

	for (i = 0; i < sizeof *d; i++)          /* byte copy: the freestanding kernel has no memcpy */
		dp[i] = sp[i];
}

static void snapshot(void)
{
	int s;

	nrows = 0;
	for (s = 0; s < JOB_MAX; s++) {
		struct tm_row *r;

		if (!jobs[s].in_use)
			continue;
		r = &rows[nrows++];
		r->id = jobs[s].id;
		r->slot = s;
		r->start = jobctl_start_tick(s);
		r->state = jobctl_state(s);
		r->flags = jobctl_flags(s);
		r->con = vcon_job_con(s);
#ifdef QDOS_SURFACES
		{
			struct gsurf *g = vsess_gfx(r->con);

			r->gw = g ? g->w : 0u;
			r->gh = g ? g->h : 0u;
		}
#endif
		r->users = vcon_users(r->con);
		r->parent = 0;
#ifdef QDOS_DYNPROC
		{
			unsigned int proc_parent_of(int slot);
			unsigned int p = proc_parent_of(s);
			r->parent = p == 0xFFFFFFFFu ? 0 : p;
		}
#endif
		copy_str(r->title, jobctl_title(s), JT_TITLE_MAX);
		copy_str(r->exe, jobctl_exe_base(s), JC2_EXE_MAX);
	}
	for (s = 0; s < nrows; s++)
		rows[s].users = jobctl_con_programs(rows[s].con);
	/* oldest first (kernel and boot jobs at the top, the job started last at the bottom: End selects it); a stable
	 * insertion sort, so jobs started in the same tick stay in slot order */
	for (s = 1; s < nrows; s++) {
		int i = s;

		row_copy(&tmp_row, &rows[s]);
		while (i > 0 && rows[i - 1].start > tmp_row.start) {
			row_copy(&rows[i], &rows[i - 1]);
			i--;
		}
		row_copy(&rows[i], &tmp_row);
	}
}

static int find_row(unsigned int id)
{
	int i;

	for (i = 0; i < nrows; i++)
		if (rows[i].id == id)
			return i;
	return -1;
}

static void put(int row, int col, const char *s, int width, unsigned int attr)
{
	int i = 0;

	for (; i < width && s[i] && col + i < TM_COLS; i++)
		scr[row * TM_COLS + col + i] = (unsigned short) ((unsigned char) s[i] | (attr << 8));
	for (; i < width && col + i < TM_COLS; i++)
		scr[row * TM_COLS + col + i] = (unsigned short) (' ' | (attr << 8));
}

static void fill(int row, unsigned int attr)
{
	put(row, 0, "", TM_COLS, attr);
}

static char *num(char *b, unsigned long v)
{
	char t[12];
	int n = 0, k = 0;

	do {
		t[n++] = (char) ('0' + v % 10u);
		v /= 10u;
	} while (v && n < 11);
	while (n)
		b[k++] = t[--n];
	b[k] = 0;
	return b;
}

static const char *state_name(unsigned char st)
{
	switch (st) {
	case JC_S_RUN: return "RUN";
	case JC_S_HELD: return "HELD";
	case JC_S_SLEEP: return "SLEEP";
	case JC_S_DEAD: return "ENDED";
	case JC_S_CONWAIT: return "INPUT";
	default: return "?";
	}
}

static void cat(char *d, const char *s, unsigned int max)
{
	unsigned int n = 0, i;

	while (d[n])
		n++;
	for (i = 0; s[i] && n < max - 1; i++)
		d[n++] = s[i];
	d[n] = 0;
}

static void set_msg(const char *a, const char *b, const char *c)
{
	tm_msg[0] = 0;
	cat(tm_msg, a, sizeof tm_msg);
	if (b)
		cat(tm_msg, b, sizeof tm_msg);
	if (c)
		cat(tm_msg, c, sizeof tm_msg);
}

#ifndef QDOS_CON_VGA
/* No display (headless builds, Tiny386): the selector's rows also go to the diagnostic stream when they change. */
static unsigned short shadow[TM_ROWS * TM_COLS];

static void diag_dump(void)
{
	int r, c, last;
	int changed = 0;

	for (r = 0; r < TM_ROWS * TM_COLS; r++)
		if (shadow[r] != scr[r]) {
			changed = 1;
			break;
		}
	if (!changed)
		return;
	for (r = 0; r < TM_ROWS; r++) {
		last = 0;
		for (c = 0; c < TM_COLS; c++) {
			shadow[r * TM_COLS + c] = scr[r * TM_COLS + c];
			if ((scr[r * TM_COLS + c] & 0xff) != ' ')
				last = c + 1;
		}
		if (last == 0)
			continue;
		diag_putc('T');
		diag_putc('M');
		diag_putc('|');
		for (c = 0; c < last; c++)
			diag_putc((unsigned char) (scr[r * TM_COLS + c] & 0xff));
		diag_putc('\n');
	}
}
#endif

/* Snapshot, keep the selection by id, compose the screen and show it - all with interrupts off, and only while the
 * selector is still open (an Alt+Fn in the keyboard IRQ may have closed it). */
static void draw(void)
{
	char b[TM_COLS + 1], n[12];
	int sel, r;
	unsigned int irq = irq_save();

	if (!tm_open) {
		irq_restore(irq);
		return;
	}
	snapshot();
	sel = find_row(tm_sel_id);
	if (sel < 0 && tm_sel_id != 0 && !tm_gone_said) {
		/* the selected job has ended: nothing is selected (an action now cannot hit another job by accident) */
		char n2[12];
		set_msg("job ", num(n2, tm_sel_id), " has ended; Up/Down selects another job");
		tm_gone_said = 1;
	}
	if (sel >= 0 && sel < tm_top)
		tm_top = sel;
	if (sel >= tm_top + TM_LIST_ROWS)
		tm_top = sel - TM_LIST_ROWS + 1;
	if (tm_top > nrows - 1)
		tm_top = nrows > 0 ? nrows - 1 : 0;

	fill(0, A_BAR);
	put(0, 1, "QDOS/386 Task Manager", 30, A_BAR);
	b[0] = 0;
	cat(b, num(n, (unsigned long) nrows), sizeof b);
	cat(b, " jobs   ticks ", sizeof b);
	cat(b, num(n, sched_ticks), sizeof b);
	put(0, 50, b, 29, A_BAR);
	fill(1, A_LIST);
	fill(2, A_HEAD);
	put(2, 1, "ID", 8, A_HEAD);
	put(2, 10, "TITLE", 28, A_HEAD);
	put(2, 39, "PROGRAM", 14, A_HEAD);
	put(2, 54, "STATE", 7, A_HEAD);
	put(2, 62, "CON", 4, A_HEAD);
	put(2, 67, "PARENT", 8, A_HEAD);
	put(2, 76, "", 4, A_HEAD);
	for (r = 0; r < TM_LIST_ROWS; r++) {
		int k = tm_top + r, row = TM_LIST_TOP + r;
		unsigned int a;
		struct tm_row *x;

		if (k >= nrows) {
			fill(row, A_LIST);
			continue;
		}
		x = &rows[k];
		a = k == sel ? A_SEL : A_LIST;
		fill(row, a);
		put(row, 1, num(n, x->id), 8, a);
		put(row, 10, x->title[0] ? x->title : "(no title)", 28, a);
		put(row, 39, x->exe, 14, a);
		put(row, 54, state_name(x->state), 7, a);
		b[0] = 0;
		cat(b, num(n, (unsigned long) x->con), sizeof b);
		if (x->users > 1)
			cat(b, "+", sizeof b);         /* shared console */
		put(row, 62, b, 4, a);
		put(row, 67, x->parent ? num(n, x->parent) : "-", 8, a);
#ifdef QDOS_SURFACES
		put(row, 76, (x->flags & JC_F_KERNEL) ? "K" : (x->flags & JC_F_PROTECTED) ? "P" : x->gw ? "G" : "", 3, a);
#else
		put(row, 76, (x->flags & JC_F_KERNEL) ? "K" : (x->flags & JC_F_PROTECTED) ? "P" : "", 3, a);
#endif
	}
	fill(21, A_LIST);
	fill(22, A_BAR);
	if (sel >= 0) {
		struct tm_row *x = &rows[sel];

		b[0] = 0;
		cat(b, x->title[0] ? x->title : x->exe, sizeof b);
		cat(b, ": console ", sizeof b);
		cat(b, num(n, (unsigned long) x->con), sizeof b);
		if (x->users > 1) {
			cat(b, ", shared by ", sizeof b);
			cat(b, num(n, (unsigned long) x->users), sizeof b);
			cat(b, " programs", sizeof b);
		}
#ifdef QDOS_SURFACES
		if (x->gw) {
			cat(b, "; GFX ", sizeof b);
			cat(b, num(n, (unsigned long) x->gw), sizeof b);
			cat(b, "x", sizeof b);
			cat(b, num(n, (unsigned long) x->gh), sizeof b);
		} else {
			cat(b, "; TEXT", sizeof b);
		}
#endif
		if (x->flags & JC_F_KERNEL)
			cat(b, "; kernel job, cannot be ended", sizeof b);
		else if (x->flags & JC_F_PROTECTED)
			cat(b, "; protected, cannot be ended", sizeof b);
		put(22, 1, b, 78, A_BAR);
	}
	fill(23, tm_confirm_id ? A_WARN : A_LIST);
	put(23, 1, tm_msg, 78, tm_confirm_id ? A_WARN : A_LIST);
	fill(24, A_BAR);
	put(24, 1, "Up/Down select  Enter go to its console  F1 end job  Esc back  Alt+Fn console", 78, A_BAR);
	vdisp_load(scr);
	vdisp_cursor(24, 79, 0);
#ifndef QDOS_CON_VGA
	diag_dump();
#endif
	tm_draws++;
	tm_last_draw = sched_ticks;
	irq_restore(irq);
}

/* The first selection: the console waiter of the console that was shown, else a job on it, else the first row. */
static void first_selection(void)
{
	unsigned int irq = irq_save();
	int w, i;

	snapshot();
	tm_sel_id = nrows > 0 ? rows[0].id : 0;
	w = vcon_waiter(tm_prev_fg);
	for (i = nrows - 1; i >= 0; i--)
		if (rows[i].con == tm_prev_fg && !(rows[i].flags & JC_F_KERNEL)) {
			tm_sel_id = rows[i].id;
			break;
		}
	for (i = 0; i < nrows; i++)
		if (rows[i].slot == w)
			tm_sel_id = rows[i].id;
	tm_top = 0;
	tm_confirm_id = 0;
	tm_gone_said = 0;
	set_msg("", 0, 0);
	irq_restore(irq);
}

static void move(int d)
{
	int i = find_row(tm_sel_id);

	tm_gone_said = 0;                    /* a new selection: its end is reported again */

	if (i < 0) {
		i = 0;
		d = 0;                           /* after the selected job ended: the first key selects the first row */
	}
	i += d;
	if (i < 0)
		i = 0;
	if (i > nrows - 1)
		i = nrows - 1;
	if (i >= 0)
		tm_sel_id = rows[i].id;
}

static void do_end(unsigned int id)
{
	char n[12];
	int rc;
	unsigned int irq = irq_save();

	rc = jobctl_kill(id);                /* refuses stale ids, protected jobs, SYSTASK itself */
	irq_restore(irq);
	if (rc == 0) {
		tm_kills++;
		tm_gone_said = 1;                /* this message, not "has ended", explains the missing row */
		set_msg("job ", num(n, id), " ended; the other jobs go on");
	} else if (rc == -2) {
		set_msg("job ", num(n, id), " no longer exists: nothing was ended");
	} else if (rc == JC_ERR_DEAD) {
		set_msg("job ", num(n, id), " had already ended");
	} else if (rc == JC_ERR_PROTECT || rc == JC_ERR_SELF) {
		set_msg("job ", num(n, id), " is protected (SYSTEM, IDLE, the shell): not ended");
	} else {
		set_msg("job ", num(n, id), ": end refused");
	}
}

static void key(int c)
{
	int i;
	char n[12];

	if (tm_confirm_id) {
		unsigned int id = tm_confirm_id;

		tm_confirm_id = 0;
		if (c == 'y' || c == 'Y')
			do_end(id);
		else
			set_msg("job ", num(n, id), " kept");
		return;
	}
	switch (c) {
	case K_UP: move(-1); break;
	case K_DOWN: move(1); break;
	case K_PGUP: move(-TM_LIST_ROWS); break;
	case K_PGDN: move(TM_LIST_ROWS); break;
	case K_HOME: move(-JOB_MAX); break;
	case K_END: move(JOB_MAX); break;
	case 0x1b: {
		unsigned int irq = irq_save();
		tm_close(1);
		irq_restore(irq);
		break;
	}
	case '\r':
	case '\n': {
		unsigned int irq = irq_save();
		int slot = job_lookup(tm_sel_id);

		if (slot == JOB_NONE || !jobs[slot].in_use) {
			irq_restore(irq);
			set_msg("job ", num(n, tm_sel_id), " no longer exists: choose again");
			break;
		}
		(void) vcon_switch(vcon_job_con(slot));
		tm_switches++;
		tm_close(0);
		irq_restore(irq);
		break;
	}
	case K_F1:
		i = find_row(tm_sel_id);
		if (i < 0) {
			set_msg("job ", num(n, tm_sel_id), " no longer exists: choose again");
			break;
		}
		if (rows[i].flags & JC_F_PROTECTED) {
			set_msg(rows[i].title, " is protected (SYSTEM, IDLE, the shell): it cannot be ended here", 0);
			break;
		}
		tm_confirm_id = rows[i].id;
		set_msg("End ", rows[i].title, "?  Y = end it, any other key = keep it");
		break;
	default:
		break;                           /* every other key is swallowed */
	}
}

#ifdef QDOS_SYNC
static unsigned long next_sync;
static unsigned int sync_gen = 0xFFFFFFFFu;
#endif

/* One SYSTASK step, called by the SYSTEM job's loop (kernel/shellboot.c) every tick: the selector's keys and screen
 * while it is open, then the periodic SYNC when it is due.  Runs in the SYSTEM job, a kernel job with its own stack -
 * never in an interrupt handler - so it can end other jobs and wait for the drive. */
void taskmgr_poll(void)
{
	int c;

	if (tm_slot < 0)
		return;
	if (tm_open) {
		int dirty = 0;

		if (tm_fresh) {
			tm_fresh = 0;
			first_selection();
			dirty = 1;
		}
		while (tm_open && (c = pop_key()) >= 0) {
			key(c);
			dirty = 1;
		}
		if (tm_open && (dirty || sched_ticks - tm_last_draw >= TM_REFRESH))
			draw();
	}
#ifdef QDOS_SYNC
	if (sync_gen != sync_interval_gen()) {           /* the interval was set: count from now */
		sync_gen = sync_interval_gen();
		next_sync = sched_ticks + (unsigned long) sync_interval() * 100ul;
	}
	if (sync_interval() != 0 && (long) (sched_ticks - next_sync) >= 0) {
		unsigned int irq = irq_save();
		(void) sync_now(1);                      /* interrupts off as in a trap */
		irq_restore(irq);
		next_sync = sched_ticks + (unsigned long) sync_interval() * 100ul;
	}
#endif
}

/* SYSTASK runs in the SYSTEM job (slot 0), which already wakes every tick to watch the shell: it takes no job slot,
 * so programs keep the 13 slots they had in R12A/R12B. */
int taskmgr_start(void)
{
	tm_slot = JOB_SYSTEM;
	return 0;
}
#else
typedef int taskmgr_off_unit;        /* an empty translation unit is not ISO C */
#endif /* QDOS_TASKMGR */
