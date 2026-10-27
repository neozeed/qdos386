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

void exit(int status)
{
	/* stdio writes are not buffered across calls (printf flushes at its end),
	 * so there is nothing to flush before the kernel reclaims the job. */
	qd_exit(status);
}
