/* scanf.c - QDOS/386 libc (R11D): the C89 input conversions.
 *
 * One engine serves scanf and fscanf (a FILE) and sscanf and vsscanf (a
 * string).  The input is read one character at a time through a single
 * pushback slot; the conversions are:
 *   d i u o x X p (integers, with *, width, h l)  c s [set]  n  e f g E G (floats)
 *   %%  and white space in the format (skips input white space).
 * Return value: the number of assignments, or EOF when input fails before the
 * first conversion.  A matching failure stops the scan and returns the count.
 * Integer overflow wraps (C89 leaves it undefined).  Floats use strtod, so
 * they are correctly rounded.  See docs/R11D-LIBC-PORT-MAP.md. */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <string.h>

#define QD_FIELD_MAX 256            /* longest collected numeric or float field */

struct qd_src {
	const char *s;              /* string source (fp == NULL) */
	FILE *fp;                   /* stream source */
	int pb;                     /* pushed-back character, or -1 */
	int nread;                  /* characters consumed (for %n) */
};

static int src_get(struct qd_src *src)
{
	int c;
	if (src->pb >= 0) {
		c = src->pb;
		src->pb = -1;
	} else if (src->fp != NULL) {
		c = fgetc(src->fp);
	} else {
		c = (*src->s == '\0') ? EOF : (unsigned char) *src->s++;
	}
	if (c != EOF)
		src->nread++;
	return c;
}

static void src_unget(struct qd_src *src, int c)
{
	if (c == EOF)
		return;
	src->pb = c;
	src->nread--;
}

static void src_skip_space(struct qd_src *src)
{
	int c;
	do
		c = src_get(src);
	while (c != EOF && isspace(c));
	src_unget(src, c);
}

static int digit_value(int c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	return 99;
}

/* Collected field: characters that satisfy a class, up to width. */
struct qd_field {
	char buf[QD_FIELD_MAX + 1];
	int n;
};

static void field_add(struct qd_field *f, int c)
{
	if (f->n < QD_FIELD_MAX)
		f->buf[f->n++] = (char) c;
	f->buf[f->n] = '\0';
}

/* Integer conversion of the collected field in the given base.  The sign is
 * the first character when it is '+' or '-'. */
static unsigned long field_value(const char *p, int base, int *neg)
{
	unsigned long v = 0;
	*neg = 0;
	if (*p == '+' || *p == '-') {
		*neg = (*p == '-');
		p++;
	}
	while (*p != '\0') {
		v = v * (unsigned long) base + (unsigned long) digit_value((unsigned char) *p);
		p++;
	}
	return v;
}

