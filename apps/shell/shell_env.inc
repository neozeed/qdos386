/* R12B shell (-DR12B_SHELL): the process environment (SET, UNSET, ECHO, %NAME% expansion).
 * See docs/R12B-ENV-PROCESS-ABI.md section 5 for the exact rules.  Included by shell.c. */

#define ENV_E_NOMEM   (-3)
#define ENV_E_RANGE   (-4)
#define ENV_E_FULL    (-5)
#define ENV_E_ABSENT  (-7)
#define ENV_E_NAME   (-15)
#define ENV_NAME_LIM  31
#include "shell_parse.inc"

static char xline[SH_LINE_MAX];            /* the line after expansion */
static char envv[260];                     /* one value or one "NAME=value" entry */
static int sh_fatal;                       /* set by errors that end a script (unknown command, overflow, ...) */

static void say_env_error(int rc)
{
	switch (rc) {
	case ENV_E_NAME:  outs("invalid variable name (letters, digits, _; 1-31 characters; not starting with a digit)\n"); break;
	case ENV_E_RANGE: outs("value too long (limit 255 characters)\n"); break;
	case ENV_E_FULL:  outs("environment full (limit 32 variables, 2048 bytes)\n"); break;
	case ENV_E_NOMEM: outs("out of memory\n"); break;
	case ENV_E_ABSENT: outs("not set\n"); break;
	default:
		outs("environment not available (code ");
		outnum(rc);
		outs(")\n");
	}
}

static int sh_lookup(const char *name, char *buf, unsigned int size)
{
	return (int) qtrap(APP_QT_ENV_GET, (unsigned int) name, (unsigned int) buf, size);
}

static void env_list(void)
{
	unsigned int i;
	int r, any = 0;

	for (i = 0; i < 64; i++) {
		r = (int) qtrap(APP_QT_ENV_LIST, i, (unsigned int) envv, sizeof envv);
		if (r == ENV_E_ABSENT)
			break;
		if (r < 0) {
			say_env_error(r);
			return;
		}
		outs(envv);
		outc('\n');
		any = 1;
	}
	if (!any)
		outs("(no environment variables)\n");
}

/* SET                 list every variable (name order)
 * SET NAME            show NAME=value, or "NAME is not set"
 * SET NAME=value      set (spaces around = are allowed: SET NAME = value); SET NAME= sets an empty value
 * SET NAME="value"    the outer quotes are removed; spaces, ; and = inside are kept */
static void cmd_set(char *arg)
{
	char *n = arg, *p, *v;
	unsigned int nl, vl;
	int rc;

	if (*arg == 0) {
		env_list();
		return;
	}
	p = n;
	while (*p && *p != '=' && *p != ' ')
		p++;
	nl = (unsigned int) (p - n);
	while (*p == ' ')
		p++;
	if (*p == 0) {                                 /* query */
		n[nl] = 0;
		rc = (int) qtrap(APP_QT_ENV_GET, (unsigned int) n, (unsigned int) envv, sizeof envv);
		if (rc >= 0) {
			fold(n);
			outs(n);
			outc('=');
			outs(envv);
			outc('\n');
		} else if (rc == ENV_E_ABSENT) {
			fold(n);
			outs(n);
			outs(" is not set\n");
		} else
			say_env_error(rc);
		return;
	}
	if (*p != '=' || nl == 0) {
		outs("usage: SET [NAME[=value]]\n");
		return;
	}
	v = p + 1;
	while (*v == ' ')
		v++;
	vl = slen(v);
	if (*v == '"') {
		if (vl < 2 || v[vl - 1] != '"') {
			outs("unterminated quote\n");
			return;
		}
		v[vl - 1] = 0;
		v++;
	}
	n[nl] = 0;
	rc = (int) qtrap(APP_QT_ENV_SET, (unsigned int) n, (unsigned int) v, 0);
	if (rc != 0)
		say_env_error(rc);
#ifdef R12C_SHELL
	else
		sync_interval_hook(n, v);
#endif
#ifdef R13A_SHELL
	if (rc == 0)
		font_env_hook(n, v);
#endif
}

static void cmd_unset(char *arg)
{
	int rc;

	if (*arg == 0) {
		outs("usage: UNSET NAME\n");
		return;
	}
	rc = (int) qtrap(APP_QT_ENV_SET, (unsigned int) arg, 0, 0);
#ifdef R12C_SHELL
	if (rc == 0)
		sync_interval_hook(arg, 0);
#endif
	if (rc == ENV_E_ABSENT) {
		fold(arg);
		outs(arg);
		outs(" is not set\n");
	} else if (rc != 0)
		say_env_error(rc);
}

static void cmd_echo(char *arg)
{
	outs(arg);
	outc('\n');
}

/* ---- R12B Stage 2: scripts (CALL / SOURCE) and /ETC/STARTUP.CMD ------------------------------------------------------
 * A script is read whole (at most SC_MAX bytes) before its first line runs, so nothing changes under it and a read
 * error runs nothing.  Lines end with LF or CR LF; the last line may lack its newline; blank lines and REM lines are
 * skipped; a TAB counts as a space; any other control byte stops the script before it runs.  Each line goes through the
 * same dispatcher as a typed line (exec_line, with %NAME% expansion).  A line over 159 characters, an unknown command,
 * an expansion overflow or the nesting limit (SC_DEPTH) ends the script (and the scripts that called it) with an
 * error; any other failing command reports and the script goes on.  EXIT in a script ends that script (status 0). */
