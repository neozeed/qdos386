/* string.c - QDOS/386 libc (R9): C89 memory and string functions.
 * Portable C; no QDOS calls.  Written from the C89 definitions. */
#include <string.h>
#include <stdlib.h>

void *memcpy(void *dst, const void *src, size_t n)
{
	char *d = (char *) dst;
	const char *s = (const char *) src;
	while (n-- != 0)
		*d++ = *s++;
	return dst;
}

void *memmove(void *dst, const void *src, size_t n)
{
	char *d = (char *) dst;
	const char *s = (const char *) src;
	if (d == s || n == 0)
		return dst;
	if (d < s || d >= s + n) {
		while (n-- != 0)
			*d++ = *s++;
	} else {
		d += n;
		s += n;
		while (n-- != 0)
			*--d = *--s;
	}
	return dst;
}

void *memset(void *dst, int c, size_t n)
{
	unsigned char *d = (unsigned char *) dst;
	while (n-- != 0)
		*d++ = (unsigned char) c;
	return dst;
}

int memcmp(const void *a, const void *b, size_t n)
{
	const unsigned char *p = (const unsigned char *) a;
	const unsigned char *q = (const unsigned char *) b;
	while (n-- != 0) {
		if (*p != *q)
			return (int) *p - (int) *q;
		p++;
		q++;
	}
	return 0;
}

void *memchr(const void *s, int c, size_t n)
{
	const unsigned char *p = (const unsigned char *) s;
	while (n-- != 0) {
		if (*p == (unsigned char) c)
			return (void *) p;
		p++;
	}
	return NULL;
}

size_t strlen(const char *s)
{
	size_t n = 0;
	while (s[n] != '\0')
		n++;
	return n;
}

int strcmp(const char *a, const char *b)
{
	while (*a != '\0' && *a == *b) {
		a++;
		b++;
	}
	return (int) (unsigned char) *a - (int) (unsigned char) *b;
}

int strncmp(const char *a, const char *b, size_t n)
{
	if (n == 0)
		return 0;
	while (--n != 0 && *a != '\0' && *a == *b) {
		a++;
		b++;
	}
	return (int) (unsigned char) *a - (int) (unsigned char) *b;
}

char *strcpy(char *dst, const char *src)
{
	char *d = dst;
	while ((*d++ = *src++) != '\0')
		;
	return dst;
}

char *strncpy(char *dst, const char *src, size_t n)
{
	char *d = dst;
	while (n != 0 && *src != '\0') {
		*d++ = *src++;
		n--;
	}
	while (n-- != 0)
		*d++ = '\0';
	return dst;
}

char *strcat(char *dst, const char *src)
{
	char *d = dst + strlen(dst);
	while ((*d++ = *src++) != '\0')
		;
	return dst;
}

char *strncat(char *dst, const char *src, size_t n)
{
	char *d = dst + strlen(dst);
	while (n != 0 && *src != '\0') {
		*d++ = *src++;
		n--;
	}
	*d = '\0';
	return dst;
}

char *strchr(const char *s, int c)
{
	for (;;) {
		if (*s == (char) c)
			return (char *) s;
		if (*s == '\0')
			return NULL;
		s++;
	}
}

char *strrchr(const char *s, int c)
{
	const char *last = NULL;
	for (;;) {
		if (*s == (char) c)
			last = s;
		if (*s == '\0')
			return (char *) last;
		s++;
	}
}

char *strstr(const char *hay, const char *needle)
{
	size_t n = strlen(needle);
	if (n == 0)
		return (char *) hay;
	for (; *hay != '\0'; hay++) {
		if (*hay == *needle && strncmp(hay, needle, n) == 0)
			return (char *) hay;
	}
	return NULL;
}

char *strdup(const char *s)
{
	size_t n = strlen(s) + 1;
	char *p = (char *) malloc(n);
	if (p != NULL)
		memcpy(p, s, n);
	return p;
}

/* ---- span and token functions (C89 4.11.5) ----------------------------- */

size_t strspn(const char *s, const char *accept)
{
	const char *p = s;
	while (*p != '\0' && strchr(accept, *p) != NULL)
		p++;
	return (size_t) (p - s);
}

size_t strcspn(const char *s, const char *reject)
{
	const char *p = s;
	while (*p != '\0' && strchr(reject, *p) == NULL)
		p++;
	return (size_t) (p - s);
}

char *strpbrk(const char *s, const char *accept)
{
	for (; *s != '\0'; s++)
		if (strchr(accept, *s) != NULL)
			return (char *) s;
	return NULL;
}

/* strtok keeps its position in a static pointer: it is not reentrant, as the
 * C89 standard allows.  Delimiters at the start and end are skipped; an empty
 * token never returned. */
static char *qd_tok_next;

char *strtok(char *s, const char *delim)
{
	char *start;
	char *end;

	if (s != NULL)
		qd_tok_next = s;
	if (qd_tok_next == NULL)
		return NULL;
	start = qd_tok_next + strspn(qd_tok_next, delim);
	if (*start == '\0') {
		qd_tok_next = NULL;
		return NULL;
	}
	end = start + strcspn(start, delim);
	if (*end == '\0')
		qd_tok_next = NULL;
	else {
		*end = '\0';
		qd_tok_next = end + 1;
	}
	return start;
}

/* C locale: collation is byte order, so strcoll is strcmp (C89 7.11.4.3). */
int strcoll(const char *a, const char *b)
{
	return strcmp(a, b);
}
