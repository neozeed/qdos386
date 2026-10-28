/* stdlib.c - QDOS/386 libc (R9): number conversion, abs, exit. */
#include <stdlib.h>
#include <limits.h>
#include "qdos386.h"

static int qd_isspace(int c)
{
	return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v';
}

static int qd_digit(int c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'a' && c <= 'z')
		return c - 'a' + 10;
	if (c >= 'A' && c <= 'Z')
		return c - 'A' + 10;
	return 99;
}

/* Shared body of strtol and strtoul.  Returns the magnitude; *neg is set for
 * a minus sign; *over is set if the magnitude does not fit in 32 bits. */
static unsigned long qd_scan(const char *s, char **end, int base, int *neg, int *over)
{
	const char *p = s;
	unsigned long acc = 0;
	int any = 0;

	*neg = 0;
	*over = 0;
	while (qd_isspace((unsigned char) *p))
		p++;
	if (*p == '+')
		p++;
	else if (*p == '-') {
		*neg = 1;
		p++;
	}
	if ((base == 0 || base == 16) && p[0] == '0' && (p[1] == 'x' || p[1] == 'X')) {
		base = 16;
		p += 2;
	} else if (base == 0 && p[0] == '0') {
		base = 8;
	} else if (base == 0) {
		base = 10;
	}
	if (base < 2 || base > 36) {
		if (end != NULL)
			*end = (char *) s;
		return 0;
	}
	for (;;) {
		int d = qd_digit((unsigned char) *p);
		if (d >= base)
			break;
		if (!*over) {
			if (acc > (ULONG_MAX - (unsigned long) d) / (unsigned long) base)
				*over = 1;
			else
				acc = acc * (unsigned long) base + (unsigned long) d;
		}
		any = 1;
		p++;
	}
	if (end != NULL)
		*end = (char *) (any ? p : s);
	return any ? acc : 0;
}

long strtol(const char *s, char **end, int base)
{
	int neg, over;
	unsigned long m = qd_scan(s, end, base, &neg, &over);
	if (neg) {
		if (over || m > 2147483648UL)
			return LONG_MIN;
		return (long) (0UL - m);
	}
	if (over || m > (unsigned long) LONG_MAX)
		return LONG_MAX;
	return (long) m;
}

unsigned long strtoul(const char *s, char **end, int base)
{
	int neg, over;
	unsigned long m = qd_scan(s, end, base, &neg, &over);
	if (over)
		return ULONG_MAX;
	return neg ? 0UL - m : m;
}

int atoi(const char *s)
{
	return (int) strtol(s, NULL, 10);
}

long atol(const char *s)
{
	return strtol(s, NULL, 10);
}

int abs(int n)
{
	return n < 0 ? -n : n;
}

long labs(long n)
{
	return n < 0 ? -n : n;
}

/* Defined in stdio.c: writes out pending stdout and stderr text. */
void qd_flush_std(void);
/* Defined in lib/qdos.c: records the exit status without ending the job. */
void qd_exit_record(int status);

/* ---- exit, atexit, abort ----------------------------------------------- */

#define QD_ATEXIT_MAX 32
static void (*qd_atexit_fn[QD_ATEXIT_MAX])(void);
static int qd_atexit_n = 0;

int atexit(void (*fn)(void))
{
	if (fn == NULL || qd_atexit_n >= QD_ATEXIT_MAX)
		return 1;                 /* fails: the table is full */
	qd_atexit_fn[qd_atexit_n++] = fn;
	return 0;
}

/* exit runs the atexit functions in reverse order of registration, then
 * flushes stdio, then ends the job.  Return from main reaches exit through
 * crt0_libc.s. */
/* Shared part of exit and of main's return: the atexit functions, then stdio. */
void qd_close_std(void);          /* stdio.c */

static void qd_run_exit_hooks(void)
{
	while (qd_atexit_n > 0) {
		void (*fn)(void) = qd_atexit_fn[--qd_atexit_n];
		fn();
	}
	qd_flush_std();
	qd_close_std();
#ifdef QD_MEMSTAT
	{ extern void qd_ms_report(const char *); qd_ms_report("run"); }
#endif
}

