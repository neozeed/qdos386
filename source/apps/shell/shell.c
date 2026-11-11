/*
 * QDOS/386 R8 - the shell (slot 0; application-owned, not part of the kernel).
 *
 * Commands:  HELP   DIR   TYPE <name>   RUN <name> [arguments]   EXIT
 *
 * RUN arguments: separated by spaces; a double-quoted argument may hold spaces;
 * there is no escape character.  argv[0] is the program name.  At most 15
 * arguments, 432 bytes of argument text including the name.  Only the command
 * word and the RUN or TYPE name are folded to upper case; arguments keep case.
 *
 * Names are 8.3 on the WIN1_ volume.  Input is folded to uppercase, so
 * "hello.aou" is HELLO.AOU.  Names are limited to 12 characters (full 8.3 with
 * its dot); the WIN1_ channel name is then at most 17 characters (R11E).
 *
 * Console input: when no byte is waiting the shell sleeps in QT_CON_WAIT and
 * an arrival wakes it.  It never spins.
 *
 * RUN: the image is loaded into slot 1 by QT_IMG_LOAD (the kernel checks the
 * size cap, the a.out format and the slot).  The shell then waits for that job
 * to end (QT_JOB_EXIT) and to release its slot (QT_JOB_RECLAIMED), reports the
 * exit status, and stays in its own slot throughout.
 *
 * Built with -DSHELL_SELFTEST (SHELLT.AOU): the same command code runs lines
 * this program supplies, captures the output, and checks it.  That is a guest
 * self-test; no key is typed or injected, and the output says so.
 *
 * End of input (no EXIT) exits with status 3.  EXIT exits with status 0.
 */
#include "appcalls.h"

#ifndef SH_LINE_MAX
#define SH_LINE_MAX  80            /* R11L volumes use -DSH_LINE_MAX=160 (SHELL.AOU of the self-hosting volume) */
#endif
#define SH_RUN_MAX_ARGS 15         /* user arguments; argv[0] is the name */
#define SH_PATH_MAX  18            /* "WIN1_" plus a name, NUL included (R11E) */
#define SH_NAME_MAX  12            /* longest 8.3 name: 8 + dot + 3 (R11E) */
#define SH_CAP_MAX   1024
#define SH_REC       16
#define SH_EOF_STATUS 3

/* Kernel results the shell reports. */
#define E_RUNNING   (-16)          /* JOB_ERR_RUNNING */
#define E_NOSTATUS  (-17)          /* JOB_ERR_NOSTATUS */
#define E_NOVOL     (-18)
#define E_AGAIN     (-19)          /* CHAN_ERR_AGAIN */
#define E_IMG_SIZE  (-20)
#define E_IMG_MAGIC (-21)
#define E_IMG_RELOC (-22)
#define E_IMG_FORMAT (-23)
#define E_IMG_LAYOUT (-24)
#define E_IMG_ENTRY (-25)
#define E_IMG_XFER  (-26)
#define E_SLOTBUSY  (-28)
#define E_NOJOB     (-29)
#define E_NOMEM     (-30)
#define E_TOOBIG    (-31)
#define E_ARGS      (-34)
#define E_NOTSLOT1  (-32)
#define E_CHAIN     (-43)
#define E_NOFILE    (-44)
#define E_NAME      (-46)
#define E_RANGE     (-47)

/* Output and input buffers.  All of them are static: the kernel reads
 * channel buffers from slot memory only. */
static int con_ch = -1;
static int capturing = 0;
static char capbuf[SH_CAP_MAX];
static unsigned int caplen = 0;
static char ob[1];
static char nb[12];
static unsigned char rbuf[1];
static char line[SH_LINE_MAX];
static char pathbuf[SH_PATH_MAX];
static unsigned char recbuf[256];
static char tbuf[128];

static unsigned int slen(const char *s)
{
	unsigned int n = 0;
	while (s[n])
		n++;
	return n;
}

static int same(const char *a, const char *b)
{
	while (*a && *a == *b) {
		a++;
		b++;
	}
	return *a == *b;
}

static void out(const char *s, unsigned int n)
{
	unsigned int i;
	if (capturing) {
		for (i = 0; i < n && caplen < SH_CAP_MAX - 1; i++)
			capbuf[caplen++] = s[i];
		capbuf[caplen] = 0;
		return;
	}
	while (n > 0) {
		unsigned int k = n > 200 ? 200 : n;
		qtrap(APP_QT_CH_WRITE, (unsigned int) con_ch, (unsigned int) s, k);
		s += k;
		n -= k;
	}
}

