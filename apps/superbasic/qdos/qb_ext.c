/*
 * qb_ext.c - QDOS/386 R12B: the SuperBASIC procedures and functions that a QDOS host adds to the name table
 * (the hosted counterpart of BP.INIT; SbExtName in sb_state.h):
 *
 *   LOAD name          replace the program with the numbered lines of a file (direct mode only)
 *   MERGE name         add the lines of a file; a line number already present is replaced (direct mode only)
 *   SAVE name          write the program as LIST shows it ("<n> <text>" LF per line)
 *   DIR [name]         list a directory (default: the interpreter's current directory)
 *   EXEC name [, args$ [, con]]     start a program and go on (QDOS/386 extension: args$, con)
 *   EXEC_W name [, args$ [, con]]   start a program and wait for it; with con, show that console meanwhile
 *   EXEC_ID            function: the job id of the last EXEC / EXEC_W (0 before any)
 *   EXEC_STATUS        function: the exit status of the last EXEC_W or JOB_WAIT that ended a job
 *   JOB_WAIT(id [, ticks])  function: wait for a child; its status (>= 0), -1 still running after ticks, -2 not a child
 *   JOB_KILL id        end a child of this interpreter
 *   JOBS               list the children this interpreter started and their state
 *
 * name: a string expression, or a bare name that is not a variable (LOAD prog_bas is the file "prog_bas").
 * Paths are relative to the interpreter's current directory; programs are found like the shell's RUN (cwd, PATH,
 * root; .AOU may be left out).  docs/R12B-SUPERBASIC-FILE-ABI.md and docs/R12B-SUPERBASIC-EXEC-ABI.md.
 *
 * Strict C89.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "qdos386.h"
#include "sb_error.h"
#include "sb_expr.h"
#include "sb_interp.h"
#include "sb_lex.h"
#include "sb_state.h"
#include "sb_store.h"
#include "qb.h"
#include "qb_ext.h"

int qd_con_channel(void);            /* libc stdio.c: the CON channel of stdout */

#define QBX_NAME_MAX   64                  /* QD_PATH_MAX */
#define QBX_FILE_MAX   65536L              /* LOAD / MERGE file limit */
#define QBX_KIDS       8                   /* children remembered for JOBS (the kernel allows 8 per parent) */

/* QL error codes used here (sb_error.c has their texts). */
#define QL_NC   (-1)                       /* not complete */
#define QL_NJ   (-2)                       /* invalid Job */
#define QL_OM   (-3)                       /* out of memory */
#define QL_BF   (-5)                       /* buffer full */
#define QL_NF   (-7)                       /* not found */
#define QL_IU   (-9)                       /* in use */
#define QL_DF   (-11)                      /* drive full */
#define QL_BN   (-12)                      /* bad name */
#define QL_BP   (-15)                      /* bad parameter */
#define QL_BM   (-16)                      /* bad or changed medium */
#define QL_NI   (-19)                      /* not implemented */
#define QL_RO   (-20)                      /* read only */
#define QL_BL   (-21)                      /* bad line */

static long last_id;
static long last_status;
static long kids[QBX_KIDS];

/* ---- errors --------------------------------------------------------------------------------------------------- */

/* A QDOS/386 file or process error as the nearest QL code. */
static int ql_err(int rc)
{
	switch (rc) {
	case -44: return QL_NF;                         /* no such file */
	case -87: case -88: case -46: case -47: case -90: case -91: case -12: return QL_BN;
	case -81: case -82: return QL_DF;               /* disk full, directory full */
	case -80: return QL_RO;
	case -83: return -8;                            /* already exists */
	case -84: return QL_IU;
	case -85: case -40: case -43: case -45: case -18: return QL_BM;   /* write/read error, bad chain, no volume */
	case -60: case -30: case -3: return QL_OM;      /* no frames, no heap */
	case -29: case -69: case -2: case -68: return QL_NJ;   /* job table, child limit, not a child */
	case -20: case -21: case -22: case -23: case -24: case -25: case -26: case -31: case -32: case -61: return QL_BP;
	                                                /* not a valid program image */
	case -34: case -14: return QL_BP;
	default: return rc < -21 ? QL_BM : rc;
	}
}