#define SC_MAX   4096
#define SC_DEPTH 4
static char sc_buf[SC_DEPTH][SC_MAX + 1];
static int sc_depth;
static int sc_exit;                         /* EXIT seen in the running script */
static int sc_lines;                        /* lines run by the last top-level script */

/* Run the script at path.  0 = ran to the end (or EXIT), -1 = stopped by an error, -44 etc. = could not be read. */
static int run_script(const char *path)
{
	char lb[SH_LINE_MAX];
	char *b;
	int ch, n, total = 0, lineno = 0, rc = 0, r;
	unsigned int i, pos;

	if (sc_depth >= SC_DEPTH) {
		outs("scripts nested too deeply (limit 4)\n");
		sh_fatal = 1;
		return -1;
	}
	if (native_to(npath2, path) != 0)
		return -90;
	if ((int) qtrap(APP_QT_FS_OP, FS_STAT, (unsigned int) npath2, (unsigned int) statbuf) == 1)
		return -88;                                  /* a directory */
	ch = (int) qtrap(APP_QT_CH_OPEN, (unsigned int) npath2, 0, 0);
	if (ch <= 0)
		return ch == 0 ? -44 : ch;
	b = sc_buf[sc_depth];
	for (;;) {
		n = SC_MAX + 1 - total;
		if (n > 256)
			n = 256;                             /* the channel read limit per call */
		n = (int) qtrap(APP_QT_CH_READ, (unsigned int) ch, (unsigned int) (b + total), (unsigned int) n);
		if (n <= 0)
			break;
		total += n;
		if (total > SC_MAX)
			break;
	}
	qtrap(APP_QT_CH_CLOSE, (unsigned int) ch, 0, 0);
	if (n < 0)
		return n;
	if (total > SC_MAX) {
		outs("script too large (limit 4096 bytes): not run\n");
		sh_fatal = 1;
		return -1;
	}
	i = sc_prepare(b, (unsigned int) total);
	if (i != (unsigned int) total) {
		outs("script holds a control character (byte ");
		outnum((long) i);
		outs("): not run\n");
		sh_fatal = 1;
		return -1;
	}
	sc_depth++;
	pos = 0;
	for (;;) {
		r = sc_line(b, (unsigned int) total, &pos, lb, SH_LINE_MAX, &lineno);
		if (r == 0)
			break;
		if (r < 0) {
			outs("line ");
			outnum(lineno);
			outs(" too long (limit 159 characters)\n");
			sh_fatal = 1;
			rc = -1;
			break;
		}
		if (sc_skip(lb))
			continue;
		if (sc_depth == 1)
			sc_lines++;
		exec_line(lb);
		if (sc_exit) {
			sc_exit = 0;
			break;
		}
		if (sh_fatal) {
			outs("script ");
			outs(path);
			outs(" stopped at line ");
			outnum(lineno);
			outc('\n');
			rc = -1;
			break;
		}
	}
	sc_depth--;
	return rc;
}

static void cmd_call(char *arg)
{
	char cpath[SH_PATH_ARG + 1];                   /* on the stack: nested CALLs each keep their own name */
	unsigned int i;
	int rc;

	arg = uq(arg);
	if (*arg == 0) {
		outs("usage: CALL script\n");
		return;
	}
	for (i = 0; arg[i] && i < SH_PATH_ARG; i++)
		cpath[i] = arg[i];
	cpath[i] = 0;
	if (arg[i] != 0) {
		say_fs_error(-90);
		return;
	}
	rc = run_script(cpath);
	if (rc == -1)
		sh_fatal = 1;                          /* the caller's script stops too */
	else if (rc < 0)
		say_fs_error(rc);
}

/* At boot: run /ETC/STARTUP.CMD once, unless ESC is pressed within about two seconds (safe start). */
static void startup(void)
{
	unsigned long t0;
	int rc, skip = 0;

	rc = (int) qtrap(APP_QT_FS_OP, FS_STAT, (unsigned int) "WIN1:/ETC/STARTUP.CMD", (unsigned int) statbuf);
	if (rc != 0) {
		outs("STARTUP: none (no /ETC/STARTUP.CMD)\n");
		return;
	}
	outs("STARTUP: /ETC/STARTUP.CMD - press ESC within 2 seconds to skip it\n");
	t0 = (unsigned long) qtrap(APP_QT_TICKS, 0, 0, 0);
	while ((unsigned long) qtrap(APP_QT_TICKS, 0, 0, 0) - t0 < 200u) {
		if ((qtrap(APP_QT_CH_CTL, (unsigned int) con_ch, 0x13u, 5u) & 0xffff) > 0) {
			if (qtrap(APP_QT_CH_READ, (unsigned int) con_ch, (unsigned int) rbuf, 1) == 1 && rbuf[0] == 27) {
				skip = 1;
				break;
			}
			continue;
		}
		qtrap(APP_QT_SUSPEND, 5, 0, 0);
	}
	if (skip) {
		outs("STARTUP: skipped (ESC) - safe start, environment empty\n");
		return;
	}
	sc_lines = 0;
	rc = run_script("/ETC/STARTUP.CMD");
	sh_fatal = 0;
	if (rc == 0) {
		outs("STARTUP: done (");
		outnum(sc_lines);
		outs(" commands)\n");
	} else if (rc == -1) {
		outs("STARTUP: stopped by an error - the shell is ready; fix /ETC/STARTUP.CMD or press ESC at the next boot\n");
	} else {
		outs("STARTUP: could not read /ETC/STARTUP.CMD: ");
		say_fs_error(rc);
	}
}
