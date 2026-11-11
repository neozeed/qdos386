/* time.h - QDOS/386 libc (R11F).
 *
 * Calendar time (R11F).  time_t is unsigned 32-bit seconds since
 * 1970-01-01 00:00:00 UTC.  The valid range is 0 .. 0xFFFFFFFE, that is
 * 1970-01-01 00:00:00 UTC through 2106-02-07 06:28:14 UTC.  The value
 * 0xFFFFFFFF is the "unavailable" sentinel and is never a valid result.
 *
 * Policy (see docs/R11F-TIME-ABI.md):
 *   - UTC only.  localtime() is identical to gmtime(); tm_isdst is always 0;
 *     %Z formats as "UTC".  There is no time zone database.
 *   - Gregorian calendar, proleptic.
 *   - time() and the calendar functions fail (time() returns (time_t) -1,
 *     gmtime/localtime/ctime return NULL) when no clock is configured in the
 *     kernel, or when the clock has overflowed.  See docs/R11F-TIME-ABI.md.
 *   - mktime() normalises out-of-range fields and returns (time_t) -1 for
 *     results before the epoch, after 0xFFFFFFFE, or for fields it cannot
 *     represent.  On failure the struct tm is left unchanged.
 *   - gmtime(), localtime(), asctime() and ctime() return pointers to static
 *     storage that the next call overwrites.  Copy the result to keep it.
 *   - clock() is NOT available: it returns (clock_t) -1.  The wall clock and
 *     the scheduler tick counter are not CPU-time accounting, and clock() is
 *     not implemented by returning either of them.
 *   - strftime() supports the C89 conversions only.  An unknown conversion or
 *     a result that does not fit fails: it returns 0 and writes an empty
 *     string.
 */
#ifndef QDOS386_TIME_H
#define QDOS386_TIME_H
#include <stddef.h>

typedef unsigned long time_t;
typedef unsigned long clock_t;

/* clock() is unavailable; CLOCKS_PER_SEC is defined for source compatibility
 * only and does not describe any real clock. */
#define CLOCKS_PER_SEC 1000

struct tm {
	int tm_sec;     /* 0..60 (leap second allowance) */
	int tm_min;     /* 0..59 */
	int tm_hour;    /* 0..23 */
	int tm_mday;    /* 1..31 */
	int tm_mon;     /* 0..11, January = 0 */
	int tm_year;    /* years since 1900 */
	int tm_wday;    /* 0..6, Sunday = 0 */
	int tm_yday;    /* 0..365, January 1 = 0 */
	int tm_isdst;   /* always 0 (UTC) */
};

time_t time(time_t *t);
clock_t clock(void);                      /* unavailable: returns (clock_t) -1 */
double difftime(time_t t1, time_t t0);
time_t mktime(struct tm *tm);
struct tm *gmtime(const time_t *t);
struct tm *localtime(const time_t *t);    /* UTC: same as gmtime() */
char *asctime(const struct tm *tm);
char *ctime(const time_t *t);
size_t strftime(char *s, size_t max, const char *fmt, const struct tm *tm);

#endif