/* ---- argument parsing ------------------------------------------------------------------------------------------ */

static int at_sep(const SbLexer *lx)
{
	return lx->type == SB_TK_EOL || (lx->type == SB_TK_OP && lx->op == SB_OP_COLON);
}

static int is_comma(const SbLexer *lx)
{
	return lx->type == SB_TK_OP && lx->op == SB_OP_COMMA;
}

/* A file or program name: a string expression, or a bare identifier that is not a variable. */
static int get_name(SbState *st, SbLexer *lx, char *out)
{
	SbValue v;
	int rc;

	if (lx->type == SB_TK_IDENT && sb_var_find(st, lx->text, lx->tlen) == NULL) {
		SbLexer peek = *lx;

		sb_lex_next(&peek);
		if (at_sep(&peek) || is_comma(&peek)) {
			if (lx->tlen == 0 || lx->tlen > QBX_NAME_MAX)
				return QL_BN;
			memcpy(out, lx->text, lx->tlen);
			out[lx->tlen] = 0;
			sb_lex_next(lx);
			return SB_OK;
		}
	}
	rc = sb_expr_eval(st, lx, &v);
	if (rc != SB_OK)
		return rc;
	if (v.type != SB_VAL_STR) {
		sb_val_free(&st->mem, &v);
		return QL_BP;
	}
	if (v.len == 0 || v.len > QBX_NAME_MAX || memchr(v.str, 0, v.len) != NULL) {
		sb_val_free(&st->mem, &v);
		return QL_BN;
	}
	memcpy(out, v.str, v.len);
	out[v.len] = 0;
	sb_val_free(&st->mem, &v);
	return SB_OK;
}

static int get_string(SbState *st, SbLexer *lx, char *out, size_t cap)
{
	SbValue v, s;
	int rc;

	rc = sb_expr_eval(st, lx, &v);
	if (rc != SB_OK)
		return rc;
	rc = sb_val_to_string(&st->mem, &v, &s);
	sb_val_free(&st->mem, &v);
	if (rc != SB_OK)
		return rc;
	if (s.len >= cap) {
		sb_val_free(&st->mem, &s);
		return QL_BF;
	}
	memcpy(out, s.str, s.len);
	out[s.len] = 0;
	sb_val_free(&st->mem, &s);
	return SB_OK;
}

static int get_long(SbState *st, SbLexer *lx, long *out)
{
	SbValue v;
	double d;
	int rc;

	rc = sb_expr_eval(st, lx, &v);
	if (rc != SB_OK)
		return rc;
	rc = sb_val_to_number(&v, &d);
	sb_val_free(&st->mem, &v);
	if (rc != SB_OK)
		return rc;
	if (d < -2147483647.0 || d > 2147483647.0)
		return -4;
	*out = (long) (d < 0 ? d - 0.5 : d + 0.5);
	return SB_OK;
}

/* "WIN1:" + name unless it already names the volume. */
static void native(const char *name, char *out)
{
	if ((name[0] == 'W' || name[0] == 'w') && (name[1] == 'I' || name[1] == 'i') && (name[2] == 'N' || name[2] == 'n') &&
	    name[3] == '1' && (name[4] == ':' || name[4] == '_'))
		strcpy(out, name);
	else
		sprintf(out, "WIN1:%s", name);
}

/* ---- LOAD and MERGE --------------------------------------------------------------------------------------------- */

