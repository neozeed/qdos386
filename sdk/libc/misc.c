/* misc.c - QDOS/386 libc (R11D): assert, locale, signal, and the stdio functions
 * that the FAT16 volume and the QDOS trap ABI cannot support (they fail
 * explicitly).  See docs/R11D-BLOCKED-APIS.md. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <locale.h>
#include <signal.h>
#include "qdos386.h"

/* ---- assert --------------------------------------------------------------- */

void __qd_assert(const char *expr, const char *file, int line)
{
	fprintf(stderr, "%s:%d: Assertion `%s' failed.\n", file, line, expr);
	abort();
}

/* ---- locale: the C locale only ------------------------------------------- */

static char qd_c_name[] = "C";
static char qd_empty[] = "";
static char qd_dot[] = ".";
static char qd_no_group[] = "";

static struct lconv qd_lconv = {
	qd_dot, qd_empty, qd_no_group, qd_empty, qd_empty, qd_empty, qd_empty,
	qd_no_group, qd_empty, qd_empty,
	(char) 127, (char) 127, (char) 127, (char) 127, (char) 127, (char) 127,
	(char) 127, (char) 127
};

char *setlocale(int category, const char *locale)
{
	(void) category;
	if (locale == NULL || locale[0] == '\0' || strcmp(locale, "C") == 0)
		return qd_c_name;
	return NULL;                 /* no other locale exists */
}

struct lconv *localeconv(void)
{
	return &qd_lconv;
}

/* ---- signal: no delivery exists ------------------------------------------ */

void (*signal(int sig, void (*handler)(int)))(int)
{
	(void) sig;
	(void) handler;
	errno = 0;                   /* no errno value is defined for this */
	return SIG_ERR;
}

int raise(int sig)
{
	(void) sig;
	return -1;                   /* nothing can be delivered */
}

/* ---- error reporting ----------------------------------------------------- */

void perror(const char *s)
{
	if (s != NULL && s[0] != '\0')
		fprintf(stderr, "%s: %s\n", s, strerror(errno));
	else
		fprintf(stderr, "%s\n", strerror(errno));
}

/* ---- files: names, remove, rename, tmpnam (R11L) -------------------------------------------------- */

/* Cached kind of the current directory: 0 not asked yet, 1 the root or a kernel without directories
 * (bare names keep the legacy "WIN1_NAME" form), 2 a subdirectory (bare names are "WIN1:NAME", which the
 * kernel reads relative to the current directory).  qd_chdir invalidates it (qd_fsname_reset). */
static int cwd_kind = 0;

void qd_fsname_reset(void)
{
	cwd_kind = 0;
}

static int cwd_is_subdir(void)
{
	if (cwd_kind == 0) {
		char b[8];
		int r = qd_getcwd(b, sizeof b);
		/* 1 = "/"; > 1 or QD_ERR_DEPTH (path longer than the probe buffer) = a subdirectory; any other
		 * negative = a kernel without directories */
		cwd_kind = (r == 1 || (r < 0 && r != QD_ERR_DEPTH)) ? 1 : 2;
	}
	return cwd_kind == 2;
}

/* Is s a legal legacy flat name once upper-cased: base 1..8 and extension 0..3 characters from A-Z 0-9 _ , at most one dot?
 * A bare name that is not (spaces, '-', '~', a long name, ...) cannot be the legacy "WIN1_NAME" form; it becomes a native
 * name, which a kernel with VFAT long names (R11M Stage 4) stores and finds, and any other kernel refuses as before. */
