/* time.c - QDOS/386 libc (R11D): struct tm functions that need no clock, and
 * explicit failures for the ones that do.  See docs/R11D-BLOCKED-APIS.md. */
#include <time.h>
#include <stdio.h>

double difftime(time_t t1, time_t t0)
{
	return (double) t1 - (double) t0;
}

/* The calendar clock is not in the QDOS/386 trap ABI.  These fail. */
time_t time(time_t *t)
{
	(void) t;
	return (time_t) -1;
}

clock_t clock(void)
{
	return (clock_t) -1;
}

time_t mktime(struct tm *tm)
{
	(void) tm;
	return (time_t) -1;
}

struct tm *gmtime(const time_t *t)
{
	(void) t;
	return NULL;
}

struct tm *localtime(const time_t *t)
{
	(void) t;
	return NULL;
}

char *ctime(const time_t *t)
{
	(void) t;
	return NULL;
}

size_t strftime(char *s, size_t max, const char *fmt, const struct tm *tm)
{
	(void) s;
	(void) max;
	(void) fmt;
	(void) tm;
	return 0;                    /* no conversion is provided */
}

/* asctime: "Www Mmm dd hh:mm:ss yyyy\n" (C89 4.12.3.1).  The fields are taken
 * as given; out-of-range fields print as given, not normalised. */
static const char *qd_wday[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
static const char *qd_mon[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
static char qd_asc[26];

char *asctime(const struct tm *tm)
{
	const char *wd = (tm->tm_wday >= 0 && tm->tm_wday <= 6) ? qd_wday[tm->tm_wday] : "???";
	const char *mo = (tm->tm_mon >= 0 && tm->tm_mon <= 11) ? qd_mon[tm->tm_mon] : "???";
	snprintf(qd_asc, sizeof qd_asc, "%.3s %.3s%3d %.2d:%.2d:%.2d %d\n", wd, mo, tm->tm_mday,
	         tm->tm_hour, tm->tm_min, tm->tm_sec, tm->tm_year + 1900);
	return qd_asc;
}