void exit(int status)
{
	qd_run_exit_hooks();
	qd_exit(status);
}

/* Called by crt0_libc.s when main returns: runs the same hooks, records the
 * status, and returns so that the job ends with its entry point. */
void qd_main_exit(int status)
{
	qd_run_exit_hooks();
	qd_exit_record(status);
}

/* abort ends the job with status 3.  C89 requires termination; it does not
 * require the stdio buffers to be flushed, so they are discarded here. */
void abort(void)
{
	qd_exit(3);
}

/* ---- environment and command processor ----------------------------------- */

/* QDOS/386 has no environment: getenv always finds nothing. */
char *getenv(const char *name)
{
	(void) name;
	return NULL;
}

/* There is no command processor.  system(NULL) reports that none exists
 * (0); any command fails (-1). */
int system(const char *cmd)
{
	return cmd == NULL ? 0 : -1;
}

/* ---- rand and srand: the C89 example generator (4.10.2.2) ------------------ */

static unsigned long qd_rand_next = 1;

int rand(void)
{
	qd_rand_next = qd_rand_next * 1103515245UL + 12345UL;
	return (int) ((qd_rand_next / 65536UL) % 32768UL);
}

void srand(unsigned int seed)
{
	qd_rand_next = (unsigned long) seed;
}

/* ---- div, ldiv, mblen (C locale) ------------------------------------------- */

div_t div(int num, int den)
{
	div_t r;
	r.quot = num / den;          /* C89: truncation toward zero */
	r.rem = num % den;
	return r;
}

ldiv_t ldiv(long num, long den)
{
	ldiv_t r;
	r.quot = num / den;
	r.rem = num % den;
	return r;
}

int mblen(const char *s, size_t n)
{
	if (s == NULL)
		return 0;                 /* the C locale has no shift states */
	if (n == 0)
		return -1;
	return *s == '\0' ? 0 : 1;
}

/* ---- qsort and bsearch (C89 4.10.5) ------------------------------------ */

static void qd_swap(char *a, char *b, size_t n)
{
	while (n-- > 0) {
		char t = *a;
		*a++ = *b;
		*b++ = t;
	}
}

/* Sift the element at root down within the first n elements (max-heap). */
static void qd_sift(char *base, size_t size, size_t root, size_t n,
		    int (*cmp)(const void *, const void *))
{
	for (;;) {
		size_t child = 2 * root + 1;
		if (child >= n)
			return;
		if (child + 1 < n && cmp(base + child * size, base + (child + 1) * size) < 0)
			child++;
		if (cmp(base + root * size, base + child * size) >= 0)
			return;
		qd_swap(base + root * size, base + child * size, size);
		root = child;
	}
}

/* Heapsort: O(n log n), no recursion, no extra memory, so the stack use is the
 * same for any input.  It is not stable (C89 does not require stability). */
void qsort(void *base, size_t nmemb, size_t size, int (*cmp)(const void *, const void *))
{
	char *b = (char *) base;
	size_t n = nmemb;
	size_t i;

	if (n < 2 || size == 0 || cmp == NULL)
		return;
	for (i = n / 2; i-- > 0; )
		qd_sift(b, size, i, n, cmp);
	while (n-- > 1) {
		qd_swap(b, b + n * size, size);
		qd_sift(b, size, 0, n, cmp);
	}
}

void *bsearch(const void *key, const void *base, size_t nmemb, size_t size,
	      int (*cmp)(const void *, const void *))
{
	const char *b = (const char *) base;
	size_t lo = 0;
	size_t hi = nmemb;

	if (size == 0 || cmp == NULL)
		return NULL;
	while (lo < hi) {
		size_t mid = lo + (hi - lo) / 2;
		const char *p = b + mid * size;
		int r = cmp(key, p);
		if (r == 0)
			return (void *) p;
		if (r < 0)
			hi = mid;
		else
			lo = mid + 1;
	}
	return NULL;
}