static void outs(const char *s)
{
	out(s, slen(s));
}

static void outc(char c)
{
	ob[0] = c;
	out(ob, 1);
}

static void outnum(long v)
{
	int n = 0;
	unsigned long u;
	if (v < 0) {
		outc('-');
		u = (unsigned long) (-v);
	} else {
		u = (unsigned long) v;
	}
	if (u == 0)
		nb[n++] = '0';
	while (u != 0) {
		nb[n++] = (char) ('0' + (int) (u % 10));
		u /= 10;
	}
	while (n > 0)
		outc(nb[--n]);
}

/* One byte from CON.  Sleeps while nothing is waiting.  Returns -1 at end. */
static int con_getc(void)
{
	volatile unsigned char wake;
	int r;
	for (;;) {
		r = qtrap(APP_QT_CH_READ, (unsigned int) con_ch, (unsigned int) rbuf, 1);
		if (r == 1)
			return rbuf[0];
		if (r == 0)
			return -1;
		if (r != E_AGAIN)
			return -1;
		wake = 0;
		if (qtrap(APP_QT_CON_WAIT, 0, (unsigned int) &wake, 0) < 0)
			qtrap(APP_QT_SUSPEND, 1, 0, 0);   /* refused: back off one tick */
	}
}

/* Read one line into line[].  Echoes; Backspace or Delete edits.  Returns 0 at
 * end of input. */
static int read_line(void)
{
	unsigned int n = 0;
	int c;
	for (;;) {
		c = con_getc();
		if (c < 0)
			return 0;
		if (c == '\r' || c == '\n') {
			outc('\n');
			line[n] = 0;
			return 1;
		}
		if (c == 0x08 || c == 0x7f) {
			if (n > 0) {
				n--;
				outs("\b \b");
			}
			continue;
		}
		if (c >= 32 && c < 127 && n < SH_LINE_MAX - 1) {
			line[n++] = (char) c;
			outc((char) c);
		}
	}
}

static void fold(char *s)
{
	for (; *s; s++)
		if (*s >= 'a' && *s <= 'z')
			*s = (char) (*s - 32);
}

/* Messages for a failed channel open or read of a WIN1_ name. */
static void say_name_error(int rc)
{
	if (rc == E_NOFILE)
		outs("no such file\n");
	else if (rc == E_NAME || rc == E_RANGE)
		outs("invalid name\n");
	else if (rc == E_CHAIN)
		outs("bad cluster chain\n");
	else if (rc == E_NOVOL)
		outs("volume not mounted\n");
#ifdef R13B_SHELL
	else if (rc == -87)
		outs("not a directory (a part of the path is a file)\n");
	else if (rc == -88)
		outs("is a directory, not a program or file\n");
	else if (rc == -90)
		outs("name too long (with its directory)\n");
#endif
	else {
		outs("error ");
		outnum(rc);
		outc('\n');
	}
}

