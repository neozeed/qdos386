/* stdio.c - QDOS/386 libc (R9): a bounded stdio subset.
 *
 *   stdout, stderr   the CON channel (write).  No buffering across calls:
 *                    each printf/puts writes its text before it returns.
 *   stdin            the CON channel (read).  getchar waits one tick at a time
 *                    while the console has nothing yet (CHAN_ERR_AGAIN).
 *   fopen            read-only files on the WIN1_ FAT16 volume.  A name that
 *                    does not start with "WIN1_" or "CON" gets "WIN1_" added;
 *                    the whole name must fit the kernel's 16-byte channel name.
 *
 * Not provided: writing files, fputs/fprintf, scanf, buffering modes,
 * SEEK_END (the kernel has no file size query), errno, locale.  See
 * docs/R9-LIBC-ABI.md. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "qdos386.h"

#define KIND_CON   1
#define KIND_FILE  2
#define CHAN_AGAIN (-19)            /* CHAN_ERR_AGAIN in the kernel */
#define OUT_CHUNK  128

struct qd_file {
	int ch;             /* channel id, or 0 if not open yet (CON only) */
	int kind;
	int eof;
	int err;
	unsigned long pos;  /* logical position (KIND_FILE): the byte the next read returns */
	int has_unget;      /* one pushed-back character (ungetc), consumed first */
	int unget_c;
};

static struct qd_file con_out = { 0, KIND_CON, 0, 0, 0 };
static struct qd_file con_in  = { 0, KIND_CON, 0, 0, 0 };
static struct qd_file con_err = { 0, KIND_CON, 0, 0, 0 };

FILE *stdin = (FILE *) &con_in;
FILE *stdout = (FILE *) &con_out;
FILE *stderr = (FILE *) &con_err;

/* The CON channel is opened once and kept.  The kernel closes it with the job. */
static int con_channel(void)
{
	static int ch = 0;
	if (ch <= 0)
		ch = qd_open("CON");
	return ch;
}

/* ---- formatted output: one engine for printf, sprintf and vprintf -------- */

struct qd_out {
	int to_con;          /* 1: write through CON in chunks; 0: into dst */
	char *dst;           /* destination for the string forms */
	unsigned long cap;   /* string forms: size of dst; writes stop at cap-1 */
	char chunk[OUT_CHUNK];
	unsigned int n;
	int total;           /* characters produced (the return value) */
};

static void out_flush(struct qd_out *o)
{
	if (o->to_con && o->n != 0) {
		int ch = con_channel();
		if (ch > 0)
			qd_write(ch, o->chunk, o->n);
	}
	o->n = 0;
}

static void out_char(struct qd_out *o, int c)
{
	if (o->to_con) {
		o->total++;
		o->chunk[o->n++] = (char) c;
		if (o->n == OUT_CHUNK)
			out_flush(o);
	} else {
		/* keep room for the terminating NUL; count everything */
		if ((unsigned long) o->total + 1 < o->cap)
			o->dst[o->total] = (char) c;
		o->total++;
	}
}

static void out_pad(struct qd_out *o, int c, int count)
{
	while (count-- > 0)
		out_char(o, c);
}

/* Emit a conversion's text with width and the '-' and '0' flags. */
static void out_field(struct qd_out *o, const char *digits, int len, int width,
                      int left, int zero, const char *sign)
{
	int slen = sign != NULL ? (int) strlen(sign) : 0;
	int pad = width - len - slen;
	if (!left && !zero)
		out_pad(o, ' ', pad);
	if (sign != NULL)
		while (*sign != '\0')
			out_char(o, *sign++);
	if (!left && zero)
		out_pad(o, '0', pad);
	while (len-- > 0)
		out_char(o, *digits++);
	if (left)
		out_pad(o, ' ', pad);
}

/* Unsigned magnitude in base 2..16 into buf (filled from the end). */
static int to_base(unsigned long v, unsigned int base, int upper, char *buf)
{
	static const char lo[] = "0123456789abcdef";
	static const char hi[] = "0123456789ABCDEF";
	const char *tab = upper ? hi : lo;
	char tmp[16];
	int n = 0, i;
	do {
		tmp[n++] = tab[v % base];
		v /= base;
	} while (v != 0);
	for (i = 0; i < n; i++)
		buf[i] = tmp[n - 1 - i];
	return n;
}