/* Read the file into a new block (*len bytes).  Every line must be numbered; checked by stage(). */
static int read_file(const char *name, char **buf, long *len)
{
	char path[QBX_NAME_MAX + 8];
	int ch, n;
	long size, got = 0;
	char *b;

	native(name, path);
	ch = qd_open_ex(path, QD_O_READ | QD_O_FILE);
	if (ch < 0)
		return ql_err(ch);
	size = qd_fsize(ch);
	if (size < 0) {
		qd_close(ch);
		return ql_err((int) size);
	}
	if (size > QBX_FILE_MAX) {
		qd_close(ch);
		return QL_BF;
	}
	b = (char *) malloc((size_t) size + 1);
	if (b == NULL) {
		qd_close(ch);
		return QL_OM;
	}
	while (got < size) {
		n = qd_read(ch, b + got, (unsigned int) (size - got > 256 ? 256 : size - got));   /* 256: the read limit per call */
		if (n < 0) {
			free(b);
			qd_close(ch);
			return ql_err(n);
		}
		if (n == 0)
			break;
		got += n;
	}
	qd_close(ch);
	if (got != size) {
		free(b);
		return QL_BM;                                   /* shorter than its directory entry says */
	}
	*buf = b;
	*len = size;
	return SB_OK;
}

/* Store every line of b into the fresh state tmp, exactly as typed lines are stored (sb_prog_store: canonical form;
 * a number alone deletes).  Blank lines are ignored.  Any line without a number, with a number over 32767, longer
 * than QB_LINE_MAX, or with a control byte (other than TAB) stops the staging: nothing is changed. */
static int stage(SbState *tmp, const char *b, long len, long *bad_line)
{
	long s = 0, e, k, lineno = 0, num;
	long i;
	int rc;

	while (s < len) {
		e = s;
		while (e < len && b[e] != '\n')
			e++;
		k = e;
		if (k > s && b[k - 1] == '\r')
			k--;
		lineno++;
		*bad_line = lineno;
		for (i = s; i < k; i++)
			if ((unsigned char) b[i] < 32 && b[i] != '\t')
				return QL_BL;
		if (k - s > QB_LINE_MAX)
			return QL_BF;
		i = s;
		while (i < k && (b[i] == ' ' || b[i] == '\t'))
			i++;
		if (i < k) {
			if (b[i] < '0' || b[i] > '9')
				return QL_BL;               /* QDOS/386: only numbered lines (see the ABI document) */
			num = 0;
			while (i < k && b[i] >= '0' && b[i] <= '9') {
				num = num * 10 + (b[i] - '0');
				if (num > 32767L)
					return QL_BL;
				i++;
			}
			while (i < k && (b[i] == ' ' || b[i] == '\t'))
				i++;
			rc = sb_prog_store(tmp, num, b + i, (size_t) (k - i));
			if (rc != SB_OK)
				return rc;
		}
		s = e + 1;
	}
	*bad_line = 0;
	return SB_OK;
}

/* Move every line of src into st's program; a line number already there is replaced (as typing it would). */
static void merge_into(SbState *st, SbState *src)
{
	SbLine *n, *next, *l, *prev;

	for (n = src->prog; n != NULL; n = next) {
		next = n->next;
		prev = NULL;
		for (l = st->prog; l != NULL && l->number < n->number; l = l->next)
			prev = l;
		if (l != NULL && l->number == n->number) {
			sb_def_drop(st, n->number);
			sb_mem_free(&st->mem, l->text);
			l->text = n->text;
			l->len = n->len;
			sb_mem_free(&st->mem, n);
		} else {
			n->next = l;
			if (prev == NULL)
				st->prog = n;
			else
				prev->next = n;
		}
	}
	src->prog = NULL;
}

static long loadmsg_line;

