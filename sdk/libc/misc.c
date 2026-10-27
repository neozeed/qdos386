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

/* ---- files that cannot be changed ---------------------------------------- */

int remove(const char *name)
{
	(void) name;
	return -1;                   /* FAT16 volume is read only */
}

int rename(const char *from, const char *to)
{
	(void) from;
	(void) to;
	return -1;
}

FILE *tmpfile(void)
{
	return NULL;                 /* no writable file to create */
}

char *tmpnam(char *s)
{
	(void) s;
	return NULL;
}

/* C89 says freopen closes the stream first.  Here it fails with NULL and the
 * stream is left open (a documented deviation). */
FILE *freopen(const char *name, const char *mode, FILE *fp)
{
	(void) name;
	(void) mode;
	(void) fp;
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