static int qd_vformat(struct qd_out *o, const char *fmt, va_list ap)
{
	for (; *fmt != '\0'; fmt++) {
		int left = 0, zero = 0, width = 0;
		char digits[16];
		int len;

		if (*fmt != '%') {
			out_char(o, *fmt);
			continue;
		}
		fmt++;
		for (;; fmt++) {
			if (*fmt == '-')
				left = 1;
			else if (*fmt == '0')
				zero = 1;
			else
				break;
		}
		while (*fmt >= '0' && *fmt <= '9')
			width = width * 10 + (*fmt++ - '0');
		if (*fmt == 'l' || *fmt == 'h')    /* int and long are both 32 bits */
			fmt++;
		switch (*fmt) {
		case 'd':
		case 'i': {
			long v = va_arg(ap, long);
			unsigned long m = v < 0 ? 0UL - (unsigned long) v : (unsigned long) v;
			len = to_base(m, 10, 0, digits);
			out_field(o, digits, len, width, left, zero && !left, v < 0 ? "-" : NULL);
			break;
		}
		case 'u':
		case 'x':
		case 'X':
		case 'o': {
			unsigned long v = va_arg(ap, unsigned long);
			unsigned int base = *fmt == 'u' ? 10 : *fmt == 'o' ? 8 : 16;
			len = to_base(v, base, *fmt == 'X', digits);
			out_field(o, digits, len, width, left, zero && !left, NULL);
			break;
		}
		case 'c': {
			int c = va_arg(ap, int);
			out_field(o, (char *) &c, 1, width, left, 0, NULL);
			break;
		}
		case 's': {
			const char *s = va_arg(ap, const char *);
			if (s == NULL)
				s = "(null)";
			len = (int) strlen(s);
			out_field(o, s, len, width, left, 0, NULL);
			break;
		}
		case '%':
			out_char(o, '%');
			break;
		case '\0':
			return o->total;
		default:
			/* Unknown conversion: copied as written. */
			out_char(o, '%');
			out_char(o, *fmt);
			break;
		}
	}
	return o->total;
}

int vprintf(const char *fmt, va_list ap)
{
	struct qd_out o;
	int n;
	o.to_con = 1;
	o.dst = NULL;
	o.n = 0;
	o.total = 0;
	n = qd_vformat(&o, fmt, ap);
	out_flush(&o);
	return n;
}

int printf(const char *fmt, ...)
{
	va_list ap;
	int n;
	va_start(ap, fmt);
	n = vprintf(fmt, ap);
	va_end(ap);
	return n;
}

/* String forms share one core.  sprintf is unbounded (the caller must size
 * the buffer); snprintf and vsnprintf never write more than size-1 characters
 * plus the NUL, and return the length the full output would have had (C99). */
static int qd_format_to(char *buf, unsigned long cap, const char *fmt, va_list ap)
{
	struct qd_out o;
	int n;
	o.to_con = 0;
	o.dst = buf;
	o.cap = cap;
	o.n = 0;
	o.total = 0;
	n = qd_vformat(&o, fmt, ap);
	if (cap > 0) {
		unsigned long at = (unsigned long) n < cap - 1 ? (unsigned long) n : cap - 1;
		buf[at] = '\0';
	}
	return n;
}

int sprintf(char *buf, const char *fmt, ...)
{
	va_list ap;
	int n;
	va_start(ap, fmt);
	n = qd_format_to(buf, (unsigned long) -1, fmt, ap);
	va_end(ap);
	return n;
}

int vsnprintf(char *buf, size_t size, const char *fmt, va_list ap)
{
	return qd_format_to(buf, (unsigned long) size, fmt, ap);
}

int snprintf(char *buf, size_t size, const char *fmt, ...)
{
	va_list ap;
	int n;
	va_start(ap, fmt);
	n = qd_format_to(buf, (unsigned long) size, fmt, ap);
	va_end(ap);
	return n;
}

int putchar(int c)
{
	int ch = con_channel();
	if (ch <= 0)
		return EOF;
	qd_putc(ch, c);
	return (unsigned char) c;
}

int puts(const char *s)
{
	int ch = con_channel();
	size_t n;
	if (ch <= 0)
		return EOF;
	n = strlen(s);
	if (n != 0)
		qd_write(ch, s, (unsigned int) n);
	qd_putc(ch, '\n');
	return 0;
}

/* ---- input ----------------------------------------------------------------- */

int fgetc(FILE *fp)
{
	struct qd_file *f = (struct qd_file *) fp;
	unsigned char c;
	int got;

	if (f == NULL)
		return EOF;
	if (f->has_unget) {
		f->has_unget = 0;
		if (f->kind == KIND_FILE)
			f->pos++;
		return f->unget_c;
	}
	if (f->kind == KIND_CON) {
		int ch = con_channel();
		if (ch <= 0)
			return EOF;
		for (;;) {
			got = qd_read(ch, &c, 1);
			if (got == 1)
				return c;
			if (got == 0) {
				f->eof = 1;
				return EOF;
			}
			if (got != CHAN_AGAIN) {
				f->err = 1;
				return EOF;
			}
			qd_yield(1);     /* nothing typed yet: sleep one tick, retry */
		}
	}
	if (fread(&c, 1, 1, fp) == 1)
		return c;
	return EOF;
}

int getc(FILE *fp)
{
	return fgetc(fp);
}

int getchar(void)
{
	return fgetc(stdin);
}

/* One character of pushback.  For a file the logical position moves back by
 * one, so ftell and the next read agree.  A second ungetc before a read fails,
 * as C89 allows only one guaranteed pushback. */
