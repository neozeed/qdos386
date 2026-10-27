/* time.c - QDOS/386 libc (R11F): calendar time.
 *
 * Policy (docs/R11F-TIME-ABI.md): time_t is an unsigned 32-bit count of seconds
 * since 1970-01-01 00:00:00 UTC.  Only UTC is supported: localtime is gmtime,
 * tm_isdst is always 0, and there are no timezone rules.  The calendar is
 * proleptic Gregorian.  Pre-epoch and post-2106 dates are not representable
 * and are reported as errors (mktime returns (time_t) -1, gmtime and localtime
 * return NULL for a NULL argument only).
 *
 * The calendar arithmetic needs no RTC.  time() reads the kernel wall clock
 * (QT_WALL_TIME) and reports the clock as unavailable when none is configured.
 * clock() is explicitly unavailable: process CPU time is not implemented.
 */
#include <time.h>
#include <stdio.h>
#include "qdos386.h"

#define QD_LMAX   2147483647L
#define QD_LMIN   (-2147483647L - 1L)
#define QD_TMAX   0xFFFFFFFEUL              /* largest valid time_t */
#define QD_DAYMAX 49710L                    /* days in 0 .. QD_TMAX */
#define QD_YEARLIM 1000000L                 /* normalized years beyond this: rejected */

