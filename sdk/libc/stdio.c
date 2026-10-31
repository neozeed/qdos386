/* stdio.c - QDOS/386 libc (R11B): C89 stream I/O over the QDOS channels.
 *
 *   stdin            the CON channel, read.  The Enter key sends CR, which is
 *                    returned as LF.  Before a read waits for input, pending
 *                    stdout text is flushed.  getchar waits one tick
 *                    at a time while the console has nothing (CHAN_ERR_AGAIN).
 *   stdout           the CON channel, write, LINE buffered (flushed at each
 *                    newline, when full, by fflush, before stdin reads, and at
 *                    exit, including return from main).
 *   stderr           the CON channel, write, UNBUFFERED (each call is written
 *                    as it is produced).  The kernel has no separate error
 *                    channel, so stderr and stdout reach the same console.
 *   fopen            files on the WIN1_ volume.  "r" works on every kernel.  The
 *                    write modes ("w", "a", "r+", "w+", "a+", with or without b)
 *                    need a kernel built with -DQDOS_FS_WRITE (R11L); on any
 *                    other kernel, or on a write-protected volume, they return
 *                    NULL with errno set (ENOSYS or EROFS).  File streams have
 *                    one 512-byte buffer for reading and writing; names are
 *                    folded to upper case.
 *
 * Deviations from C89 are listed in docs/R11B-STDIO-ABI.md.  In short:
 * setvbuf/setbuf accept a NULL buffer only (the library owns its buffers),
 * fflush on an input stream returns 0 and does nothing, errno is not set,
 * and SEEK_END is not supported (the kernel has no file size query). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include "fpbig.h"
#include "qdos386.h"

#define KIND_CON   1
#define KIND_FILE  2
#define DIR_READ   1
#define DIR_WRITE  2
#define FB_SIZE    512u             /* R11L: file stream buffer */
#define CHAN_AGAIN (-19)            /* CHAN_ERR_AGAIN in the kernel */
#define WBUF_SIZE  256              /* stdout buffer; the kernel takes writes of any size */

struct qd_file {
	int ch;             /* channel id, or 0 if not open yet (CON only) */
	int kind;           /* KIND_CON or KIND_FILE */
	int dir;            /* DIR_READ or DIR_WRITE: the only direction allowed */
	int eof;
	int err;
	unsigned long pos;  /* logical position (KIND_FILE): the byte the next read returns */
	int has_unget;      /* one pushed-back character (ungetc), consumed first */
	int unget_c;
	int bufmode;        /* _IOFBF, _IOLBF or _IONBF (output streams) */
	unsigned int wlen;  /* bytes waiting in wbuf */
	unsigned char wbuf[WBUF_SIZE];
	/* R11L: file streams (KIND_FILE) */
	unsigned char *fb;  /* 512-byte buffer, heap; shared by reads and writes */
	unsigned int fb_mode;   /* 0 empty, 1 holds read data, 2 holds pending written data */
	unsigned int fb_len;    /* valid bytes (mode 1) or pending bytes (mode 2) */
	unsigned int fb_cur;    /* read cursor (mode 1) */
	unsigned long fb_off;   /* file offset of fb[0] */
	unsigned int fflags;    /* QD_O_* flags the file was opened with */
};

static struct qd_file con_in  = { 0, KIND_CON, DIR_READ,  0, 0, 0, 0, 0, _IONBF, 0, { 0 } };
static struct qd_file con_out = { 0, KIND_CON, DIR_WRITE, 0, 0, 0, 0, 0, _IOLBF, 0, { 0 } };
static struct qd_file con_err = { 0, KIND_CON, DIR_WRITE, 0, 0, 0, 0, 0, _IONBF, 0, { 0 } };

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

int qd_con_channel(void)
{
	return con_channel();
}

/* posixio.c */
int qd_fd_for_channel(int ch);
void qd_fd_forget(int ch);
int qd_fd_take(int fd, int *flags);

/* ---- output buffering ------------------------------------------------------ */

/* Write everything waiting in the buffer.  Returns 0, or EOF with err set. */
static int file_flush_w(struct qd_file *f);

#ifdef QDOS_C68_CONWRITE
/* R14A, C68 libc variant only (libc68.a; docs/R14A-LIBVT-ANSI.md): C68 libc 4.24.5's console write vector.  When a
 * program sets it (Hack: `_conwrite = ANSI_conwrite`), console text of stdout and stderr is offered to it with C68's
 * protocol: a result > 0 lets that many bytes go to the CON channel unchanged, < 0 says that many were consumed,
 * 0 says all is done.  The UFB has the leading fields of C68's struct UFB (sdk/qlcompat/include/ansicondef.h).
 * NULL (the default) means plain console output.  libc.a is built without this and is unchanged. */
struct UFB {
	char ufbtyp, ufbst;
	short ufbflg;
	long ufbfh, ufbfh1, ufbjob;
	char *ufbnam;
};
int (*_conwrite)(struct UFB *, void *, unsigned int) = 0;
static struct UFB c68_con_ufb;

/* C68 libc 4.24.5's end message (defaults/endmsg.c, unistd/_exit.c): when stdout is a console the program owns,
 * exit writes _endmsg at the cursor and waits for a key (ESC: one more key), so a program that ends in its own window
 * leaves its last screen readable.  The QDOS/386 equivalent of "owns the console" is a private session with this
 * job as its only user (START ... /NEW); in a shared session (the shell's) nothing is shown, as C68 does for an
 * inherited channel.  A program sets _endmsg = NULL to suppress it.  Armed on the first console use. */
