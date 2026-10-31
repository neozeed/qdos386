/*
 * rtc.h - QDOS/386 R13B: the PC CMOS real-time clock (MC146818 compatible) as the wall-clock anchor.  See
 * docs/R13B-CLOCK-ABI.md.
 *
 * The RTC is read once at boot (clock_boot.c); from then on the R11F clock advances with the timer, so wall time and
 * the scheduler tick count stay separate.  rtc_decode has no kernel dependencies and is unit-tested on the host
 * (tests/r13b/host-rtc.c).
 */
#ifndef QDOS386_RTC_H
#define QDOS386_RTC_H

struct rtc_raw {
	unsigned char sec, min, hour, wday, mday, mon, year;   /* registers 0x00 0x02 0x04 0x06 0x07 0x08 0x09 */
	unsigned char century;                                  /* register 0x32 (the PC/AT century byte) */
	unsigned char rega, regb, regd;                         /* status registers A, B, D */
};

#define RTC_OK            0
#define RTC_ERR_NODEV    (-1)    /* register D bit 7 (valid RAM and time) clear, or the registers float (0xFF) */
#define RTC_ERR_UNSTABLE (-2)    /* never two identical reads outside an update cycle */
#define RTC_ERR_DIGIT    (-3)    /* BCD mode, but a nibble above 9 */
#define RTC_ERR_RANGE    (-4)    /* a field out of range (second, minute, hour, day of the month, month) */
#define RTC_ERR_CENTURY  (-5)    /* no usable century byte (19, 20 or 21): the year would have to be guessed */
#define RTC_ERR_EPOCH    (-6)    /* before 1970-01-01 00:00:00 or after 2106-02-07 06:28:14 UTC (time_t range) */

/* Decode raw registers (BCD or binary per register B bit 2; 12- or 24-hour per bit 1, PM = hour bit 7 in 12-hour
 * mode) into seconds since 1970-01-01 00:00:00 UTC.  The RTC is taken to keep UTC (QEMU's default, Tiny386).  Returns
 * RTC_OK and *t, or a negative RTC_ERR_*; *t is not written on failure. */
int rtc_decode(const struct rtc_raw *r, unsigned long *t);

/* Read the CMOS clock: waits for no update in progress, reads until two consecutive reads agree (at most 8), then
 * decodes.  raw (may be 0) receives the last registers read.  PC platform only. */
int rtc_read(unsigned long *t, struct rtc_raw *raw);

/* The reason text for an RTC_ERR_* (for the boot line). */
const char *rtc_reason(int rc);

#endif