static const char *qd_wday_name[] = {
	"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"
};
static const char *qd_mon_name[] = {
	"January", "February", "March", "April", "May", "June", "July",
	"August", "September", "October", "November", "December"
};
static const char *qd_wday_abbr[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
static const char *qd_mon_abbr[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                     "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

/* Days from 1970-01-01 to y-m-d (m 1..12, d any integer), proleptic Gregorian.
 * Uses the days-from-civil method of Howard Hinnant ("chrono-Compatible Low-Level
 * Date Algorithms"), written out for this library.
 * Valid for |y| up to QD_YEARLIM: all intermediates stay within 32 bits. */
static long qd_days_from_civil(long y, long m, long d)
{
	long era, yoe, doy, doe;

	if (m <= 2)
		y = y - 1;
	era = (y >= 0 ? y : y - 399) / 400;
	yoe = y - era * 400;
	doy = (153 * (m > 2 ? m - 3 : m + 9) + 2) / 5 + d - 1;
	doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
	return era * 146097 + doe - 719468;
}

/* Fill tm from a valid time (0 .. QD_TMAX, or any value of time_t). */
static void qd_fill(unsigned long t, struct tm *tm)
{
	long days, era, doe, yoe, doy, mp, y, m, d, sod;

	days = (long) (t / 86400UL);
	sod = (long) (t % 86400UL);
	{
		long z = days + 719468;
		era = (z >= 0 ? z : z - 146096) / 146097;
		doe = z - era * 146097;
		yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
		y = yoe + era * 400;
		doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
		mp = (5 * doy + 2) / 153;
		d = doy - (153 * mp + 2) / 5 + 1;
		m = (mp < 10) ? mp + 3 : mp - 9;
		if (m <= 2)
			y = y + 1;
	}
	tm->tm_sec = (int) (sod % 60);
	tm->tm_min = (int) ((sod / 60) % 60);
	tm->tm_hour = (int) (sod / 3600);
	tm->tm_mday = (int) d;
	tm->tm_mon = (int) (m - 1);
	tm->tm_year = (int) (y - 1900);
	tm->tm_wday = (int) ((4 + days) % 7);          /* 1970-01-01 was a Thursday */
	tm->tm_yday = (int) (days - qd_days_from_civil(y, 1, 1));
	tm->tm_isdst = 0;
}

/* Checked addition on long: returns 0 (and leaves *r alone) on overflow. */
static int qd_add(long a, long b, long *r)
{
	if ((b > 0 && a > QD_LMAX - b) || (b < 0 && a < QD_LMIN - b))
		return 0;
	*r = a + b;
	return 1;
}

/* Floor division and modulus by a positive b. */
static long qd_fdiv(long a, long b)
{
	long q = a / b;
	if (a % b != 0 && a < 0)
		q = q - 1;
	return q;
}

static long qd_fmod(long a, long b)
{
	return a - qd_fdiv(a, b) * b;
}

static struct tm qd_tm;                 /* gmtime / localtime result (shared) */
static char qd_asc[26];                 /* asctime result (shared) */
static char qd_ctime[26];               /* ctime result (shared) */

double difftime(time_t t1, time_t t0)
{
	return (double) t1 - (double) t0;
}

/* time(): the kernel wall clock.  Unavailable (no clock configured, or the
 * clock has overflowed) reports (time_t) -1, and *t is set to it too. */
time_t time(time_t *t)
{
	unsigned long v = qd_wall_time();
	time_t r;

	if (v == 0xFFFFFFFFUL || v > QD_TMAX)
		r = (time_t) -1;
	else
		r = (time_t) v;
	if (t != NULL)
		*t = r;
	return r;
}

/* clock(): process CPU time is not implemented, so it is unavailable.  It is
 * deliberately not the wall clock or the tick count (docs/R11F-TIME-ABI.md). */
clock_t clock(void)
{
	return (clock_t) -1;
}

/* mktime: normalize the fields (month, day, hour, minute and second overflow,
 * day 0, negative values) and convert to time_t.  Fails with (time_t) -1 if
 * the result is before the epoch or after QD_TMAX, or if an intermediate value
 * would overflow.  On success the struct is normalized in place, and wday,
 * yday and isdst are set.  On failure the struct is left unchanged. */
time_t mktime(struct tm *tm)
{
	long sec, min, hour, mday, mon, year, c, dayc, days, dfc;
	unsigned long ut, sod;
	struct tm out;

	if (tm == NULL)
		return (time_t) -1;

	sec = tm->tm_sec;
	min = tm->tm_min;
	hour = tm->tm_hour;
	mday = tm->tm_mday;
	mon = tm->tm_mon;
	year = (long) tm->tm_year + 1900L;

	/* seconds -> minutes */
	c = qd_fdiv(sec, 60);
	sec = qd_fmod(sec, 60);
	if (!qd_add(min, c, &min))
		return (time_t) -1;
	/* minutes -> hours */
	c = qd_fdiv(min, 60);
	min = qd_fmod(min, 60);
	if (!qd_add(hour, c, &hour))
		return (time_t) -1;
	/* hours -> days */
	dayc = qd_fdiv(hour, 24);
	hour = qd_fmod(hour, 24);
	/* months -> years */
	c = qd_fdiv(mon, 12);
	mon = qd_fmod(mon, 12);
	if (!qd_add(year, c, &year))
		return (time_t) -1;
	if (year > QD_YEARLIM || year < -QD_YEARLIM)
		return (time_t) -1;

	/* day of the first of the month, plus (mday - 1) and the carried days */
	dfc = qd_days_from_civil(year, mon + 1, 1);
	if (!qd_add(dfc, mday, &days))
		return (time_t) -1;
	if (!qd_add(days, -1L, &days))
		return (time_t) -1;
	if (!qd_add(days, dayc, &days))
		return (time_t) -1;
	if (days < 0 || days > QD_DAYMAX)
		return (time_t) -1;

	/* days * 86400 is at most 4294944000, which fits unsigned long; the seconds
	 * of the day are added with a check before the sum can wrap. */
	ut = (unsigned long) days * 86400UL;
	sod = (unsigned long) (hour * 3600 + min * 60 + sec);
	if (sod > QD_TMAX - ut)
		return (time_t) -1;
	ut = ut + sod;

	qd_fill(ut, &out);
	*tm = out;
	return (time_t) ut;
}

/* gmtime: the UTC breakdown of t.  NULL, or t above QD_TMAX (0xFFFFFFFE, the
 * last supported time; 0xFFFFFFFF is the unavailable sentinel), gives NULL.
 * The result is a shared static struct. */
struct tm *gmtime(const time_t *t)
{
	if (t == NULL || *t > QD_TMAX)     /* unrepresentable: includes the -1 sentinel */
		return NULL;
	qd_fill((unsigned long) *t, &qd_tm);
	return &qd_tm;
}

/* localtime: UTC only (the policy above), so localtime is gmtime. */
struct tm *localtime(const time_t *t)
{
	return gmtime(t);
}

/* asctime: "Www Mmm dd hh:mm:ss yyyy\n" (C89 4.12.3.1).  The result is always
 * NUL-terminated and fits qd_asc: longer output (a year with more digits than
 * four, or a negative field) is truncated, never overflowed.  Out-of-range
 * wday or mon print as "???".  Numeric fields print as given, not normalized. */
char *asctime(const struct tm *tm)
{
	const char *wd;
	const char *mo;

	if (tm == NULL)
		return NULL;
	wd = (tm->tm_wday >= 0 && tm->tm_wday <= 6) ? qd_wday_abbr[tm->tm_wday] : "???";
	mo = (tm->tm_mon >= 0 && tm->tm_mon <= 11) ? qd_mon_abbr[tm->tm_mon] : "???";
	snprintf(qd_asc, sizeof qd_asc, "%.3s %.3s%3d %.2d:%.2d:%.2d %d\n", wd, mo, tm->tm_mday,
	        tm->tm_hour, tm->tm_min, tm->tm_sec, tm->tm_year + 1900);
	return qd_asc;
}

/* ctime: asctime of localtime, copied into its own static buffer. */
char *ctime(const time_t *t)
{
	struct tm *tm = localtime(t);
	const char *src;
	int i;

	if (tm == NULL)
		return NULL;
	src = asctime(tm);
	for (i = 0; i < 26; i++) {
		qd_ctime[i] = src[i];
		if (src[i] == '\0')
			break;
	}
	qd_ctime[25] = '\0';
	return qd_ctime;
}

/* ---- strftime ------------------------------------------------------------
 * C89 conversions, C locale.  The result is written only if it fits in max
 * bytes including the terminating NUL; otherwise the function returns 0 and
 * s[0] is set to NUL (when max > 0).  An empty result (fmt is "") returns 0.
 * An unknown conversion, or a name index out of range, also returns 0.
 * %Z is "UTC" (the timezone policy above). */

struct qd_out {
	char *s;
	size_t max;
	size_t n;
	int ok;
};

static void qd_put(struct qd_out *o, int c)
{
	if (!o->ok)
		return;
	if (o->n + 1 >= o->max) {        /* room for this byte and the NUL */
		o->ok = 0;
		return;
	}
	o->s[o->n] = (char) c;
	o->n = o->n + 1;
}

static void qd_puts_out(struct qd_out *o, const char *p)
{
	while (*p != '\0' && o->ok)
		qd_put(o, (unsigned char) *p++);
}

/* Decimal with a minimum width; pad is '0' or ' '.  Negative values print a
 * leading minus, counted in the width. */
static void qd_num(struct qd_out *o, long v, int width, int pad)
{
	char buf[16];
	int n = 0, neg = 0, i;
	unsigned long u;

	if (v < 0) {
		neg = 1;
		u = (unsigned long) (-(v + 1)) + 1UL;
	} else {
		u = (unsigned long) v;
	}
	do {
		buf[n++] = (char) ('0' + (int) (u % 10UL));
		u = u / 10UL;
	} while (u != 0 && n < 15);
	if (neg)
		buf[n++] = '-';
	for (i = n; i < width; i++)
		qd_put(o, pad);
	while (n > 0)
		qd_put(o, buf[--n]);
}

static int qd_name_ok(int idx, int hi)
{
	return idx >= 0 && idx <= hi;
}

static int qd_conv(struct qd_out *o, int c, const struct tm *tm)
{
	long year = (long) tm->tm_year + 1900L;
	int h12;

	switch (c) {
	case 'a':
		if (!qd_name_ok(tm->tm_wday, 6)) return 0;
		qd_puts_out(o, qd_wday_abbr[tm->tm_wday]);
		return 1;
	case 'A':
		if (!qd_name_ok(tm->tm_wday, 6)) return 0;
		qd_puts_out(o, qd_wday_name[tm->tm_wday]);
		return 1;
	case 'b':
		if (!qd_name_ok(tm->tm_mon, 11)) return 0;
		qd_puts_out(o, qd_mon_abbr[tm->tm_mon]);
		return 1;
	case 'B':
		if (!qd_name_ok(tm->tm_mon, 11)) return 0;
		qd_puts_out(o, qd_mon_name[tm->tm_mon]);
		return 1;
	case 'c':
		/* "%a %b %e %H:%M:%S %Y" in the C locale: %e is the day, space padded */
		if (!qd_name_ok(tm->tm_wday, 6) || !qd_name_ok(tm->tm_mon, 11)) return 0;
		qd_puts_out(o, qd_wday_abbr[tm->tm_wday]);
		qd_put(o, ' ');
		qd_puts_out(o, qd_mon_abbr[tm->tm_mon]);
		qd_put(o, ' ');
		qd_num(o, tm->tm_mday, 2, ' ');
		qd_put(o, ' ');
		qd_num(o, tm->tm_hour, 2, '0'); qd_put(o, ':');
		qd_num(o, tm->tm_min, 2, '0');  qd_put(o, ':');
		qd_num(o, tm->tm_sec, 2, '0');
		qd_put(o, ' ');
		qd_num(o, year, 1, '0');
		return 1;
	case 'd':
		qd_num(o, tm->tm_mday, 2, '0');
		return 1;
	case 'H':
		qd_num(o, tm->tm_hour, 2, '0');
		return 1;
	case 'I':
		h12 = tm->tm_hour % 12;
		if (h12 == 0)
			h12 = 12;
		qd_num(o, h12, 2, '0');
		return 1;
	case 'j':
		qd_num(o, (long) tm->tm_yday + 1L, 3, '0');
		return 1;
	case 'm':
		qd_num(o, (long) tm->tm_mon + 1L, 2, '0');
		return 1;
	case 'M':
		qd_num(o, tm->tm_min, 2, '0');
		return 1;
	case 'p':
		qd_puts_out(o, (tm->tm_hour >= 12) ? "PM" : "AM");
		return 1;
	case 'S':
		qd_num(o, tm->tm_sec, 2, '0');
		return 1;
	case 'U':
		if (!qd_name_ok(tm->tm_wday, 6)) return 0;
		qd_num(o, ((long) tm->tm_yday + 7L - tm->tm_wday) / 7L, 2, '0');
		return 1;
	case 'w':
		if (!qd_name_ok(tm->tm_wday, 6)) return 0;
		qd_num(o, tm->tm_wday, 1, '0');
		return 1;
	case 'W':
		if (!qd_name_ok(tm->tm_wday, 6)) return 0;
		qd_num(o, ((long) tm->tm_yday + 7L - ((tm->tm_wday + 6) % 7)) / 7L, 2, '0');
		return 1;
	case 'x':
		qd_num(o, (long) tm->tm_mon + 1L, 2, '0'); qd_put(o, '/');
		qd_num(o, tm->tm_mday, 2, '0');            qd_put(o, '/');
		qd_num(o, qd_fmod((long) tm->tm_year, 100L), 2, '0');
		return 1;
	case 'X':
		qd_num(o, tm->tm_hour, 2, '0'); qd_put(o, ':');
		qd_num(o, tm->tm_min, 2, '0');  qd_put(o, ':');
		qd_num(o, tm->tm_sec, 2, '0');
		return 1;
	case 'y':
		qd_num(o, qd_fmod((long) tm->tm_year, 100L), 2, '0');
		return 1;
	case 'Y':
		qd_num(o, year, 1, '0');
		return 1;
	case 'Z':
		qd_puts_out(o, "UTC");
		return 1;
	case '%':
		qd_put(o, '%');
		return 1;
	default:
		return 0;
	}
}

size_t strftime(char *s, size_t max, const char *fmt, const struct tm *tm)
{
	struct qd_out o;
	const char *p;

	if (s == NULL || max == 0)
		return 0;
	s[0] = '\0';
	if (fmt == NULL || tm == NULL)
		return 0;
	o.s = s;
	o.max = max;
	o.n = 0;
	o.ok = 1;
	p = fmt;
	while (*p != '\0' && o.ok) {
		if (*p != '%') {
			qd_put(&o, (unsigned char) *p);
			p++;
			continue;
		}
		p++;
		if (*p == '\0' || !qd_conv(&o, (unsigned char) *p, tm)) {
			o.ok = 0;
			break;
		}
		p++;
	}
	if (!o.ok) {
		s[0] = '\0';          /* does not fit, or unknown conversion: s is emptied */
		return 0;
	}
	if (o.n == 0)
		return 0;
	s[o.n] = '\0';
	return o.n;
}