static int load_or_merge(SbState *st, SbLexer *lx, int merge)
{
	char name[QBX_NAME_MAX + 1];
	char *buf = NULL;
	long len = 0;
	SbState tmp;
	int rc;

	rc = get_name(st, lx, name);
	if (rc != SB_OK)
		return rc;
	if (!at_sep(lx))
		return SB_E_EXPRESSION;
	if (!sb_in_direct_mode(st))
		return QL_NI;                          /* not from a running program (documented) */
	rc = read_file(name, &buf, &len);
	if (rc != SB_OK)
		return rc;
	sb_init(&tmp, &st->mem);
	rc = stage(&tmp, buf, len, &loadmsg_line);
	free(buf);
	if (rc != SB_OK) {
		char t[80];

		sb_destroy(&tmp);
		sprintf(t, "%s: line %ld of the file refused; the program is unchanged\n", merge ? "MERGE" : "LOAD", loadmsg_line);
		qb_puts(t);
		return rc;
	}
	if (merge) {
		merge_into(st, &tmp);
	} else {
		sb_new(st);
		st->prog = tmp.prog;
		tmp.prog = NULL;
	}
	sb_destroy(&tmp);
	return SB_OK;
}

static int x_load(SbState *st, SbLexer *lx)
{
	return load_or_merge(st, lx, 0);
}

static int x_merge(SbState *st, SbLexer *lx)
{
	return load_or_merge(st, lx, 1);
}

/* ---- SAVE ------------------------------------------------------------------------------------------------------- */

/* The program as LIST shows it, in pieces: one call per line into a 300-byte buffer. */
static size_t line_text(const SbLine *l, char *out)
{
	size_t n = (size_t) sprintf(out, "%ld ", l->number);
	size_t t = l->len > 280 ? 280 : l->len;

	memcpy(out + n, l->text, t);
	n += t;
	out[n++] = '\n';
	return n;
}

/* Write n bytes; 0, or the kernel's error (qd_write_all reports a short count instead of the error, e.g. disk full). */
static int write_rc(int ch, const char *p, size_t n)
{
	size_t done = 0;
	int r;

	while (done < n) {
		r = qd_write(ch, p + done, (unsigned int) (n - done > QD_WRITE_MAX ? QD_WRITE_MAX : n - done));
		if (r < 0)
			return r;
		if (r == 0)
			return -85;
		done += (size_t) r;
	}
	return 0;
}

static int write_prog(int ch, const SbState *st, long *bytes)
{
	char lb[300];
	const SbLine *l;
	size_t n;
	int rc;

	*bytes = 0;
	for (l = st->prog; l != NULL; l = l->next) {
		n = line_text(l, lb);
		rc = write_rc(ch, lb, n);
		if (rc < 0)
			return rc;
		*bytes += (long) n;
	}
	return 0;
}

/* Read the file back and compare it with the program text.  0 if equal. */
static int verify(const char *path, const SbState *st, long bytes)
{
	char lb[300], rb[300];
	const SbLine *l;
	size_t n, got;
	int ch, r;

	ch = qd_open_ex(path, QD_O_READ | QD_O_FILE);
	if (ch < 0)
		return ch;
	if (qd_fsize(ch) != bytes) {
		qd_close(ch);
		return -85;
	}
	for (l = st->prog; l != NULL; l = l->next) {
		n = line_text(l, lb);
		for (got = 0; got < n; got += (size_t) r) {
			r = qd_read(ch, rb + got, (unsigned int) (n - got > 256 ? 256 : n - got));
			if (r <= 0) {
				qd_close(ch);
				return r < 0 ? r : -85;
			}
		}
		if (memcmp(lb, rb, n) != 0) {
			qd_close(ch);
			return -85;
		}
	}
	r = qd_read(ch, rb, 1);
	qd_close(ch);
	return r == 0 ? 0 : -85;
}

static void sibling(const char *target, const char *leaf, char *out)
{
	const char *s = strrchr(target, '/');

	if (s == NULL)
		s = strrchr(target, ':');
	if (s == NULL) {
		strcpy(out, leaf);
		return;
	}
	memcpy(out, target, (size_t) (s - target) + 1);
	strcpy(out + (s - target) + 1, leaf);
}

static int exists(const char *p)
{
	struct qd_stat sbuf;

	return qd_stat(p, &sbuf) == 0;
}

