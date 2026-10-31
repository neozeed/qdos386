/*
 * rtc.c - QDOS/386 R13B: CMOS real-time clock.  See rtc.h.  Strict C89.  rtc_decode and rtc_reason have no kernel
 * dependencies; rtc_read uses the PC's ports 0x70/0x71 (not compiled with RTC_HOST_TEST).
 */
#include "rtc.h"

static int bcd(unsigned int v, int binary, unsigned int *out)
{
	if (binary) {
		*out = v;
		return 0;
	}
	if ((v & 0x0fu) > 9u || (v >> 4) > 9u)
		return -1;
	*out = (v >> 4) * 10u + (v & 0x0fu);
	return 0;
}

static int leap(unsigned long y)
{
	return (y % 4ul == 0ul && y % 100ul != 0ul) || y % 400ul == 0ul;
}

/* Days from 1970-01-01 to y-m-d (y >= 1970). */
static unsigned long days_from_civil(unsigned long y, unsigned int m, unsigned int d)
{
	static const unsigned short cum[12] = { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };
	unsigned long n, yy = y - 1ul;

	n = (y - 1970ul) * 365ul + (yy / 4ul - 1969ul / 4ul) - (yy / 100ul - 1969ul / 100ul) + (yy / 400ul - 1969ul / 400ul);
	n += cum[m - 1u];
	if (m > 2u && leap(y))
		n++;
	return n + d - 1ul;
}

int rtc_decode(const struct rtc_raw *r, unsigned long *t)
{
	static const unsigned char mdays[12] = { 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
	int bin = (r->regb & 0x04u) != 0, h24 = (r->regb & 0x02u) != 0, pm = 0;
	unsigned int sec, min, hour, mday, mon, yy, cen, hr = r->hour;
	unsigned long year, days, secs;

	if (!(r->regd & 0x80u) || (r->sec == 0xffu && r->min == 0xffu && r->hour == 0xffu && r->mon == 0xffu))
		return RTC_ERR_NODEV;
	if (!h24) {
		pm = (hr & 0x80u) != 0;
		hr &= 0x7fu;
	}
	if (bcd(r->sec, bin, &sec) || bcd(r->min, bin, &min) || bcd(hr, bin, &hour) || bcd(r->mday, bin, &mday) ||
	    bcd(r->mon, bin, &mon) || bcd(r->year, bin, &yy))
		return RTC_ERR_DIGIT;
	if (bcd(r->century, bin, &cen))
		return RTC_ERR_CENTURY;
	if (!h24) {                                  /* 12-hour: 12 AM is 0, 12 PM is 12 */
		if (hour < 1u || hour > 12u)
			return RTC_ERR_RANGE;
		hour = (hour % 12u) + (pm ? 12u : 0u);
	}
	if (sec > 59u || min > 59u || hour > 23u || mon < 1u || mon > 12u || mday < 1u || mday > mdays[mon - 1u] || yy > 99u)
		return RTC_ERR_RANGE;
	if (cen < 19u || cen > 21u)
		return RTC_ERR_CENTURY;
	year = (unsigned long) cen * 100ul + yy;
	if (mon == 2u && mday == 29u && !leap(year))
		return RTC_ERR_RANGE;
	if (year < 1970ul || year > 2106ul)
		return RTC_ERR_EPOCH;
	days = days_from_civil(year, mon, mday);
	secs = (unsigned long) hour * 3600ul + (unsigned long) min * 60ul + sec;
	/* 0xFFFFFFFE = 2106-02-07 06:28:14 is day 49710, second 23294 */
	if (days > 49710ul || (days == 49710ul && secs > 23294ul))
		return RTC_ERR_EPOCH;
	*t = days * 86400ul + secs;
	return RTC_OK;
}

const char *rtc_reason(int rc)
{
	switch (rc) {
	case RTC_OK:           return "ok";
	case RTC_ERR_NODEV:    return "no RTC (status register D says the time is not valid)";
	case RTC_ERR_UNSTABLE: return "RTC never read the same twice";
	case RTC_ERR_DIGIT:    return "RTC registers are not valid BCD";
	case RTC_ERR_RANGE:    return "RTC date or time out of range";
	case RTC_ERR_CENTURY:  return "RTC has no usable century byte";
	case RTC_ERR_EPOCH:    return "RTC date outside 1970..2106-02-07 06:28:14 UTC";
	default:               return "RTC error";
	}
}

#ifndef RTC_HOST_TEST
#include "platform.h"

static unsigned int cmos(unsigned int reg)
{
	outb(0x70, reg);                             /* bit 7 clear: NMI stays enabled, as everywhere else in the kernel */
	return inb(0x71) & 0xffu;
}

static void snap(struct rtc_raw *r)
{
	unsigned int n;

	for (n = 0; n < 100000u && (cmos(0x0a) & 0x80u); n++)     /* update in progress: at most ~2 ms on real parts */
		;
	r->sec = (unsigned char) cmos(0x00);
	r->min = (unsigned char) cmos(0x02);
	r->hour = (unsigned char) cmos(0x04);
	r->wday = (unsigned char) cmos(0x06);
	r->mday = (unsigned char) cmos(0x07);
	r->mon = (unsigned char) cmos(0x08);
	r->year = (unsigned char) cmos(0x09);
	r->century = (unsigned char) cmos(0x32);
	r->rega = (unsigned char) cmos(0x0a);
	r->regb = (unsigned char) cmos(0x0b);
	r->regd = (unsigned char) cmos(0x0d);
}

static int same(const struct rtc_raw *a, const struct rtc_raw *b)
{
	return a->sec == b->sec && a->min == b->min && a->hour == b->hour && a->mday == b->mday && a->mon == b->mon &&
	       a->year == b->year && a->century == b->century && a->regb == b->regb;
}

int rtc_read(unsigned long *t, struct rtc_raw *raw)
{
	struct rtc_raw r[2];
	unsigned int k;
	int i, cur = 0, ok = 0;

	snap(&r[0]);
	for (i = 0; i < 8; i++) {                    /* no struct assignment: the kernel has no memcpy */
		snap(&r[1 - cur]);
		cur = 1 - cur;
		if (same(&r[0], &r[1])) {
			ok = 1;
			break;
		}
	}
	if (raw != 0)
		for (k = 0; k < sizeof *raw; k++)
			((unsigned char *) raw)[k] = ((const unsigned char *) &r[cur])[k];
	if (!ok)
		return RTC_ERR_UNSTABLE;
	return rtc_decode(&r[cur], t);
}
#endif