static void cmd_help(void)
{
#ifdef R11M_SHELL
	outs("HELP                 this list\n");
	outs("DIR [path] [/W] [/P] list a directory; /W columns, /P pause (<DIR> marks subdirectories)\n");
	outs("HISTORY              list the last commands (Up/Down recall, Left/Right/Home/End/Del edit)\n");
	outs("PWD / CD [path]      show / change the current directory\n");
	outs("MD path / RD path    make / remove (empty) directory\n");
	outs("TYPE path            print a file\n");
	outs("COPY src dst [/Y]    copy a file (dst may be a directory; /Y replaces)\n");
	outs("DEL path / REN a b   delete / rename (or move) a file\n");
#ifdef R12B_SHELL
	outs("RUN name [args]      run a program (a bare name: cwd, then each PATH entry, then /; .AOU may be left out)\n");
#else
	outs("RUN path [args]      run an a.out (cwd, then /BIN)\n");
#endif
#ifdef R11M_JOBS
	outs("START path [args] [/CON:n]   run in the background on console n (default: this one); prints the job id\n");
	outs("PS                   list jobs (ENDED rows: started jobs that finished, with their status)\n");
	outs("WAIT id              wait for a started job; Ctrl+C stops waiting\n");
	outs("KILL id | /CON:n     terminate one started job / every started job on console n\n");
	outs("CON                  show the consoles, the visible one (*) and the jobs on each\n");
	outs("CON n                make console n visible (Alt+F1..F4 does the same from the keyboard)\n");
	outs("CONDUMP n            print the text of console n\n");
	outs("DIAG 0|1|2           diagnostic mirror: console 0 / none / all consoles\n");
	outs("SLEEP ticks          pause the shell\n");
#endif
#ifdef R12B_SHELL
	outs("SET [NAME[=value]]   list / show / set environment variables (SET NAME= sets an empty value)\n");
	outs("UNSET NAME           remove a variable;  ECHO text   print text;  %NAME% in a line is replaced by its value\n");
	outs("CALL script          run the commands in a file (also SOURCE); REM starts a comment line\n");
#ifdef R13B_SHELL
	outs("name [args]          not a command: run program name like RUN (\"quoted name\" for spaces)\n");
	outs("CLS                  clear this session's screen;  DATE / TIME   show the clock (UTC)\n");
#endif
#ifdef R13A_SHELL
	outs("FONT [path|n] [/CON|/SURF]  list fonts / load a QFNT font and use it (SET FONT=path too)\n");
	outs("CONSOLE [GFX|VGA] [/ALL]    rendered text console (640x480, the font) or VGA text mode\n");
#endif
#ifdef R13_SHELL
	outs("START ... /NEW       start in a new private session (any number: Ctrl+Esc selects them)\n");
	outs("CON / CON n          list the sessions (TEXT or GFX) / show session n (0..19; 0..3 also Alt+F1..F4)\n");
#endif
#ifdef R12C_SHELL
	outs("TASKS [/L]           task selector (also Ctrl+Esc) / list every job with its title\n");
	outs("SYNC [/STATUS]       write pending file data, flush the drive cache / counters\n");
	outs("SYNC /INTERVAL:s     sync every s seconds, 0 = off (also SET SYNC_INTERVAL=s)\n");
#endif
#endif
	outs("EXIT                 leave the shell\n");
	return;
#endif
	outs("HELP          this list\n");
	outs("DIR           list the files on the data disk\n");
	outs("TYPE <name>   print a file\n");
	outs("RUN <name> [arguments]   run an a.out program (slot 1), show its status\n");
	outs("EXIT          leave the shell\n");
}

static void cmd_dir(void)
{
	int ch, n, i, files = 0;
	unsigned int c;

	ch = (int) qtrap(APP_QT_CH_OPEN, (unsigned int) "WIN1_", 0, 0);
	if (ch <= 0) {
		say_name_error(ch);
		return;
	}
	for (;;) {
		n = (int) qtrap(APP_QT_CH_READ, (unsigned int) ch, (unsigned int) recbuf, sizeof recbuf);
		if (n <= 0)
			break;
		for (i = 0; i + SH_REC <= n; i += SH_REC) {
			unsigned long size;
			unsigned int col = 0;
			for (c = 0; c < 12 && recbuf[i + c] != 0; c++) {
				outc((char) recbuf[i + c]);
				col++;
			}
			while (col < 14) {
				outc(' ');
				col++;
			}
			size = (unsigned long) recbuf[i + 12] | ((unsigned long) recbuf[i + 13] << 8) |
			       ((unsigned long) recbuf[i + 14] << 16) | ((unsigned long) recbuf[i + 15] << 24);
			outnum((long) size);
			outc('\n');
			files++;
		}
	}
	qtrap(APP_QT_CH_CLOSE, (unsigned int) ch, 0, 0);
	if (n < 0)
		say_name_error(n);
	outnum(files);
	outs(files == 1 ? " file\n" : " files\n");
}

/* Build "WIN1_<name>" in pathbuf.  Returns 0, or -1 if the name is unusable. */
static int make_path(const char *name)
{
	unsigned int k = 0, i;
	const char *p = "WIN1_";
	if (slen(name) == 0 || slen(name) > SH_NAME_MAX)
		return -1;
	for (i = 0; p[i]; i++)
		pathbuf[k++] = p[i];
	for (i = 0; name[i]; i++)
		pathbuf[k++] = name[i];
	pathbuf[k] = 0;
	return 0;
}

static void cmd_type(char *name)
{
	int ch, n;
	fold(name);
	if (slen(name) == 0) {
		outs("usage: TYPE <name>\n");
		return;
	}
	if (make_path(name) != 0) {
		outs("invalid name\n");
		return;
	}
	ch = (int) qtrap(APP_QT_CH_OPEN, (unsigned int) pathbuf, 0, 0);
	if (ch <= 0) {
		say_name_error(ch);
		return;
	}
	for (;;) {
		n = (int) qtrap(APP_QT_CH_READ, (unsigned int) ch, (unsigned int) tbuf, sizeof tbuf);
		if (n <= 0)
			break;
		out(tbuf, (unsigned int) n);
	}
	qtrap(APP_QT_CH_CLOSE, (unsigned int) ch, 0, 0);
	if (n < 0)
		say_name_error(n);
}