/* Write a temporary sibling, read it back, then rename the old file aside, rename the new one into place and remove
 * the old one.  On any failure before the final rename the old file is untouched; the temporary file is removed.
 * NOT atomic: a reset between the two renames leaves the old file as ~SBxxxx.BAK (documented). */
static int x_save(SbState *st, SbLexer *lx)
{
	char name[QBX_NAME_MAX + 1], path[QBX_NAME_MAX + 8], tmp[QBX_NAME_MAX + 24], bak[QBX_NAME_MAX + 24], leaf[16];
	struct qd_stat sbuf;
	long bytes;
	int ch, rc, had;

	rc = get_name(st, lx, name);
	if (rc != SB_OK)
		return rc;
	if (!at_sep(lx))
		return SB_E_EXPRESSION;
	native(name, path);
	if (qd_stat(path, &sbuf) == 0 && sbuf.is_dir)
		return QL_BN;
	sprintf(leaf, "~SB%04X.TMP", (unsigned int) qd_self() & 0xFFFFu);
	sibling(path, leaf, tmp);
	sprintf(leaf, "~SB%04X.BAK", (unsigned int) qd_self() & 0xFFFFu);
	sibling(path, leaf, bak);
	if (exists(tmp))
		qd_remove(tmp);
	ch = qd_open_ex(tmp, QD_O_WRITE | QD_O_CREATE | QD_O_EXCL | QD_O_FILE);
	if (ch < 0)
		return ql_err(ch);
	rc = write_prog(ch, st, &bytes);
	if (rc == 0)
		rc = qd_flush(ch);
	if (rc < 0) {
		qd_close(ch);
		qd_remove(tmp);
		return ql_err(rc);
	}
	rc = qd_close(ch);
	if (rc < 0) {
		qd_remove(tmp);
		return ql_err(rc);
	}
	rc = verify(tmp, st, bytes);
	if (rc < 0) {
		qd_remove(tmp);
		return ql_err(rc);
	}
	had = exists(path);
	if (had) {
		if (exists(bak))
			qd_remove(bak);
		rc = qd_rename(path, bak);
		if (rc < 0) {
			qd_remove(tmp);
			return ql_err(rc);
		}
	}
	rc = qd_rename(tmp, path);
	if (rc < 0) {
		if (had)
			qd_rename(bak, path);
		qd_remove(tmp);
		return ql_err(rc);
	}
	if (had)
		qd_remove(bak);
	return SB_OK;
}

/* ---- DIR -------------------------------------------------------------------------------------------------------- */

static int x_dir(SbState *st, SbLexer *lx)
{
	char name[QBX_NAME_MAX + 1], path[QBX_NAME_MAX + 8], t[100];
	struct qd_dirent_long e;
	int ch, r, files = 0, dirs = 0;

	name[0] = 0;
	if (!at_sep(lx)) {
		r = get_name(st, lx, name);
		if (r != SB_OK)
			return r;
		if (!at_sep(lx))
			return SB_E_EXPRESSION;
	}
	if (name[0] == 0)
		strcpy(path, "WIN1:.");
	else
		native(name, path);
	ch = qd_dir_open_long(path);
	if (ch < 0)
		return ql_err(ch);
	for (;;) {
		r = qd_dir_next_long(ch, &e);
		if (r <= 0)
			break;
		if (e.size == 0xFFFFFFFFul) {
			sprintf(t, "%-40s <DIR>\n", e.name);
			dirs++;
		} else {
			sprintf(t, "%-40s %8lu\n", e.name, e.size);
			files++;
		}
		qb_puts(t);
	}
	qd_close(ch);
	if (r < 0)
		return ql_err(r);
	sprintf(t, "%d file(s), %d dir(s)\n", files, dirs);
	qb_puts(t);
	return SB_OK;
}

/* ---- EXEC, EXEC_W and the job functions ----------------------------------------------------------------------- */