static int legacy_name_ok(const char *s)
{
	unsigned int base = 0, ext = 0, dot = 0;

	for (; *s != '\0'; s++) {
		char c = *s;
		if (c >= 'a' && c <= 'z')
			c = (char) (c - 'a' + 'A');
		if (c == '.') {
			if (dot)
				return 0;
			dot = 1;
		} else if ((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_') {
			if (dot) {
				if (++ext > 3)
					return 0;
			} else if (++base > 8) {
				return 0;
			}
		} else {
			return 0;
		}
	}
	return base > 0;
}

/* Fold a program-visible file name into the kernel's channel name.
 *   "CON"                     stays.
 *   "WIN1_NAME"               the legacy flat root name: folded to upper case, no separators allowed.
 *   "WIN1_A/B" "WIN1:A/B"     passed through (kernel: root path / native path).
 *   "A/B", "./A", "/A"        a path: becomes "WIN1:" + path (the kernel folds case).
 *   "NAME"                    in the root: "WIN1_NAME" (as before R11M) if it is a legal 8.3 name; otherwise, and in a
 *                             subdirectory: "WIN1:NAME".
 * A path part is at most QD_PATH_MAX characters.  Returns 0, or -1 if the name is empty, uses '\\' or
 * a stray ':' , or is too long. */
int qd_fsname(const char *in, char *out, size_t outsz)
{
	size_t n = 0, i, pl;
	const char *base;
	int slash = 0, native = 0;

	if (in == NULL || in[0] == '\0')
		return -1;
	if (strcmp(in, "CON") == 0) {
		if (outsz < 4)
			return -1;
		strcpy(out, "CON");
		return 0;
	}
	while (in[0] == '.' && in[1] == '/' && in[2] != '\0')
		in += 2;                     /* a leading "./" is dropped (not for "./" alone) */
	base = in;
	if (strncmp(in, "WIN1_", 5) == 0 || strncmp(in, "win1_", 5) == 0) {
		base = in + 5;
		native = 0;
	} else if (strncmp(in, "WIN1:", 5) == 0 || strncmp(in, "win1:", 5) == 0) {
		base = in + 5;
		native = 1;
	} else {
		native = -1;                 /* undecided: a bare name or a path */
	}
	for (i = 0; base[i] != '\0'; i++) {
		if (base[i] == '\\')
			return -1;
		if (base[i] == ':')
			return -1;
		if (base[i] == '/')
			slash = 1;
	}
	pl = i;
	if (pl > QD_PATH_MAX)
		return -1;
	if (native < 0)
		native = (slash || cwd_is_subdir() || !legacy_name_ok(base)) ? 1 : 0;
	if (outsz < 5 + pl + 1)
		return -1;
	memcpy(out, native ? "WIN1:" : "WIN1_", 5);
	n = 5;
	for (i = 0; base[i] != '\0'; i++) {
		char c = base[i];
		if (!native && !slash && c >= 'a' && c <= 'z')
			c = (char) (c - 'a' + 'A');     /* the legacy flat form is uppercase 8.3 only */
		out[n++] = c;
	}
	out[n] = '\0';
	return 0;
}

/* 1 if a mapped name is just a volume prefix (no file part). */
int qd_fs_isroot(const char *full)
{
	return strcmp(full, "WIN1_") == 0 || strcmp(full, "WIN1:") == 0;
}

int remove(const char *name)
{
	char full[FILENAME_MAX];
	int r;

	if (qd_fsname(name, full, sizeof full) != 0 || qd_fs_isroot(full)) {
		errno = ENOENT;
		return -1;
	}
	r = qd_remove(full);
	if (r < 0) {
		qd_set_errno(r);
		return -1;
	}
	return 0;
}

int rename(const char *from, const char *to)
{
	char a[FILENAME_MAX], b[FILENAME_MAX];
	int r;

	if (qd_fsname(from, a, sizeof a) != 0 || qd_fsname(to, b, sizeof b) != 0) {
		errno = ENOENT;
		return -1;
	}
	r = qd_rename(a, b);
	if (r < 0) {
		qd_set_errno(r);
		return -1;
	}
	return 0;
}

FILE *tmpfile(void)
{
	return NULL;                 /* not implemented: no delete-on-close (documented) */
}

/* tmpnam: "TMPnnnnn.TMP" with a counter seeded from the tick count; the name is checked against
 * the directory by trying to open it for reading.  L_tmpnam is not used (the buffer is static
 * when s is NULL). */
char *tmpnam(char *s)
{
	static char buf[16];
	static unsigned long ctr = 0;
	char *d = s != NULL ? s : buf;
	int tries;

	for (tries = 0; tries < 1000; tries++) {
		unsigned long v = (qd_ticks() + ctr++) % 100000ul;
		int i;
		FILE *fp;

		d[0] = 'T'; d[1] = 'M'; d[2] = 'P';
		for (i = 7; i >= 3; i--) {
			d[i] = (char) ('0' + (int) (v % 10));
			v /= 10;
		}
		d[8] = '.'; d[9] = 'T'; d[10] = 'M'; d[11] = 'P'; d[12] = '\0';
		fp = fopen(d, "r");
		if (fp == NULL)
			return d;
		fclose(fp);
	}
	return NULL;
}

/* ---- stdio positioning and gets ------------------------------------------ */

int fgetpos(FILE *fp, fpos_t *pos)
{
	long p = ftell(fp);
	if (p < 0)
		return -1;
	*pos = p;
	return 0;
}

int fsetpos(FILE *fp, const fpos_t *pos)
{
	return fseek(fp, *pos, SEEK_SET);
}

/* gets has no length limit, as in C89; it is unsafe, and it is provided for
 * completeness only. */
char *gets(char *buf)
{
	int c;
	char *p = buf;
	while ((c = getchar()) != EOF && c != '\n')
		*p++ = (char) c;
	if (c == EOF && p == buf)
		return NULL;
	*p = '\0';
	return buf;
}