char *_endmsg = "Press a key to exit";
static int c68_end_armed;

static void c68_endmsg_hook(void)
{
	struct qd_surface_info si;
	int ch = con_channel(), k;
	(void) fflush(stdout);
	(void) fflush(stderr);
	if (_endmsg == 0 || ch <= 0 || qd_surface_info(0, &si) != 0)
		return;
	if ((si.flags & 2u) == 0 || si.users != 1)
		return;
	(void) qd_write(ch, _endmsg, (unsigned int) strlen(_endmsg));
	k = qd_con_getkey(ch);
	if (k == 27)
		(void) qd_con_getkey(ch);
}

static void c68_arm_end(void)
{
	if (!c68_end_armed) {
		c68_end_armed = 1;
		(void) atexit(c68_endmsg_hook);
	}
}

static int c68_conwrite(int ch, unsigned char *p, unsigned int left)
{
	int r;
	c68_con_ufb.ufbtyp = 2;                  /* D_CON */
	c68_con_ufb.ufbfh = ch;
	c68_con_ufb.ufbnam = "CON";
	while (left > 0) {
		r = _conwrite(&c68_con_ufb, p, left);
		if (r == 0)
			break;
		if (r > 0) {
			if ((unsigned int) r > left)
				r = (int) left;
			if (qd_write(ch, p, (unsigned int) r) != r)
				return EOF;
		} else {
			r = -r;
			if ((unsigned int) r > left)
				r = (int) left;
		}
		p += r;
		left -= (unsigned int) r;
	}
	return 0;
}
#endif

static int stream_flush(struct qd_file *f)
{
	int ch;

	if (f->kind == KIND_FILE)
		return file_flush_w(f);
	if (f->wlen == 0)
		return 0;
#ifdef QDOS_C68_CONWRITE
	c68_arm_end();
	if (_conwrite != 0) {
		ch = con_channel();
		if (ch <= 0 || c68_conwrite(ch, (unsigned char *) f->wbuf, f->wlen) != 0) {
			f->wlen = 0;
			f->err = 1;
			return EOF;
		}
		f->wlen = 0;
		return 0;
	}
#endif
	ch = con_channel();
	if (ch <= 0 || qd_write(ch, f->wbuf, f->wlen) != (int) f->wlen) {
		f->wlen = 0;
		f->err = 1;
		return EOF;
	}
	f->wlen = 0;
	return 0;
}

/* Put one character into an output stream.  Returns 0, or EOF with err set
 * (wrong direction, or a failed write while flushing). */
static int file_putc(struct qd_file *f, int c);

static int stream_putc(struct qd_file *f, int c)
{
	if (f->kind == KIND_FILE)
		return file_putc(f, c);
	if (f->dir != DIR_WRITE || f->kind != KIND_CON) {
		f->err = 1;
		return EOF;
	}
	f->wbuf[f->wlen++] = (unsigned char) c;
	if (f->wlen == WBUF_SIZE || (f->bufmode == _IOLBF && c == '\n'))
		return stream_flush(f);
	return 0;
}

/* End of one public call: an unbuffered stream is written out now. */
static int stream_done(struct qd_file *f)
{
	if (f->bufmode == _IONBF)
		return stream_flush(f);
	return 0;
}

/* Called by exit() and by the program start code when main returns, so that
 * text still in the stdout buffer reaches the console. */
void qd_flush_std(void)
{
	(void) stream_flush(&con_out);
	(void) stream_flush(&con_err);
}


/* ---- R11L: buffered file streams ----------------------------------------------------------------
 * Invariant while fb_mode is 0 (empty): the kernel's file position equals f->pos.  In read mode (1)
 * the kernel position is fb_off + fb_len and f->pos = fb_off + fb_cur.  In write mode (2) the
 * pending bytes start at fb_off, f->pos = fb_off + fb_len, and the kernel position is fb_off.  A
 * flush writes the pending bytes (write-through in the kernel) and leaves mode 0. */

int qd_fsname(const char *in, char *out, size_t outsz);     /* misc.c: folded WIN1_ name */

static int file_fail(struct qd_file *f, int kerr)
{
	f->err = 1;
	qd_set_errno(kerr);
	return EOF;
}

/* Write the pending bytes.  Returns 0, or EOF with err and errno set. */
static int file_flush_w(struct qd_file *f)
{
	int r;
	unsigned int n = f->fb_len;

	if (f->fb_mode != 2)
		return 0;
	f->fb_len = 0;
	f->fb_mode = 0;
	if (n == 0)
		return 0;
	r = qd_write_all(f->ch, f->fb, n);
	if (r != (int) n)
		return file_fail(f, r < 0 ? r : -85);
	f->fb_off += n;
	if (f->fflags & QD_O_APPEND) {
		long t = qd_tell(f->ch);
		if (t >= 0) {
			f->fb_off = (unsigned long) t;
			f->pos = f->fb_off;
		}
	}
	return 0;
}

/* Drop read data: the kernel position becomes f->pos again. */
static int file_drop_r(struct qd_file *f)
{
	if (f->fb_mode != 1)
		return 0;
	f->fb_mode = 0;
	if (f->fb_off + f->fb_len != f->pos) {
		if (qd_seek(f->ch, f->pos) < 0)
			return file_fail(f, -47);
	}
	f->fb_len = 0;
	f->fb_cur = 0;
	return 0;
}