int ungetc(int c, FILE *fp)
{
	struct qd_file *f = (struct qd_file *) fp;
	if (c == EOF || f == NULL || f->has_unget)
		return EOF;
	if (f->kind == KIND_FILE) {
		if (f->pos == 0)
			return EOF;
		f->pos--;
	}
	f->has_unget = 1;
	f->unget_c = (unsigned char) c;
	f->eof = 0;
	return (unsigned char) c;
}

/* Reads at most n-1 characters, stopping after a newline (which is kept).
 * Returns NULL if nothing was read (end of file or error). */
char *fgets(char *buf, int n, FILE *fp)
{
	int i = 0;
	int c;

	if (buf == NULL || n <= 0 || fp == NULL)
		return NULL;
	while (i < n - 1) {
		c = fgetc(fp);
		if (c == EOF)
			break;
		buf[i++] = (char) c;
		if (c == '\n')
			break;
	}
	if (i == 0)
		return NULL;
	buf[i] = '\0';
	return buf;
}

FILE *fopen(const char *name, const char *mode)
{
	char full[FILENAME_MAX];
	struct qd_file *f;
	int ch;
	size_t n;

	if (name == NULL || mode == NULL)
		return NULL;
	if (mode[0] != 'r' || (mode[1] != '\0' && !(mode[1] == 'b' && mode[2] == '\0')))
		return NULL;                 /* read only */
	if (strncmp(name, "WIN1_", 5) == 0 || strcmp(name, "CON") == 0) {
		n = strlen(name);
		if (n >= FILENAME_MAX)
			return NULL;
		strcpy(full, name);
	} else {
		n = strlen(name) + 5;
		if (n >= FILENAME_MAX)
			return NULL;
		strcpy(full, "WIN1_");
		strcat(full, name);
	}
	ch = qd_open(full);
	if (ch <= 0)
		return NULL;
	f = (struct qd_file *) malloc(sizeof(struct qd_file));
	if (f == NULL) {
		qd_close(ch);
		return NULL;
	}
	f->ch = ch;
	f->kind = KIND_FILE;
	f->eof = 0;
	f->err = 0;
	f->pos = 0;
	f->has_unget = 0;
	f->unget_c = 0;
	return (FILE *) f;
}

int fclose(FILE *fp)
{
	struct qd_file *f = (struct qd_file *) fp;
	if (f == NULL || f == &con_in || f == &con_out || f == &con_err)
		return 0;                    /* the standard streams stay open */
	qd_close(f->ch);
	free(f);
	return 0;
}

size_t fread(void *ptr, size_t size, size_t nmemb, FILE *fp)
{
	struct qd_file *f = (struct qd_file *) fp;
	unsigned long want, total = 0;
	char *p = (char *) ptr;

	if (f == NULL || f->kind != KIND_FILE || size == 0 || nmemb == 0)
		return 0;
	if (nmemb > (size_t) -1 / size) {
		f->err = 1;
		return 0;
	}
	want = (unsigned long) (size * nmemb);
	if (f->has_unget && want > 0) {
		p[0] = (char) f->unget_c;
		f->has_unget = 0;
		f->pos++;
		total = 1;
	}
	while (total < want) {
		unsigned int chunk = (unsigned int) (want - total);
		int got;
		if (chunk > QD_READ_MAX)
			chunk = QD_READ_MAX;
		got = qd_read(f->ch, p + total, chunk);
		if (got > 0) {
			total += (unsigned long) got;
			f->pos += (unsigned long) got;
			continue;
		}
		if (got == 0)
			f->eof = 1;
		else
			f->err = 1;
		break;
	}
	return (size_t) (total / size);
}

int fseek(FILE *fp, long off, int whence)
{
	struct qd_file *f = (struct qd_file *) fp;
	long target;

	if (f == NULL || f->kind != KIND_FILE)
		return -1;
	if (whence == SEEK_SET)
		target = off;
	else if (whence == SEEK_CUR)
		target = (long) f->pos + off;
	else
		return -1;                   /* SEEK_END: no file size in the kernel */
	if (target < 0)
		return -1;
	if (qd_seek(f->ch, (unsigned long) target) < 0)
		return -1;
	f->pos = (unsigned long) target;
	f->eof = 0;
	f->has_unget = 0;                /* seeking discards a pushed-back character */
	return 0;
}

long ftell(FILE *fp)
{
	struct qd_file *f = (struct qd_file *) fp;
	if (f == NULL || f->kind != KIND_FILE)
		return -1L;
	return (long) f->pos;
}

int feof(FILE *fp)
{
	return fp != NULL && ((struct qd_file *) fp)->eof;
}

int ferror(FILE *fp)
{
	return fp != NULL && ((struct qd_file *) fp)->err;
}

void clearerr(FILE *fp)
{
	if (fp != NULL) {
		((struct qd_file *) fp)->eof = 0;
		((struct qd_file *) fp)->err = 0;
	}
}
