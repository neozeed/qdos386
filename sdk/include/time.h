/* time.h - QDOS/386 libc (R11D).  The structure and formatting functions work
 * on a caller's struct tm.  The calendar clock is not available in the QDOS/386
 * trap ABI: time, clock, mktime, gmtime, localtime, ctime and strftime fail
 * explicitly.  See docs/R11D-BLOCKED-APIS.md. */
#ifndef QDOS386_TIME_H
#define QDOS386_TIME_H
#include <stddef.h>

typedef unsigned long time_t;
typedef unsigned long clock_t;

#define CLOCKS_PER_SEC 1000  /* nominal: the tick rate is not in the trap ABI */

struct tm {
	int tm_sec;
	int tm_min;
	int tm_hour;
	int tm_mday;
	int tm_mon;
	int tm_year;
	int tm_wday;
	int tm_yday;
	int tm_isdst;
};

time_t time(time_t *t);
clock_t clock(void);
double difftime(time_t t1, time_t t0);
time_t mktime(struct tm *tm);
struct tm *gmtime(const time_t *t);
struct tm *localtime(const time_t *t);
char *asctime(const struct tm *tm);
char *ctime(const time_t *t);
size_t strftime(char *s, size_t max, const char *fmt, const struct tm *tm);

#endif