/* Split args into the packed form (NUL after each argument); spaces separate, "..." or '...' keeps spaces. */
static int pack_args(const char *a, char *out, unsigned int cap, unsigned int *len)
{
	unsigned int n = 0;

	while (*a) {
		while (*a == ' ')
			a++;
		if (*a == 0)
			break;
		if (*a == '"' || *a == '\'') {         /* "..." or '...' keeps spaces ('...' because a BASIC string cannot hold ") */
			char q = *a++;

			while (*a && *a != q) {
				if (n + 2 > cap)
					return QL_BF;
				out[n++] = *a++;
			}
			if (*a != q)
				return QL_BP;                   /* unterminated quote */
			a++;
		} else {
			while (*a && *a != ' ') {
				if (n + 2 > cap)
					return QL_BF;
				out[n++] = *a++;
			}
		}
		out[n++] = 0;
	}
	*len = n;
	return SB_OK;
}

static void kid_add(long id)
{
	int i;

	for (i = 0; i < QBX_KIDS; i++)
		if (kids[i] == 0 || qd_poll((int) kids[i]) == QD_PROC_ERR_NOTCHILD) {
			kids[i] = id;
			return;
		}
}

static void kid_forget(long id)
{
	int i;

	for (i = 0; i < QBX_KIDS; i++)
		if (kids[i] == id)
			kids[i] = 0;
}

static int exec_common(SbState *st, SbLexer *lx, int wait)
{
	char name[QBX_NAME_MAX + 1], args[300], packed[420];
	unsigned int plen = 0;
	long con = -1;
	int rc, id, cch = -1, mycon = -1, focus = 0;

	rc = get_name(st, lx, name);
	if (rc != SB_OK)
		return rc;
	args[0] = 0;
	if (is_comma(lx)) {
		sb_lex_next(lx);
		rc = get_string(st, lx, args, sizeof args);
		if (rc != SB_OK)
			return rc;
		if (is_comma(lx)) {
			sb_lex_next(lx);
			rc = get_long(st, lx, &con);
			if (rc != SB_OK)
				return rc;
			if (con < 0 || con >= QD_CON_MAX)
				return QL_BP;
		}
	}
	if (!at_sep(lx))
		return SB_E_EXPRESSION;
	rc = pack_args(args, packed, sizeof packed, &plen);
	if (rc != SB_OK)
		return rc;
	cch = qd_con_channel();
	if (cch > 0)
		mycon = qd_con_number(cch);
	id = qd_spawn(name, packed, plen, 0, con >= 0 ? (int) con : QD_SPAWN_CON_OWN);
	if (id < 0)
		return ql_err(id);
	last_id = id;
	if (!wait) {
		kid_add(id);
		return SB_OK;
	}
	/* EXEC_W on another console: show it while the program runs, then come back (only if this console was shown) */
	if (con >= 0 && con != mycon && cch > 0 && qd_con_is_foreground(cch) == 1) {
		qd_con_focus(cch, (int) con);
		focus = 1;
	}
	rc = qd_wait(id, -1L);
	if (focus)
		qd_con_focus(cch, mycon);
	if (rc < 0 && rc != QD_ERR_KILLED && rc != QD_ERR_NOSTATUS)
		return ql_err(rc);
	last_status = rc;                          /* -66 killed, -17 ended without a status (a fault) */
	return SB_OK;
}

static int x_exec(SbState *st, SbLexer *lx)
{
	return exec_common(st, lx, 0);
}

static int x_exec_w(SbState *st, SbLexer *lx)
{
	return exec_common(st, lx, 1);
}

static int x_job_kill(SbState *st, SbLexer *lx)
{
	long id;
	int rc;

	rc = get_long(st, lx, &id);
	if (rc != SB_OK)
		return rc;
	if (!at_sep(lx))
		return SB_E_EXPRESSION;
	rc = qd_kill((int) id);
	if (rc < 0)
		return QL_NJ;                       /* not a child of this interpreter, or already gone */
	return SB_OK;
}