/* Wait for a job to end and for its slot to be released.  Returns its status. */
static int wait_job(int id)
{
	int st;
	for (;;) {
		st = (int) qtrap(APP_QT_JOB_EXIT, (unsigned int) id, 0, 0);
		if (st != E_RUNNING)
			break;
		qtrap(APP_QT_SUSPEND, 1, 0, 0);
	}
	while (qtrap(APP_QT_JOB_RECLAIMED, (unsigned int) id, 0, 0) == 0)
		qtrap(APP_QT_SUSPEND, 1, 0, 0);
	return st;
}

/* Packed argument strings for RUN: the arguments after the name, each
 * NUL-terminated.  The kernel adds argv[0] (the name) itself. */
static unsigned char runargs[APP_ARG_STR_MAX];
#ifdef R13B_SHELL
static int run_direct, run_notfound;        /* R13B: a program run by its name as the command word */
#endif
#ifdef R11M_JOBS
static int run_bg;                          /* START: report the job id and return without waiting */
static int run_con = -1;                    /* START /CON:n */
#ifdef R13_SHELL
static int job_session(unsigned long id);   /* shell_jobs.inc: the session a job is on, or -1 */
static int run_new;                         /* START /NEW */
#endif
static int pg_known, pg_ok;

/* The jobs this shell started (so PS can still name them after they ended, and WAIT knows which ids are its own). */
#ifndef SJ_MAX
#define SJ_MAX 8
#endif
static struct sh_job {
	int id;
	int con;
	int done;                              /* the end was seen: status is valid (-2 = lost before it was seen) */
	int status;
	char name[16];
} jt[SJ_MAX];
static int jt_n;

/* Record the end of every started job that has ended.  The kernel keeps an exit record only until its slot is reused,
 * so this runs before every command (and before a START takes a slot). */
static void jt_harvest(void)
{
	int i, st;

	for (i = 0; i < jt_n; i++) {
		if (jt[i].done)
			continue;
		st = (int) qtrap(APP_QT_JOB_EXIT, (unsigned int) jt[i].id, 0, 0);
		if (st == E_RUNNING)
			continue;
		jt[i].done = 1;
		jt[i].status = st;                 /* -2 if the slot was reused before the end was seen */
	}
}

static void jt_add(int id, const char *name)
{
	int i, k;
	const char *b = name;

	if (jt_n == SJ_MAX) {                  /* forget the oldest */
		for (i = 1; i < SJ_MAX; i++)
			jt[i - 1] = jt[i];
		jt_n--;
	}
	for (k = 0; name[k]; k++)
		if (name[k] == '/' || name[k] == ':')
			b = name + k + 1;
	jt[jt_n].id = id;
	jt[jt_n].con = run_con;
	jt[jt_n].done = 0;
	jt[jt_n].status = 0;
	for (k = 0; b[k] && k < 15; k++)
		jt[jt_n].name[k] = b[k];
	jt[jt_n].name[k] = 0;
	jt_n++;
}

/* RUN loads into the shared slot 1 and waits.  START uses the private paged image window when the kernel has one, so
 * several programs linked for the same origin can run at once; otherwise it falls back to the slot 1 loader. */
static int load_prog(const char *name, unsigned int plen)
{
	if (run_bg) {
		if (!pg_known) {
			pg_known = 1;
			pg_ok = (int) qtrap(APP_QT_PG_CAPS, 0, 0, 0) >= 0;
		}
		if (pg_ok)
			return (int) qtrap(APP_QT_PG_START, (unsigned int) name, (unsigned int) runargs, plen);
	}
	return (int) qtrap(APP_QT_IMG_LOAD, (unsigned int) name, (unsigned int) runargs, plen);
}
#else
static int load_prog(const char *name, unsigned int plen)
{
	return (int) qtrap(APP_QT_IMG_LOAD, (unsigned int) name, (unsigned int) runargs, plen);
}
#endif

