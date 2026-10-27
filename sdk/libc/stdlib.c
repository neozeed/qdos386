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

void exit(int status)
{
	qd_flush_std();
	qd_exit(status);
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