/* Make the buffer a write buffer. */
static int file_begin_write(struct qd_file *f)
{
	if (f->fb_mode == 2)
		return 0;
	if (file_drop_r(f) == EOF)
		return EOF;
	if (f->fflags & QD_O_APPEND) {
		long sz = qd_fsize(f->ch);
		if (sz < 0)
			return file_fail(f, (int) sz);
		f->pos = (unsigned long) sz;
		if (qd_seek(f->ch, f->pos) < 0)
			return file_fail(f, -47);
	}
	f->fb_off = f->pos;
	f->fb_len = 0;
	f->fb_mode = 2;
	return 0;
}

static int file_putc(struct qd_file *f, int c)
{
	if (!(f->dir & DIR_WRITE)) {
		f->err = 1;
		qd_set_errno(-80);
		return EOF;
	}
	if (f->fb_mode != 2 && file_begin_write(f) == EOF)
		return EOF;
	f->fb[f->fb_len++] = (unsigned char) c;
	f->pos++;
	if (f->fb_len == FB_SIZE)
		return file_flush_w(f);
	return 0;
}

/* Write n bytes: through the buffer, large blocks straight to the kernel.  0, or EOF. */
static int file_write_bytes(struct qd_file *f, const unsigned char *p, unsigned long n)
{
	if (!(f->dir & DIR_WRITE)) {
		f->err = 1;
		qd_set_errno(-80);
		return EOF;
	}
	while (n > 0) {
		unsigned int room;
		unsigned int take;
		unsigned int i;

		if (f->fb_mode != 2 && file_begin_write(f) == EOF)
			return EOF;
		room = FB_SIZE - f->fb_len;

		if (f->fb_len == 0 && n >= FB_SIZE) {
			/* whole sectors straight to the kernel */
			unsigned long blk = n & ~(unsigned long) (FB_SIZE - 1u);
			int r = qd_write_all(f->ch, p, (unsigned int) blk);
			if (r != (int) blk) {
				f->fb_mode = 0;
				return file_fail(f, r < 0 ? r : -85);
			}
			f->fb_off += blk;
			f->pos += blk;
			p += blk;
			n -= blk;
			if (f->fflags & QD_O_APPEND) {
				long t = qd_tell(f->ch);
				if (t >= 0) {
					f->fb_off = (unsigned long) t;
					f->pos = f->fb_off;
				}
			}
			continue;
		}
		take = n < room ? (unsigned int) n : room;
		for (i = 0; i < take; i++)
			f->fb[f->fb_len + i] = p[i];
		f->fb_len += take;
		f->pos += take;
		p += take;
		n -= take;
		if (f->fb_len == FB_SIZE && file_flush_w(f) == EOF)
			return EOF;
	}
	return 0;
}

/* fflush on a file stream: pending bytes, then the directory entry. */
static int file_flush_all(struct qd_file *f)
{
	int r;
	if (file_flush_w(f) == EOF)
		return EOF;
	if (f->dir & DIR_WRITE) {
		r = qd_flush(f->ch);
		if (r < 0)
			return file_fail(f, r);
	}
	return 0;
}

/* Refill the read buffer.  Returns the bytes now available, 0 at end of file, EOF on error. */
static int file_fill(struct qd_file *f)
{
	unsigned int got = 0;

	if (f->fb_mode == 2 && file_flush_w(f) == EOF)
		return EOF;
	f->fb_mode = 0;
	f->fb_off = f->pos;
	while (got < FB_SIZE) {
		unsigned int chunk = FB_SIZE - got;
		int r;
		if (chunk > QD_READ_MAX)
			chunk = QD_READ_MAX;
		r = qd_read(f->ch, f->fb + got, chunk);
		if (r > 0) {
			got += (unsigned int) r;
			continue;
		}
		if (r < 0) {
			if (got == 0) {
				file_fail(f, r);
				return EOF;
			}
		}
		break;                       /* end of file, or data already in hand */
	}
	f->fb_len = got;
	f->fb_cur = 0;
	f->fb_mode = 1;
	if (got == 0)
		f->eof = 1;
	return (int) got;
}

/* ---- formatted output: one engine for printf, fprintf and sprintf ---------- */

struct qd_out {
	struct qd_file *f;   /* stream forms: characters go to f; NULL for string forms */
	char *dst;           /* destination for the string forms */
	unsigned long cap;   /* string forms: size of dst; writes stop at cap-1 */
	int total;           /* characters produced (the return value) */
	int failed;          /* a stream write failed */
};