static void cmd_run(char *arg)
{
	char *p = arg, *tok, *name;
	unsigned int plen = 0, argc = 0, n, k, nlen;
	int id, st;

	if (*p == 0) {
		outs("usage: RUN <name> [arguments]\n");
		return;
	}
#ifdef R11M_SHELL
	if (*p == '"') {                           /* RUN "Long Name.aou" arguments */
		name = ++p;
		while (*p && *p != '"')
			p++;
		if (*p == 0) {
			outs("unterminated quote\n");
			return;
		}
		*p++ = 0;
		while (*p == ' ')
			p++;
	} else
#endif
	{
		name = p;
		while (*p && *p != ' ')
			p++;
		if (*p) {
			*p++ = 0;
			while (*p == ' ')
				p++;
		}
	}
#ifndef R12B_SHELL
	fold(name);                                /* R12B: the name keeps its case (the volume is searched without regard to case) */
#endif
#ifdef R11M_SHELL
	if (slen(name) > 64) {
#else
	if (slen(name) > SH_NAME_MAX) {
#endif
		outs("invalid name\n");
		return;
	}
	nlen = slen(name) + 1;                     /* argv[0] with its NUL */

	while (*p) {
		if (argc + 2 > APP_ARG_MAX) {          /* argv[0] counts too */
			outs("too many arguments (limit 15)\n");
			return;
		}
		if (*p == '"') {
			tok = ++p;
			while (*p && *p != '"')
				p++;
			if (*p == 0) {
				outs("unterminated quote\n");
				return;
			}
			n = (unsigned int) (p - tok);
			p++;                              /* the closing quote */
		} else {
			tok = p;
			while (*p && *p != ' ')
				p++;
			n = (unsigned int) (p - tok);
		}
		if (plen + n + 1 + nlen > APP_ARG_STR_MAX) {
			outs("arguments too long (limit 432 bytes with the name)\n");
			return;
		}
		for (k = 0; k < n; k++)
			runargs[plen + k] = (unsigned char) tok[k];
		runargs[plen + n] = 0;
		plen += n + 1;
		argc++;
		while (*p == ' ')
			p++;
	}

	id = load_prog(name, plen);
#if defined(R11M_SHELL) && !defined(R12B_SHELL)
	if (id == E_NOFILE) {                      /* not in the current directory: try /BIN (R12B: the kernel searches PATH) */
		unsigned int q;
		for (q = 0; name[q]; q++)
			if (name[q] == '/')
				break;
		if (name[q] == 0 && slen(name) + 5 <= 64) {
			static char binname[72];
			unsigned int bi = 0, bj;
			const char *bp = "/BIN/";
			for (bj = 0; bp[bj]; bj++)
				binname[bi++] = bp[bj];
			for (bj = 0; name[bj]; bj++)
				binname[bi++] = name[bj];
			binname[bi] = 0;
			id = load_prog(binname, plen);
		}
	}
#endif
	if (id < 0) {
		switch (id) {
		case E_TOOBIG:
			outs("too large: over the loader file limit\n");
			break;
		case E_NOTSLOT1:
			outs("not a slot 1 program (RUN loads slot 1 only)\n");
			break;
		case E_IMG_LAYOUT:
			outs("not a valid a.out image (code -24): text, data and BSS are over the slot limit\n");
			break;
		case E_ARGS:
			outs("arguments refused by the loader (code -34)\n");
			break;
		case E_IMG_SIZE:
		case E_IMG_MAGIC:
		case E_IMG_RELOC:
		case E_IMG_FORMAT:
		case E_IMG_ENTRY:
		case E_IMG_XFER:
			outs("not a valid a.out image (code ");
			outnum(id);
			outs(")\n");
			break;
		case E_SLOTBUSY:
			outs("slot 1 is busy\n");
			break;
		case E_NOJOB:
			outs("no free job slot\n");
			break;
		case E_NOMEM:
			outs("out of memory\n");
			break;
#ifdef R11M_JOBS
		case -60:
#ifdef R12A_SHELL
			outs("out of memory: no free page frames (code -60)\n");
#else
			outs("no free image window (limit of started programs reached)\n");
#endif
			break;
		case -61:
			outs("not linked for the standard origin: cannot be started in the background\n");
			break;
		case -63:
			outs("no free address space (limit of started programs reached)\n");
			break;
#endif
		default:
#ifdef R13B_SHELL
			if (run_direct && id == E_NOFILE) {
				outs("unknown command or program: ");
				outs(name);
				outs(" (not built in; not found here or in PATH)\n");
				run_notfound = 1;
				break;
			}
#endif
			say_name_error(id);
		}
		return;
	}
#ifdef R11M_JOBS
	if (run_bg) {
		jt_add(id, name);
#ifdef R13_SHELL
		{
			int sc = (int) qtrap(APP_QT_CH_CTL, (unsigned int) con_ch, 0x13u, 3u) & 0xff;
			int jc = job_session((unsigned long) id);

			outs("job ");
			outnum(id);
			outs(" started on session ");
			outnum(jc);
			if (jc == sc)
				outs(run_new ? " (this session: no free private session)" : " (this session; /NEW gives it its own)");
			else if (jc >= 0 && jc < 4) {
				outs(" (Alt+F");
				outnum(jc + 1);
				outs(" shows it)");
			} else {
				outs(" (Ctrl+Esc or CON ");
				outnum(jc);
				outs(" shows it)");
			}
			outc('\n');
		}
		if (1)
			return;
#endif
		{
			int sc = (int) qtrap(APP_QT_CH_CTL, (unsigned int) con_ch, 0x13u, 3u) & 0xff;
			int jc = run_con >= 0 ? run_con : sc;

			outs("job ");
			outnum(id);
			outs(" started on console ");
			outnum(jc);
			if (jc != sc) {
				outs(" (Alt+F");
				outnum(jc + 1);
				outs(" shows it)");
			} else {
				outs(" (this console; /CON:n gives it its own)");
			}
			outc('\n');
		}
		return;
	}
#endif
	st = wait_job(id);
	if (st >= 0) {
		outs("exit status ");
		outnum(st);
		outc('\n');
	} else if (st == E_NOSTATUS) {
		outs("program ended without an exit status\n");
	} else {
		outs("program status error ");
		outnum(st);
		outc('\n');
	}
}

#ifdef R11M_SHELL
#include "shell_r11m.inc"
#include "shell_edit.inc"
#endif
#ifdef R11M_JOBS
#include "shell_jobs.inc"
#endif
#ifdef R12B_SHELL
static int exec_line(char *s);
#ifdef R12C_SHELL
static void sync_interval_hook(const char *name, const char *value);
#endif
#ifdef R13A_SHELL
static void font_env_hook(const char *name, const char *value);
#endif
#include "shell_env.inc"
#endif
#ifdef R12C_SHELL
#include "shell_r12c.inc"
#endif
#ifdef R13A_SHELL
#include "shell_r13a.inc"
#endif
#ifdef R13B_SHELL
#include "shell_r13b.inc"
#endif

#ifdef R13B_SHELL
static char word0[SH_LINE_MAX];             /* the command word as typed (a program name keeps its case) */
#endif

/* Run one line.  Returns 1 for EXIT, 0 otherwise.  The line is changed in place. */
static int exec_line(char *s)
{
	char *w = s, *arg;
	unsigned int e;

#ifdef R12B_SHELL
	sh_fatal = 0;
	if (sh_expand(s, xline, SH_LINE_MAX, sh_lookup) != 0) {
		outs("line too long after %NAME% expansion (limit ");
		outnum(SH_LINE_MAX - 1);
		outs(" characters): not run\n");
		sh_fatal = 1;
		return 0;
	}
	w = xline;
#endif
	while (*w == ' ')
		w++;
	if (*w == 0)
		return 0;
#ifdef R13B_SHELL
	if (*w == '"') {          /* R13B: "a quoted program name" [arguments] */
		jt_harvest();
		exit_warned = 0;
		r13b_run_word(w, "");
		if (run_notfound)
			sh_fatal = 1;
		run_notfound = 0;
		return 0;
	}
#endif
	arg = w;
	while (*arg && *arg != ' ')
		arg++;
	if (*arg) {
		*arg++ = 0;
		while (*arg == ' ')
			arg++;
	}
#ifdef R13B_SHELL
	{
		unsigned int q;

		for (q = 0; w[q] && q < SH_LINE_MAX - 1; q++)
			word0[q] = w[q];
		word0[q] = 0;
	}
#endif
	fold(w);                  /* only the command word; arguments keep case */
#ifdef R11M_JOBS
	jt_harvest();
	if (!same(w, "EXIT"))
		exit_warned = 0;
#endif
	e = slen(arg);
	while (e > 0 && arg[e - 1] == ' ')
		arg[--e] = 0;

	if (same(w, "HELP"))
		cmd_help();
#ifdef R11M_SHELL
	else if (same(w, "DIR"))
		cmd_dir_m(arg);
	else if (same(w, "TYPE"))
		cmd_type_m(uq(arg));
	else if (same(w, "PWD"))
		cmd_pwd();
	else if (same(w, "CD") || same(w, "CHDIR"))
		cmd_cd(uq(arg));
	else if (same(w, "HISTORY"))
		cmd_history();
	else if (same(w, "MD") || same(w, "MKDIR"))
		cmd_name_op(FS_MKDIR, uq(arg), "usage: MD <directory>\n");
	else if (same(w, "RD") || same(w, "RMDIR"))
		cmd_name_op(FS_RMDIR, uq(arg), "usage: RD <directory>\n");
	else if (same(w, "DEL") || same(w, "ERASE"))
		cmd_del(uq(arg));
	else if (same(w, "REN") || same(w, "RENAME"))
		cmd_ren(arg);
	else if (same(w, "COPY"))
		cmd_copy(arg);
#else
	else if (same(w, "DIR"))
		cmd_dir();
	else if (same(w, "TYPE"))
		cmd_type(arg);
#endif
#ifdef R11M_JOBS
	else if (same(w, "START"))
		cmd_start(arg);
	else if (same(w, "CON"))
		cmd_con(arg);
	else if (same(w, "CONDUMP"))
		cmd_condump(arg);
	else if (same(w, "DIAG"))
		cmd_diag(arg);
	else if (same(w, "PS"))
		cmd_ps();
	else if (same(w, "WAIT"))
		cmd_wait(arg);
	else if (same(w, "KILL"))
		cmd_kill(arg);
	else if (same(w, "SLEEP"))
		cmd_sleep(arg);
#endif
	else if (same(w, "RUN"))
		cmd_run(arg);
#ifdef R12B_SHELL
	else if (same(w, "SET"))
		cmd_set(arg);
	else if (same(w, "UNSET"))
		cmd_unset(arg);
	else if (same(w, "ECHO"))
		cmd_echo(arg);
	else if (same(w, "CALL") || same(w, "SOURCE"))
		cmd_call(arg);
	else if (same(w, "REM"))
		;
#endif
#ifdef R12C_SHELL
	else if (same(w, "TASKS"))
		cmd_tasks(arg);
	else if (same(w, "SYNC"))
		cmd_sync(arg);
#endif
#ifdef R13A_SHELL
	else if (same(w, "FONT"))
		cmd_font(arg);
	else if (same(w, "CONSOLE"))
		cmd_console(arg);
#endif
#ifdef R13B_SHELL
	else if (same(w, "CLS"))
		cmd_cls(arg);
	else if (same(w, "DATE"))
		cmd_datetime(arg, 1);
	else if (same(w, "TIME"))
		cmd_datetime(arg, 0);
#endif
	else if (same(w, "EXIT")) {
#ifdef R12B_SHELL
		if (sc_depth > 0) {
			sc_exit = 1;
			return 0;
		}
#endif
#ifdef R11M_JOBS
		if (exit_blocked())
			return 0;
#endif
		return 1;
	}
	else {
#ifdef R13B_SHELL
		r13b_run_word(word0, arg);        /* R13B: not a built-in: a program, found like RUN finds it */
		if (run_notfound)
			sh_fatal = 1;
		run_notfound = 0;
#else
		outs("unknown command: ");
		outs(w);
		outc('\n');
#ifdef R12B_SHELL
		sh_fatal = 1;
#endif
#endif
	}
	return 0;
}

#ifndef SHELL_SELFTEST

int main(void)
{
	con_ch = (int) qtrap(APP_QT_CH_OPEN, (unsigned int) "CON", 0, 0);
	if (con_ch <= 0)
		return 1;
#if defined(R13B_DYN)
	/* R13B: a dynamic shell (SH.AOU) inherits the environment and directory of its creator; STARTUP.CMD is the boot
	 * shell's alone. */
	outs("QDOS/386 R13B shell (SH: a program; EXIT ends only this shell). Type HELP.\n");
#elif defined(R13B_SHELL)
	startup();
	outs("QDOS/386 R13B shell. Type HELP.\n");
#elif defined(R13A_SHELL)
	startup();
	outs("QDOS/386 R13A shell. Type HELP.\n");
#elif defined(R13_SHELL)
	startup();
	outs("QDOS/386 R13 shell. Type HELP.\n");
#elif defined(R12C_SHELL)
	startup();
	outs("QDOS/386 R12C shell. Type HELP.\n");
#elif defined(R12B_SHELL)
	startup();
	outs("QDOS/386 R12B shell. Type HELP.\n");
#elif defined(R11M_SHELL)
	outs("QDOS/386 R11M shell. Type HELP.\n");
#else
	outs("QDOS/386 R8 shell. Type HELP.\n");
#endif
	for (;;) {
#ifdef R11M_SHELL
		get_cwd();
		outs(cwdbuf);
#endif
		outs("> ");
#ifdef R11M_SHELL
		if (ed_mode < 0)
			ed_probe();
		if (!(ed_mode ? read_line_ed() : read_line())) {
#else
		if (!read_line()) {
#endif
			outs("end of input\n");
			return SH_EOF_STATUS;
		}
		if (exec_line(line))
			return 0;
	}
}

#else

/* ---- guest self-test (SHELLT.AOU) ---------------------------------------- */

static int checks = 0, fails = 0;
static char selbuf[SH_LINE_MAX];

static int has(const char *needle)
{
	unsigned int i, k, n = slen(needle);
	for (i = 0; i + n <= caplen; i++) {
		for (k = 0; k < n && capbuf[i + k] == needle[k]; k++)
			;
		if (k == n)
			return 1;
	}
	return 0;
}

/* Run a command text as if typed, with its output captured. */
static int run_text(const char *text)
{
	unsigned int i;
	int r;
	for (i = 0; text[i] && i < SH_LINE_MAX - 1; i++)
		selbuf[i] = text[i];
	selbuf[i] = 0;
	capturing = 1;
	caplen = 0;
	capbuf[0] = 0;
	r = exec_line(selbuf);
	capturing = 0;
	return r;
}

static void check(const char *name, int ok)
{
	checks++;
	if (!ok)
		fails++;
	outs(ok ? "CHECK shell: " : "CHECK shell FAIL: ");
	outs(name);
	outc('\n');
}

static int selftest(void)
{
	int r;

	run_text("HELP");
	check("HELP lists the commands", has("DIR") && has("RUN") && has("TYPE"));

	run_text("DIR");
	check("DIR lists the regular files", has("README.TXT") && has("HELLO.AOU") &&
	      has("CHAIN.AOU") && has("EMPTY.TXT") && has("10 files"));
	check("DIR hides the deleted, label and subdirectory entries",
	      !has("GONE") && !has("WIN1") && !has("SUBDIR") && !has("TRAILER"));

	run_text("TYPE README.TXT");
	check("TYPE prints a file", has("WIN1 volume, read only."));
	run_text("TYPE NOPE.TXT");
	check("TYPE of a missing file: no such file", has("no such file"));
	run_text("TYPE");
	check("TYPE without a name: usage", has("usage: TYPE"));

	run_text("BOGUS");
	check("unknown command reported", has("unknown command: BOGUS"));

	run_text("RUN NOPE.AOU");
	check("RUN of a missing file: no such file", has("no such file"));
	run_text("RUN BAD.AOU");
	check("RUN of a non-a.out file refused", has("not a valid a.out image"));
	run_text("RUN BIG.AOU");
	check("RUN of an 8200-byte non-a.out file: not a valid a.out (code -21)",
	      has("not a valid a.out image (code -21)"));
	run_text("RUN SLOT0.AOU");
	check("RUN of a slot 0 image refused", has("not a slot 1 program"));
	run_text("RUN EMPTY.TXT");
	check("RUN of an empty file refused", has("not a valid a.out image"));
	run_text("RUN CHAIN.AOU");
	check("RUN of a file with a bad chain refused", has("bad cluster chain"));
	run_text("RUN ABCDEFGHIJKLMNOP");
	check("RUN of an overlong name refused", has("invalid name"));

	run_text("RUN HELLO.AOU");
	check("RUN HELLO.AOU: exit status 7", has("exit status 7"));
	run_text("RUN hello.aou");
	check("RUN again without a reboot, lower case: exit status 7", has("exit status 7"));
	run_text("RUN PREP.AOU");
	check("RUN PREP.AOU: exit status 42", has("exit status 42"));
	run_text("RUN HELLO.AOU");
	check("RUN after another program: exit status 7", has("exit status 7"));

	r = run_text("EXIT");
	check("EXIT leaves the shell", r == 1);

	outs("SHELL SELFTEST: ");
	outnum(checks);
	outs(" checks, ");
	outnum(fails);
	outs(" failed\n");
	if (fails == 0) {
		outs("QDOS/386 R8 SHELL SELFTEST PASS\n");
		return 0;
	}
	return 1;
}

int main(void)
{
	con_ch = (int) qtrap(APP_QT_CH_OPEN, (unsigned int) "CON", 0, 0);
	if (con_ch <= 0)
		return 1;
	return selftest();
}

#endif