static int x_jobs(SbState *st, SbLexer *lx)
{
	char t[80];
	int i, r, any = 0;

	(void) st;
	if (!at_sep(lx))
		return SB_E_EXPRESSION;
	for (i = 0; i < QBX_KIDS; i++) {
		if (kids[i] == 0)
			continue;
		r = qd_poll((int) kids[i]);
		if (r == QD_PROC_ERR_NOTCHILD) {
			kids[i] = 0;
			continue;
		}
		if (r == QD_PROC_ERR_TIMEOUT)
			sprintf(t, "job %ld running\n", kids[i]);
		else
			sprintf(t, "job %ld ended, status %d (JOB_WAIT collects it)\n", kids[i], r);
		qb_puts(t);
		any = 1;
	}
	if (!any)
		qb_puts("no jobs\n");
	return SB_OK;
}

/* Functions: lx is at the token after the name. */
static int no_args(SbLexer *lx)
{
	if (lx->type == SB_TK_OP && lx->op == SB_OP_LPAR) {
		sb_lex_next(lx);
		if (!(lx->type == SB_TK_OP && lx->op == SB_OP_RPAR))
			return SB_E_EXPRESSION;
		sb_lex_next(lx);
	}
	return SB_OK;
}

static int f_exec_id(SbState *st, SbLexer *lx, SbValue *out)
{
	int rc = no_args(lx);

	(void) st;
	if (rc != SB_OK)
		return rc;
	sb_val_init_num(out, (double) last_id);
	return SB_OK;
}

static int f_exec_status(SbState *st, SbLexer *lx, SbValue *out)
{
	int rc = no_args(lx);

	(void) st;
	if (rc != SB_OK)
		return rc;
	sb_val_init_num(out, (double) last_status);
	return SB_OK;
}

static int f_job_wait(SbState *st, SbLexer *lx, SbValue *out)
{
	long id, ticks = -1;
	int rc;

	if (!(lx->type == SB_TK_OP && lx->op == SB_OP_LPAR))
		return SB_E_EXPRESSION;
	sb_lex_next(lx);
	rc = get_long(st, lx, &id);
	if (rc != SB_OK)
		return rc;
	if (is_comma(lx)) {
		sb_lex_next(lx);
		rc = get_long(st, lx, &ticks);
		if (rc != SB_OK)
			return rc;
		if (ticks < -1)
			return QL_BP;
	}
	if (!(lx->type == SB_TK_OP && lx->op == SB_OP_RPAR))
		return SB_E_EXPRESSION;
	sb_lex_next(lx);
	rc = qd_wait((int) id, ticks);
	if (rc == QD_PROC_ERR_TIMEOUT) {
		rc = QL_NC;                                     /* -1: still running */
	} else if (rc == QD_PROC_ERR_NOTCHILD || rc == QD_ERR_NOJOB) {
		rc = QL_NJ;                                     /* -2: not a child (or already collected) */
	} else if (rc >= 0 || rc == QD_ERR_KILLED || rc == QD_ERR_NOSTATUS) {
		last_status = rc;
		kid_forget(id);
	} else {
		rc = ql_err(rc);
	}
	sb_val_init_num(out, (double) rc);
	return SB_OK;
}

static const SbExtName qb_names[] = {
	{ "LOAD", x_load, NULL },
	{ "MERGE", x_merge, NULL },
	{ "SAVE", x_save, NULL },
	{ "DIR", x_dir, NULL },
	{ "EXEC", x_exec, NULL },
	{ "EXEC_W", x_exec_w, NULL },
	{ "JOB_KILL", x_job_kill, NULL },
	{ "JOBS", x_jobs, NULL },
	{ "EXEC_ID", NULL, f_exec_id },
	{ "EXEC_STATUS", NULL, f_exec_status },
	{ "JOB_WAIT", NULL, f_job_wait },
	{ NULL, NULL, NULL }
};

void qb_ext_install(SbState *st)
{
	st->ext = qb_names;
	st->ext_ctx = NULL;
}