static int qd_vscan(struct qd_src *src, const char *fmt, va_list ap)
{
	int count = 0;
	int c;

	while (*fmt != '\0') {
		if (isspace((unsigned char) *fmt)) {
			src_skip_space(src);
			fmt++;
			continue;
		}
		if (*fmt != '%') {
			c = src_get(src);
			if (c == EOF)
				return count == 0 ? EOF : count;
			if (c != (unsigned char) *fmt) {
				src_unget(src, c);
				return count;
			}
			fmt++;
			continue;
		}
		fmt++;                  /* past '%' */
		if (*fmt == '%') {
			src_skip_space(src);
			c = src_get(src);
			if (c == EOF)
				return count == 0 ? EOF : count;
			if (c != '%') {
				src_unget(src, c);
				return count;
			}
			fmt++;
			continue;
		}
		{
			int suppress = 0, lenmod = 0, width = 0, base = 10;
			char conv;
			struct qd_field f;
			int neg = 0;

			if (*fmt == '*') {
				suppress = 1;
				fmt++;
			}
			while (*fmt >= '0' && *fmt <= '9') {
				width = width * 10 + (*fmt - '0');
				fmt++;
			}
			if (*fmt == 'h' || *fmt == 'l' || *fmt == 'L') {
				lenmod = *fmt;
				fmt++;
			}
			conv = *fmt;
			if (conv == '\0')
				return count;
			fmt++;
			f.n = 0;
			f.buf[0] = '\0';

			if (conv == 'n') {
				if (!suppress)
					*va_arg(ap, int *) = src->nread;
				continue;
			}
			if (conv == 'c') {
				int want = width > 0 ? width : 1;
				char *dst = suppress ? NULL : va_arg(ap, char *);
				int i;
				for (i = 0; i < want; i++) {
					c = src_get(src);
					if (c == EOF)
						return (count == 0 && i == 0) ? EOF : count;
					if (dst != NULL)
						dst[i] = (char) c;
				}
				if (!suppress)
					count++;
				continue;
			}
			if (conv != '[')
				src_skip_space(src);
			if (conv == 's') {
				char *dst = suppress ? NULL : va_arg(ap, char *);
				int n = 0;
				c = src_get(src);
				if (c == EOF)
					return count == 0 ? EOF : count;
				while (c != EOF && !isspace(c) && (width == 0 || n < width)) {
					if (dst != NULL)
						dst[n] = (char) c;
					n++;
					c = src_get(src);
				}
				src_unget(src, c);
				if (dst != NULL)
					dst[n] = '\0';
				if (!suppress)
					count++;
				continue;
			}
			if (conv == '[') {
				char *dst = suppress ? NULL : va_arg(ap, char *);
				const char *set = fmt;
				int invert = 0, n = 0;
				if (*fmt == '^') {
					invert = 1;
					fmt++;
					set = fmt;
				}
				/* the set ends at the first ']' that is not the first character */
				if (*fmt == ']')
					fmt++;
				while (*fmt != '\0' && *fmt != ']')
					fmt++;
				if (*fmt == '\0')
					return count;
				{
					const char *end = fmt;
					int inset = 0;
					c = src_get(src);
					if (c == EOF)
						return count == 0 ? EOF : count;
					while (c != EOF) {
						const char *p = set;
						inset = 0;
						while (p < end) {
							if (p[1] == '-' && p + 2 < end) {
								if ((unsigned char) c >= (unsigned char) p[0] &&
								    (unsigned char) c <= (unsigned char) p[2])
									inset = 1;
								p += 3;
							} else {
								if (c == (unsigned char) *p)
									inset = 1;
								p++;
							}
						}
						if (inset == invert || (width != 0 && n >= width))
							break;
						if (dst != NULL)
							dst[n] = (char) c;
						n++;
						c = src_get(src);
					}
					src_unget(src, c);
					if (n == 0)
						return count;
				}
				fmt++;          /* past ']' */
				if (dst != NULL)
					dst[n] = '\0';
				if (!suppress)
					count++;
				continue;
			}
			if (conv == 'd' || conv == 'i' || conv == 'u' || conv == 'o' ||
			    conv == 'x' || conv == 'X' || conv == 'p') {
				int max = width > 0 ? width : QD_FIELD_MAX;
				c = src_get(src);
				if (c == EOF)
					return count == 0 ? EOF : count;
				if (c == '+' || c == '-') {
					field_add(&f, c);
					c = src_get(src);
				}
				if (conv == 'i' || conv == 'x' || conv == 'X' || conv == 'p') {
					if (c == '0') {
						field_add(&f, c);
						c = src_get(src);
						if (c == 'x' || c == 'X') {
							base = 16;          /* the 0x prefix is consumed, not stored */
							c = src_get(src);
							f.n = 0;
							f.buf[0] = '\0';
						} else if (conv == 'i') {
							base = 8;
						} else {
							base = (conv == 'x' || conv == 'X' || conv == 'p') ? 16 : 10;
						}
					} else if (conv == 'i') {
						base = 10;
					} else {
						base = 16;
					}
				} else if (conv == 'o') {
					base = 8;
				} else if (conv == 'u' || conv == 'd') {
					base = 10;
				}
				while (c != EOF && f.n < max && digit_value(c) < base) {
					field_add(&f, c);
					c = src_get(src);
				}
				src_unget(src, c);
				if (f.n == 0 || (f.n == 1 && (f.buf[0] == '+' || f.buf[0] == '-')))
					return count;
				{
					unsigned long v = field_value(f.buf, base, &neg);
					if (neg)
						v = (unsigned long) (-(long) v);
					if (!suppress) {
						if (lenmod == 'l') {
							*va_arg(ap, long *) = (long) v;
						} else if (lenmod == 'h') {
							*va_arg(ap, short *) = (short) v;
						} else {
							*va_arg(ap, int *) = (int) v;
						}
						count++;
					}
				}
				continue;
			}
			if (conv == 'e' || conv == 'E' || conv == 'f' || conv == 'g' || conv == 'G') {
				int max = width > 0 ? width : QD_FIELD_MAX;
				int seen_digit = 0;
				c = src_get(src);
				if (c == EOF)
					return count == 0 ? EOF : count;
				if (c == '+' || c == '-') {
					field_add(&f, c);
					c = src_get(src);
				}
				while (c != EOF && f.n < max && isdigit(c)) {
					field_add(&f, c);
					seen_digit = 1;
					c = src_get(src);
				}
				if (c == '.' && f.n < max) {
					field_add(&f, c);
					c = src_get(src);
					while (c != EOF && f.n < max && isdigit(c)) {
						field_add(&f, c);
						seen_digit = 1;
						c = src_get(src);
					}
				}
				if (!seen_digit) {
					src_unget(src, c);
					return count;
				}
				if ((c == 'e' || c == 'E') && f.n < max) {
					field_add(&f, c);
					c = src_get(src);
					if ((c == '+' || c == '-') && f.n < max) {
						field_add(&f, c);
						c = src_get(src);
					}
					while (c != EOF && f.n < max && isdigit(c)) {
						field_add(&f, c);
						c = src_get(src);
					}
				}
				src_unget(src, c);
				if (!suppress) {
					double v = strtod(f.buf, NULL);
					if (lenmod == 'l' || lenmod == 'L')
						*va_arg(ap, double *) = v;
					else
						*va_arg(ap, float *) = (float) v;
					count++;
				}
				continue;
			}
			/* unknown conversion: stop the scan */
			return count;
		}
	}
	return count;
}

int vsscanf(const char *s, const char *fmt, va_list ap)
{
	struct qd_src src;
	src.s = s;
	src.fp = NULL;
	src.pb = -1;
	src.nread = 0;
	return qd_vscan(&src, fmt, ap);
}

int sscanf(const char *s, const char *fmt, ...)
{
	va_list ap;
	int n;
	va_start(ap, fmt);
	n = vsscanf(s, fmt, ap);
	va_end(ap);
	return n;
}

int fscanf(FILE *fp, const char *fmt, ...)
{
	struct qd_src src;
	va_list ap;
	int n;
	src.s = NULL;
	src.fp = fp;
	src.pb = -1;
	src.nread = 0;
	va_start(ap, fmt);
	n = qd_vscan(&src, fmt, ap);
	va_end(ap);
	if (src.pb >= 0)
		ungetc(src.pb, fp);      /* the pushed-back character stays unread */
	return n;
}

int scanf(const char *fmt, ...)
{
	struct qd_src src;
	va_list ap;
	int n;
	src.s = NULL;
	src.fp = stdin;
	src.pb = -1;
	src.nread = 0;
	va_start(ap, fmt);
	n = qd_vscan(&src, fmt, ap);
	va_end(ap);
	if (src.pb >= 0)
		ungetc(src.pb, stdin);
	return n;
}