static void out_char(struct qd_out *o, int c)
{
	if (o->f != NULL) {
		if (stream_putc(o->f, c) == EOF)
			o->failed = 1;
		o->total++;
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

/* ---- floating-point conversions (R11C) -------------------------------------
 * %f, %e, %E, %g and %G of a double.  The digits are the exact decimal value
 * of the double (fpbig.c); rounding is to nearest, ties to even, on those
 * exact digits, as glibc does in the C locale.  The precision is limited to
 * FMT_MAXPREC digits: a larger request is cut to that (documented). */
#define FMT_MAXPREC 500
#define FMT_MAXDIG  800

/* Round dig (ndig digits, decimal point after dp) to keep digits.  Returns
 * the new decimal point position; *ndig becomes the digit count (0 when the
 * value rounds to zero). */
static int fp_round(char *dig, int *ndig, int dp, int keep)
{
	int i, up;
	char c;
	int rest = 0;

	if (keep >= *ndig)
		return dp;
	if (keep < 0) {
		*ndig = 0;
		return dp;
	}
	c = dig[keep];
	for (i = keep + 1; i < *ndig; i++)
		if (dig[i] != '0') {
			rest = 1;
			break;
		}
	if (c > '5')
		up = 1;
	else if (c < '5')
		up = 0;
	else if (rest)
		up = 1;
	else
		up = keep > 0 ? (dig[keep - 1] - '0') & 1 : 0;   /* ties to even */
	*ndig = keep;
	if (up) {
		i = keep - 1;
		while (i >= 0 && dig[i] == '9')
			i--;
		if (i < 0) {
			dig[0] = '1';
			*ndig = 1;
			dp++;
		} else {
			dig[i]++;
			*ndig = i + 1;
		}
	}
	return dp;
}

/* Digit at position idx of a digit string, '0' outside it. */
static char fp_at(const char *dig, int ndig, int idx)
{
	return (idx >= 0 && idx < ndig) ? dig[idx] : '0';
}

/* Write the body of %f (no sign) with prec fraction digits. */
static int fp_fixed(char *out, const char *dig0, int ndig0, int dp0, int prec, int alt)
{
	char wd[FMT_MAXDIG];
	int nd = ndig0, dp = dp0, n = 0, i;

	memcpy(wd, dig0, ndig0 > 0 ? ndig0 : 0);
	if (nd > 0)
		dp = fp_round(wd, &nd, dp, dp + prec);
	if (dp <= 0)
		out[n++] = '0';
	for (i = 0; i < dp; i++)
		out[n++] = fp_at(wd, nd, i);
	if (prec > 0 || alt)
		out[n++] = '.';
	for (i = 0; i < prec; i++)
		out[n++] = fp_at(wd, nd, dp + i);
	return n;
}

/* The mantissa of %e (no sign, no exponent) with prec fraction digits.  The
 * decimal exponent of the rounded value is returned in *xexp. */
static int fp_mant(char *out, const char *dig0, int ndig0, int dp0, int prec, int alt, int *xexp)
{
	char wd[FMT_MAXDIG];
	int nd = ndig0, dp = 1, n = 0, i, x;

	if (nd == 0) {
		x = 0;
	} else {
		memcpy(wd, dig0, nd);
		dp = fp_round(wd, &nd, dp0, prec + 1);
		x = dp - 1;
	}
	out[n++] = fp_at(wd, nd, 0);
	if (prec > 0 || alt)
		out[n++] = '.';
	for (i = 1; i <= prec; i++)
		out[n++] = fp_at(wd, nd, i);
	*xexp = x;
	return n;
}

/* Append the exponent part "e+05" (upper selects 'E'); returns its length. */
static int fp_expsuffix(char *out, int x, int upper)
{
	int n = 0;

	out[n++] = upper ? 'E' : 'e';
	out[n++] = x < 0 ? '-' : '+';
	if (x < 0)
		x = -x;
	if (x >= 100)
		out[n++] = (char) ('0' + x / 100);
	out[n++] = (char) ('0' + (x / 10) % 10);
	out[n++] = (char) ('0' + x % 10);
	return n;
}

/* Remove the trailing zeros of the fraction in body[0..n), and the point
 * when nothing is left after it.  Returns the new length. */
static int fp_strip(char *body, int n)
{
	int point = -1, i, m;

	for (i = 0; i < n; i++)
		if (body[i] == '.') {
			point = i;
			break;
		}
	if (point < 0)
		return n;
	m = n;
	while (m > point + 1 && body[m - 1] == '0')
		m--;
	if (m == point + 1)
		m = point;
	return m;
}

/* The body (no sign) of a conversion of the magnitude of v. */
static int fp_body(char *body, double v, char conv, int prec, int alt)
{
	union {
		double d;
		unsigned long w[2];
	} u;
	char dig[FMT_MAXDIG];
	int ndig = 0, dp = 0, n, x, p;

	u.d = v;
	u.w[1] &= 0x7fffffffUL;               /* magnitude only */
	if (u.d != 0.0)
		qd_fp_exact(u.d, dig, &ndig, &dp);

	if (conv == 'f')
		return fp_fixed(body, dig, ndig, dp, prec, alt);
	if (conv == 'e' || conv == 'E') {
		n = fp_mant(body, dig, ndig, dp, prec, alt, &x);
		return n + fp_expsuffix(body + n, x, conv == 'E');
	}

	/* %g and %G: P significant digits; %f style when -4 <= X < P, else %e */
	p = prec == 0 ? 1 : prec;
	if (ndig == 0) {
		x = 0;
	} else {
		char tmp[FMT_MAXDIG];
		int nd = ndig, dpr;
		memcpy(tmp, dig, ndig);
		dpr = fp_round(tmp, &nd, dp, p);
		x = dpr - 1;
	}
	if (x < p && x >= -4) {
		n = fp_fixed(body, dig, ndig, dp, p - 1 - x, alt);
		if (!alt)
			n = fp_strip(body, n);
		return n;
	}
	n = fp_mant(body, dig, ndig, dp, p - 1, alt, &x);
	if (!alt)
		n = fp_strip(body, n);
	return n + fp_expsuffix(body + n, x, conv == 'G');
}

static void out_fp(struct qd_out *o, double v, char conv, int width, int left, int zero,
                   int plus, int space, int prec, int alt)
{
	char body[FMT_MAXDIG + 16];
	union {
		double d;
		unsigned long w[2];
	} u;
	int n;
	const char *sign = NULL;

	u.d = v;
	if (u.w[1] & 0x80000000UL)
		sign = "-";
	else if (plus)
		sign = "+";
	else if (space)
		sign = " ";
	n = fp_body(body, v, conv, prec, alt);
	out_field(o, body, n, width, left, zero && !left, sign);
}

/* Integer precision (C89 7.9.6.1): pad the digits with leading zeros to prec;
 * a zero value with precision 0 produces no digits.  Returns the new length. */
static int int_prec(char *buf, const char *digits, int len, int prec, int is_zero)
{
	int i, zeros;
	if (prec < 0) {                 /* no precision: digits unchanged */
		for (i = 0; i < len; i++)
			buf[i] = digits[i];
		return len;
	}
	if (prec == 0 && is_zero)
		return 0;
	zeros = prec > len ? prec - len : 0;
	for (i = 0; i < zeros; i++)
		buf[i] = '0';
	for (i = 0; i < len; i++)
		buf[zeros + i] = digits[i];
	return zeros + len;
}

static int qd_vformat(struct qd_out *o, const char *fmt, va_list ap)
{
	for (; *fmt != '\0'; fmt++) {
		int left = 0, zero = 0, plus = 0, space = 0, alt = 0;
		int width = 0, prec = -1;
		char digits[16];
		char pbuf[FMT_MAXPREC + 16];   /* digits padded to the precision (C89) */
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
			else if (*fmt == '+')
				plus = 1;
			else if (*fmt == ' ')
				space = 1;
			else if (*fmt == '#')
				alt = 1;
			else
				break;
		}
		while (*fmt >= '0' && *fmt <= '9')
			width = width * 10 + (*fmt++ - '0');
		if (*fmt == '.') {              /* precision (R11C) */
			fmt++;
			prec = 0;
			while (*fmt >= '0' && *fmt <= '9') {
				if (prec < FMT_MAXPREC)
					prec = prec * 10 + (*fmt - '0');
				fmt++;
			}
			if (prec > FMT_MAXPREC)
				prec = FMT_MAXPREC;
		}
		if (*fmt == 'l' || *fmt == 'h' || *fmt == 'L')    /* int and long are both 32 bits */
			fmt++;
		switch (*fmt) {
		case 'd':
		case 'i': {
			long v = va_arg(ap, long);
			unsigned long m = v < 0 ? 0UL - (unsigned long) v : (unsigned long) v;
			len = to_base(m, 10, 0, digits);
			len = int_prec(pbuf, digits, len, prec, m == 0);
			out_field(o, pbuf, len, width, left, zero && !left && prec < 0,
			          v < 0 ? "-" : plus ? "+" : space ? " " : NULL);
			break;
		}
		case 'u':
		case 'x':
		case 'X':
		case 'o': {
			unsigned long v = va_arg(ap, unsigned long);
			unsigned int base = *fmt == 'u' ? 10 : *fmt == 'o' ? 8 : 16;
			len = to_base(v, base, *fmt == 'X', digits);
			len = int_prec(pbuf, digits, len, prec, v == 0);
			out_field(o, pbuf, len, width, left, zero && !left && prec < 0, NULL);
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
			if (prec >= 0 && len > prec)
				len = prec;
			out_field(o, s, len, width, left, 0, NULL);
			break;
		}
		case 'f':
		case 'e':
		case 'E':
		case 'g':
		case 'G': {
			double v = va_arg(ap, double);
			out_fp(o, v, *fmt, width, left, zero, plus, space, prec < 0 ? 6 : prec, alt);
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

int vfprintf(FILE *fp, const char *fmt, va_list ap)
{
	struct qd_out o;
	struct qd_file *f = (struct qd_file *) fp;
	int n;

	if (f == NULL)
		return -1;
	o.f = f;
	o.dst = NULL;
	o.cap = 0;
	o.total = 0;
	o.failed = 0;
	n = qd_vformat(&o, fmt, ap);
	if (stream_done(f) == EOF)
		o.failed = 1;
	return o.failed ? -1 : n;
}

int fprintf(FILE *fp, const char *fmt, ...)
{
	va_list ap;
	int n;
	va_start(ap, fmt);
	n = vfprintf(fp, fmt, ap);
	va_end(ap);
	return n;
}

int vprintf(const char *fmt, va_list ap)
{
	return vfprintf(stdout, fmt, ap);
}

int printf(const char *fmt, ...)
{
	va_list ap;
	int n;
	va_start(ap, fmt);
	n = vfprintf(stdout, fmt, ap);
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
	o.f = NULL;
	o.dst = buf;
	o.cap = cap;
	o.total = 0;
	o.failed = 0;
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

int vsprintf(char *buf, const char *fmt, va_list ap)
{
	return qd_format_to(buf, (unsigned long) -1, fmt, ap);
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

/* ---- character and string output ----------------------------------------- */

int fputc(int c, FILE *fp)
{
	struct qd_file *f = (struct qd_file *) fp;
	if (f == NULL)
		return EOF;
	if (stream_putc(f, c) == EOF)
		return EOF;
	if (stream_done(f) == EOF)
		return EOF;
	return (unsigned char) c;
}

int putc(int c, FILE *fp)
{
	return fputc(c, fp);
}

int putchar(int c)
{
	return fputc(c, stdout);
}

int fputs(const char *s, FILE *fp)
{
	struct qd_file *f = (struct qd_file *) fp;
	if (f == NULL || s == NULL)
		return EOF;
	while (*s != '\0')
		if (stream_putc(f, (unsigned char) *s++) == EOF)
			return EOF;
	return stream_done(f) == EOF ? EOF : 0;
}

int puts(const char *s)
{
	if (fputs(s, stdout) == EOF)
		return EOF;
	return fputc('\n', stdout) == EOF ? EOF : 0;
}

size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *fp)
{
	struct qd_file *f = (struct qd_file *) fp;
	const unsigned char *p = (const unsigned char *) ptr;
	unsigned long want, i;

	if (f == NULL || ptr == NULL || size == 0 || nmemb == 0)
		return 0;
	if (nmemb > (size_t) -1 / size) {
		f->err = 1;
		return 0;
	}
	want = (unsigned long) (size * nmemb);
	if (f->kind == KIND_FILE) {
		if (file_write_bytes(f, p, want) != 0)
			return 0;
		return nmemb;
	}
	for (i = 0; i < want; i++)
		if (stream_putc(f, p[i]) == EOF)
			return 0;                /* wrong direction or failed write */
	/* Conservative: if the final flush fails, no item is reported as written. */
	if (stream_done(f) == EOF)
		return 0;
	return nmemb;
}

int fflush(FILE *fp)
{
	struct qd_file *f;
	int r1, r2;

	if (fp != NULL) {
		f = (struct qd_file *) fp;
		if (f->kind == KIND_FILE)
			return file_flush_all(f);
		/* Input streams have no buffer to flush: no effect, returns 0. */
		if (f->dir != DIR_WRITE)
			return 0;
		return stream_flush(f);
	}
	r1 = stream_flush(&con_out);
	r2 = stream_flush(&con_err);
	return (r1 == EOF || r2 == EOF) ? EOF : 0;
}

/* Buffer modes can be chosen; caller-supplied buffers cannot (the library
 * owns the stdout buffer), so a non-NULL buf fails.  Input streams are not
 * buffered by the library: the mode is accepted and has no effect. */
int setvbuf(FILE *fp, char *buf, int mode, size_t size)
{
	struct qd_file *f = (struct qd_file *) fp;

	(void) size;
	if (f == NULL || buf != NULL)
		return -1;
	if (mode != _IOFBF && mode != _IOLBF && mode != _IONBF)
		return -1;
	if (!(f->dir & DIR_WRITE))
		return 0;
	if (stream_flush(f) == EOF)
		return -1;
	f->bufmode = mode;
	return 0;
}

void setbuf(FILE *fp, char *buf)
{
	if (buf == NULL)
		(void) setvbuf(fp, NULL, _IONBF, BUFSIZ);
	/* a caller buffer is not supported: the call has no effect */
}

/* ---- input ----------------------------------------------------------------- */

static int file_fill(struct qd_file *f);

int fgetc(FILE *fp)
{
	struct qd_file *f = (struct qd_file *) fp;
	unsigned char c;
	int got;

	if (f == NULL)
		return EOF;
	if (!(f->dir & DIR_READ)) {
		f->err = 1;
		return EOF;
	}
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
		(void) stream_flush(&con_out);   /* a prompt must be seen before we wait */
#ifdef QDOS_C68_CONWRITE
		/* R14A C68 variant: wait in the kernel's console wait (no polling); ENTER (CR) is returned as LF */
		c68_arm_end();
		{
			int k = qd_con_getkey(ch);
			if (k < 0) {
				f->eof = 1;
				return EOF;
			}
			return k == '\r' ? '\n' : k;
		}
#endif
		for (;;) {
			got = qd_read(ch, &c, 1);
			if (got == 1)
				return c == '\r' ? '\n' : c;   /* the Enter key sends CR */
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
	if (f->fb_mode != 1 || f->fb_cur >= f->fb_len) {
		if (file_fill(f) <= 0)
			return EOF;
	}
	c = f->fb[f->fb_cur++];
	f->pos++;
	return c;
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
	if (c == EOF || f == NULL || !(f->dir & DIR_READ) || f->has_unget)
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

/* Open NAME in MODE and set up stream F (already allocated; its static parts are kept).  Returns 0,
 * or -1 with errno set and F untouched. */
static int stream_open_into(struct qd_file *f, const char *name, const char *mode)
{
	char full[FILENAME_MAX];
	int ch;
	unsigned int flags = 0, dirbits = 0;
	int plus = 0, i;
	unsigned char *fb;

	if (name == NULL || mode == NULL) {
		errno = EINVAL;
		return -1;
	}
	if (mode[0] != 'r' && mode[0] != 'w' && mode[0] != 'a') {
		errno = EINVAL;
		return -1;
	}
	for (i = 1; mode[i] != '\0'; i++) {
		if (mode[i] == '+')
			plus = 1;
		else if (mode[i] != 'b') {
			errno = EINVAL;
			return -1;
		}
	}
	if (mode[0] == 'r')
		flags = plus ? (QD_O_READ | QD_O_WRITE) : QD_O_READ;
	else if (mode[0] == 'w')
		flags = QD_O_WRITE | QD_O_CREATE | QD_O_TRUNC | (plus ? QD_O_READ : 0);
	else
		flags = QD_O_WRITE | QD_O_CREATE | QD_O_APPEND | (plus ? QD_O_READ : 0);
	if (flags & QD_O_READ)
		dirbits |= DIR_READ;
	if (flags & QD_O_WRITE)
		dirbits |= DIR_WRITE;

	if (qd_fsname(name, full, sizeof full) != 0) {
		errno = ENOENT;
		return -1;
	}
	if (flags == QD_O_READ && strncmp(full, "WIN1:", 5) != 0)
		ch = qd_open(full);          /* the legacy flat form works on every kernel */
	else
		ch = qd_open_ex(full, flags | (strncmp(full, "WIN1:", 5) == 0 ? QD_O_FILE : 0));
	if (ch <= 0) {
		qd_set_errno(ch);
		return -1;
	}
	fb = (unsigned char *) malloc(FB_SIZE);
	if (fb == NULL) {
		qd_close(ch);
		errno = ENOMEM;
		return -1;
	}
	f->fb = fb;
	f->ch = ch;
	f->kind = KIND_FILE;
	f->dir = (int) dirbits;
	f->eof = 0;
	f->err = 0;
	f->pos = 0;
	f->has_unget = 0;
	f->unget_c = 0;
	f->bufmode = _IOFBF;
	f->wlen = 0;
	f->fb_mode = 0;
	f->fb_len = 0;
	f->fb_cur = 0;
	f->fb_off = 0;
	f->fflags = flags;
	return 0;
}

FILE *fopen(const char *name, const char *mode)
{
	struct qd_file *f = (struct qd_file *) malloc(sizeof(struct qd_file));

	if (f == NULL) {
		errno = ENOMEM;
		return NULL;
	}
	if (stream_open_into(f, name, mode) != 0) {
		free(f);
		return NULL;
	}
	return (FILE *) f;
}

/* Close the file part of a stream (flush, descriptor, channel, buffer).  Returns 0 or EOF. */
static int stream_close_file(struct qd_file *f)
{
	int r, bad = 0;

	if (f->kind != KIND_FILE || f->ch <= 0)
		return 0;
	if (file_flush_w(f) == EOF)
		bad = 1;
	qd_fd_forget(f->ch);
	r = qd_close(f->ch);             /* writes the directory entry (R11L kernels) */
	if (r < 0 && !bad) {
		qd_set_errno(r);
		bad = 1;
	}
	if (f->fb != NULL)
		free(f->fb);
	f->fb = NULL;
	f->ch = 0;
	return bad ? EOF : 0;
}

/* freopen: close the stream's file (the console streams are only flushed), then open NAME into the
 * same FILE object, so stdout can be redirected to a file.  On failure NULL is returned: a
 * console stream stays on the console, any other stream is left closed (every operation fails). */
FILE *freopen(const char *name, const char *mode, FILE *fp)
{
	struct qd_file *f = (struct qd_file *) fp;
	int std = (f == &con_in || f == &con_out || f == &con_err);
	struct qd_file save;

	if (f == NULL)
		return NULL;
	if (f->kind == KIND_FILE)
		(void) stream_close_file(f);
	else
		(void) stream_flush(f);
	save = *f;
	if (stream_open_into(f, name, mode) != 0) {
		if (std) {
			*f = save;               /* stays a console stream */
		} else {
			f->kind = KIND_FILE;
			f->ch = 0;
			f->dir = 0;
			f->err = 1;
		}
		return NULL;
	}
	return fp;
}

/* Exit: a standard stream that was redirected to a file is closed so that the directory entry is
 * written (qd_flush_std has already written the data). */
void qd_close_std(void)
{
	if (con_out.kind == KIND_FILE)
		(void) stream_close_file(&con_out);
	if (con_err.kind == KIND_FILE)
		(void) stream_close_file(&con_err);
}

/* R11L: POSIX descriptor view of a stream (see posixio.c).  A file stream gets a descriptor on its
 * channel on first request; mixing fd-level and stream-level I/O on one file is not supported. */
int fileno(FILE *fp)
{
	struct qd_file *f = (struct qd_file *) fp;

	if (f == NULL) {
		errno = EBADF;
		return -1;
	}
	if (f->kind == KIND_FILE && f->ch > 0) {
		/* a file stream, or a standard stream redirected to a file by freopen */
		(void) file_flush_w(f);
		return qd_fd_for_channel(f->ch);
	}
	if (f == &con_in)
		return 0;
	if (f == &con_out)
		return 1;
	if (f == &con_err)
		return 2;
	errno = EBADF;
	return -1;
}

/* fdopen: a stream on the channel of an open descriptor.  The stream takes over the channel:
 * fclose closes it and releases the descriptor. */
FILE *fdopen(int fd, const char *mode)
{
	struct qd_file *f;
	int fl = 0, ch;
	unsigned int dirbits = 0, flags = 0;
	long pos;

	if (mode == NULL || (mode[0] != 'r' && mode[0] != 'w' && mode[0] != 'a')) {
		errno = EINVAL;
		return NULL;
	}
	if (fd >= 0 && fd <= 2) {
		FILE *s = fd == 0 ? stdin : (fd == 1 ? stdout : stderr);
		return s;
	}
	ch = qd_fd_take(fd, &fl);
	if (ch <= 0)
		return NULL;
	if (mode[0] == 'r') {
		flags = QD_O_READ;
		dirbits = DIR_READ;
	} else {
		flags = QD_O_WRITE;
		dirbits = DIR_WRITE;
	}
	if (strchr(mode, '+') != NULL) {
		flags = QD_O_READ | QD_O_WRITE;
		dirbits = DIR_READ | DIR_WRITE;
	}
	f = (struct qd_file *) malloc(sizeof(struct qd_file));
	if (f == NULL) {
		errno = ENOMEM;
		return NULL;
	}
	f->fb = (unsigned char *) malloc(FB_SIZE);
	if (f->fb == NULL) {
		free(f);
		errno = ENOMEM;
		return NULL;
	}
	pos = qd_tell(ch);
	f->ch = ch;
	f->kind = KIND_FILE;
	f->dir = (int) dirbits;
	f->eof = 0;
	f->err = 0;
	f->pos = pos < 0 ? 0ul : (unsigned long) pos;
	f->has_unget = 0;
	f->unget_c = 0;
	f->bufmode = _IOFBF;
	f->wlen = 0;
	f->fb_mode = 0;
	f->fb_len = 0;
	f->fb_cur = 0;
	f->fb_off = f->pos;
	f->fflags = flags;
	return (FILE *) f;
}

int fclose(FILE *fp)
{
	struct qd_file *f = (struct qd_file *) fp;
	int std, r;

	if (f == NULL)
		return EOF;
	std = (f == &con_in || f == &con_out || f == &con_err);
	if (std && f->kind != KIND_FILE) {
		/* the console streams stay open; pending output is written */
		(void) stream_flush(f);
		return 0;
	}
	r = stream_close_file(f);
	if (std) {
		f->kind = KIND_CON;         /* a redirected standard stream falls back to the console */
		f->ch = 0;
		f->has_unget = 0;
		f->wlen = 0;
		return r;
	}
	free(f);
	return r;
}

size_t fread(void *ptr, size_t size, size_t nmemb, FILE *fp)
{
	struct qd_file *f = (struct qd_file *) fp;
	unsigned long want, total = 0;
	char *p = (char *) ptr;

	if (f == NULL || f->kind != KIND_FILE || !(f->dir & DIR_READ) || size == 0 || nmemb == 0)
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
		unsigned long left = want - total;
		if (f->fb_mode == 1 && f->fb_cur < f->fb_len) {
			unsigned int avail = f->fb_len - f->fb_cur;
			unsigned int take = left < avail ? (unsigned int) left : avail;
			unsigned int i;
			for (i = 0; i < take; i++)
				p[total + i] = (char) f->fb[f->fb_cur + i];
			f->fb_cur += take;
			f->pos += take;
			total += take;
			continue;
		}
		if (f->fb_mode == 2 && file_flush_w(f) == EOF)
			break;
		if (left >= FB_SIZE && (f->fb_mode == 0 || f->fb_mode == 1)) {
			/* buffer drained: read whole blocks straight into the caller's memory */
			unsigned long blk = left & ~(unsigned long) (FB_SIZE - 1u);
			unsigned long got = 0;
			f->fb_mode = 0;
			f->fb_len = f->fb_cur = 0;
			while (got < blk) {
				unsigned int chunk = (unsigned int) (blk - got);
				int r;
				if (chunk > QD_READ_MAX)
					chunk = QD_READ_MAX;
				r = qd_read(f->ch, p + total + got, chunk);
				if (r > 0) {
					got += (unsigned long) r;
					continue;
				}
				if (r == 0)
					f->eof = 1;
				else
					file_fail(f, r);
				break;
			}
			total += got;
			f->pos += got;
			if (got < blk)
				break;
			continue;
		}
		{
			int n = file_fill(f);
			if (n <= 0)
				break;
		}
	}
	return (size_t) (total / size);
}

int fseek(FILE *fp, long off, int whence)
{
	struct qd_file *f = (struct qd_file *) fp;
	long target;

	if (f == NULL || f->kind != KIND_FILE)
		return -1;
	if (file_flush_w(f) == EOF)
		return -1;
	if (whence == SEEK_SET)
		target = off;
	else if (whence == SEEK_CUR)
		target = (long) f->pos + off;
	else if (whence == SEEK_END) {
		long sz = qd_fsize(f->ch);
		if (sz < 0) {
			qd_set_errno((int) sz);
			return -1;
		}
		target = sz + off;
	} else {
		errno = EINVAL;
		return -1;
	}
	if (target < 0) {
		errno = EINVAL;
		return -1;
	}
	f->fb_mode = 0;                  /* the read buffer is dropped; the kernel seek below sets the position */
	f->fb_len = f->fb_cur = 0;
	if (qd_seek(f->ch, (unsigned long) target) < 0) {
		/* past the end: a writable file is extended with zeros (C semantics of a gap) */
		if ((f->dir & DIR_WRITE) && qd_truncate(f->ch, (unsigned long) target) == 0 &&
		    qd_seek(f->ch, (unsigned long) target) == 0)
			;
		else {
			(void) qd_seek(f->ch, f->pos);
			errno = EINVAL;
			return -1;
		}
	}
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

void rewind(FILE *fp)
{
	if (fp != NULL) {
		(void) fseek(fp, 0L, SEEK_SET);
		clearerr(fp);            /* C89: rewind also clears the error flag */
	}
}

void clearerr(FILE *fp)
{
	if (fp != NULL) {
		((struct qd_file *) fp)->eof = 0;
		((struct qd_file *) fp)->err = 0;
	}
}
