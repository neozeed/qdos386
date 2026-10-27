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
 * "hello.aou" is HELLO.AOU.  Names are limited to 10 characters, because a
 * channel name is at most 15 characters after the WIN1_ prefix.
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

#define SH_LINE_MAX  80
#define SH_RUN_MAX_ARGS 15         /* user arguments; argv[0] is the name */
#define SH_PATH_MAX  16            /* "WIN1_" plus a name, NUL included */
#define SH_NAME_MAX  10            /* longest name the channel prefix allows */
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
	else {
		outs("error ");
		outnum(rc);
		outc('\n');
	}
}

static void cmd_help(void)
{
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

static void cmd_run(char *arg)
{
	char *p = arg, *tok, *name;
	unsigned int plen = 0, argc = 0, n, k, nlen;
	int id, st;

	if (*p == 0) {
		outs("usage: RUN <name> [arguments]\n");
		return;
	}
	name = p;
	while (*p && *p != ' ')
		p++;
	if (*p) {
		*p++ = 0;
		while (*p == ' ')
			p++;
	}
	fold(name);
	if (slen(name) > SH_NAME_MAX) {
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

	id = (int) qtrap(APP_QT_IMG_LOAD, (unsigned int) name, (unsigned int) runargs, plen);
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
		default:
			say_name_error(id);
		}
		return;
	}
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

/* Run one line.  Returns 1 for EXIT, 0 otherwise.  The line is changed in place. */
static int exec_line(char *s)
{
	char *w = s, *arg;
	unsigned int e;

	while (*w == ' ')
		w++;
	if (*w == 0)
		return 0;
	arg = w;
	while (*arg && *arg != ' ')
		arg++;
	if (*arg) {
		*arg++ = 0;
		while (*arg == ' ')
			arg++;
	}
	fold(w);                  /* only the command word; arguments keep case */
	e = slen(arg);
	while (e > 0 && arg[e - 1] == ' ')
		arg[--e] = 0;

	if (same(w, "HELP"))
		cmd_help();
	else if (same(w, "DIR"))
		cmd_dir();
	else if (same(w, "TYPE"))
		cmd_type(arg);
	else if (same(w, "RUN"))
		cmd_run(arg);
	else if (same(w, "EXIT"))
		return 1;
	else {
		outs("unknown command: ");
		outs(w);
		outc('\n');
	}
	return 0;
}

#ifndef SHELL_SELFTEST

int main(void)
{
	con_ch = (int) qtrap(APP_QT_CH_OPEN, (unsigned int) "CON", 0, 0);
	if (con_ch <= 0)
		return 1;
	outs("QDOS/386 R8 shell. Type HELP.\n");
	for (;;) {
		outs("> ");
		if (!read_line()) {
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
