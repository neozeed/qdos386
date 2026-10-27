#!/usr/bin/env python3
"""Generate timeabi_vectors.h: expected values for the R11F calendar tests.

The expected values come from Python's datetime and the host C library's
strftime, not from the SDK code under test.  Run:  python3 gen_vectors.py > timeabi_vectors.h

Conventions (must match docs/R11F-TIME-ABI.md):
  time_t is unsigned 32-bit seconds since 1970-01-01 00:00:00 UTC.
  Valid time_t values are 0 .. 0xFFFFFFFE (time 0xFFFFFFFF is the unavailable
  sentinel and is never a result of mktime).
  struct tm: tm_year = year - 1900, tm_mon 0..11, tm_wday 0 = Sunday,
  tm_yday 0..365, tm_isdst 0.
"""
import datetime
import time
import sys

EPOCH = datetime.datetime(1970, 1, 1)
TMAX = 0xFFFFFFFE
WDAY = ["Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"]
WDAY_FULL = ["Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"]
MON = ["Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"]
MON_FULL = ["January", "February", "March", "April", "May", "June", "July",
            "August", "September", "October", "November", "December"]


def fields_of(dt):
    """C struct tm fields for a datetime (UTC, proleptic Gregorian)."""
    # Python weekday(): Monday = 0.  C tm_wday: Sunday = 0.
    wday = (dt.weekday() + 1) % 7
    yday = dt.timetuple().tm_yday - 1
    return (dt.second, dt.minute, dt.hour, dt.day, dt.month - 1,
            dt.year - 1900, wday, yday)


def time_of(dt):
    return int((dt - EPOCH).total_seconds())


def gmtime_vectors():
    samples = [
        0, 1, 59, 60, 3599, 3600, 86399, 86400,
        951782400,      # 2000-02-29 00:00:00 (leap day, 2000)
        951868800,      # 2000-03-01
        946684799,      # 1999-12-31 23:59:59
        946684800,      # 2000-01-01 00:00:00
        4102444800,     # 2100-01-01 00:00:00 (not a leap year)
        1234567890,
        1000000000,
        811036800,      # 1995-09-14 00:00:00
        1700000000,
        2147483647,     # 2038-01-19 03:14:07 (the old signed boundary)
        2147483648,     # first value past the old signed limit
        4294967294,     # TMAX: 2106-02-07 06:28:14
    ]
    out = []
    for t in samples:
        dt = EPOCH + datetime.timedelta(seconds=t)
        out.append((t, fields_of(dt)))
    return out


def gregorian_leap_checks():
    """Dates chosen across the century rules.  Each is a midnight in range."""
    dates = [
        (2000, 2, 29), (2000, 3, 1),       # 2000 is a leap year (divisible by 400)
        (2004, 2, 29), (2008, 2, 29), (2012, 2, 29),
        (2100, 2, 28), (2100, 3, 1),       # 2100 is not a leap year, but is in range
        (2096, 2, 29),
        (1970, 1, 1), (1970, 12, 31), (1971, 1, 1),
        (1972, 2, 29), (1976, 2, 29),
        (2038, 1, 19),
        (2106, 2, 7),
    ]
    out = []
    for (y, m, d) in dates:
        dt = datetime.datetime(y, m, d)
        t = time_of(dt)
        if t < 0 or t > TMAX:
            continue
        out.append((t, fields_of(dt)))
    return out


def mktime_vectors():
    """(input fields, expected time or -1, expected normalized fields or None).

    Input: (sec, min, hour, mday, mon, year-1900).  Expected: normalized via
    Python datetime arithmetic: the first of month plus (mday-1) days, plus
    hour/min/sec as offsets.  Out-of-range results give -1."""
    cases = [
        # exact in-range values
        (0, 0, 0, 1, 0, 70),                # epoch
        (59, 59, 23, 31, 11, 69),           # 1969-12-31 23:59:59: before epoch
        (0, 0, 0, 1, 0, 100),               # 2000-01-01
        (0, 0, 0, 29, 1, 100),              # 2000-02-29 (leap)
        (0, 0, 0, 29, 1, 200),              # 2100-03-01 is not 2100-02-29 (not leap)
        (30, 45, 12, 15, 5, 96),            # 1996-06-15 12:45:30
        (0, 0, 0, 1, 0, 136),               # 2036-01-01
        (14, 28, 6, 7, 1, 206),             # 2106-02-07 06:28:14 == TMAX
        (15, 28, 6, 7, 1, 206),             # one second past TMAX: rejected
        # month overflow
        (0, 0, 0, 1, 12, 70),               # Jan 1971
        (0, 0, 0, 1, 13, 70),               # Feb 1971
        (0, 0, 0, 1, 25, 70),               # Feb 1972 (leap year)
        (0, 0, 0, 1, -1, 71),               # Dec 1970
        (0, 0, 0, 1, -12, 71),              # Jan 1970
        (0, 0, 0, 1, -13, 71),              # Dec 1969: before epoch, rejected
        # day zero and day overflow
        (0, 0, 0, 0, 2, 100),               # Feb 29 2000 (day 0 of March 2000)
        (0, 0, 0, 0, 0, 70),                # Dec 31 1969: before epoch, rejected
        (0, 0, 0, 0, 0, 71),                # Dec 31 1970 (day 0 of Jan 1971)
        (0, 0, 0, 32, 0, 70),               # Feb 1 1970
        (0, 0, 0, 366, 0, 70),              # Jan 1 1971 (1970 has 365 days)
        (0, 0, 0, 31, 1, 100),              # Mar 2 2000 (Feb 31 overflows)
        # hour, minute, second overflow and negative values
        (60, 0, 0, 1, 0, 70),               # 00:01:00
        (0, 60, 0, 1, 0, 70),               # 01:00:00
        (0, 0, 24, 1, 0, 70),               # 1970-01-02 00:00:00
        (3600 * 25, 0, 0, 1, 0, 70),        # 1970-01-02 01:00:00
        (-1, 0, 0, 1, 0, 70),               # 1969-12-31 23:59:59: rejected
        (-1, 0, 0, 2, 0, 70),               # 1970-01-01 23:59:59
        (0, 0, 0, 1, 0, 70),                # epoch again (sanity)
        (-86400, 0, 0, 2, 0, 70),           # 1970-01-01 00:00:00
        (0, -1, 0, 2, 0, 70),               # 1970-01-01 23:59:00
        (0, 0, -1, 2, 0, 70),               # 1970-01-01 23:00:00
        (0, 0, -25, 3, 0, 70),              # 1970-01-01 23:00:00 (hour -25)
        (0, 0, 0, 1, 1, 135),               # Feb 2035 (not a rejected case)
    ]
    out = []
    for (sec, mn, hr, md, mo, yr) in cases:
        year = 1900 + yr
        total = (year * 12) + mo
        y = total // 12
        m = total % 12 + 1
        try:
            base = datetime.datetime(y, m, 1) + datetime.timedelta(
                days=md - 1, hours=hr, minutes=mn, seconds=sec)
        except OverflowError:
            base = None
        if base is None or base < EPOCH:
            expect = -1
            norm = None
        else:
            expect = time_of(base)
            if expect > TMAX:
                expect = -1
                norm = None
            else:
                norm = fields_of(base)
        out.append(((sec, mn, hr, md, mo, yr), expect, norm))
    return out


def strftime_vectors():
    """(t, format, expected string).  Expected produced by the C library's
    strftime in the C locale, with %Z removed (it is 'UTC' by policy and
    checked separately)."""
    times = [0, 951782400, 1234567890, 4294967294, 946684800, 811036800]
    fmts = ["%a %A %b %B", "%d %H %I %j", "%m %M %p %S", "%U %w %W",
            "%x %X", "%y %Y", "%%", "%c", "[%a%%%b]", "plain text", ""]
    out = []
    for t in times:
        tm = time.gmtime(t)
        for f in fmts:
            expect = time.strftime(f, tm)
            out.append((t, f, expect))
    return out


def asctime_vectors():
    out = []
    for t in [0, 951782400, 1234567890, 4294967294, 946684800]:
        dt = EPOCH + datetime.timedelta(seconds=t)
        s = "%s %s %2d %02d:%02d:%02d %d\n" % (
            WDAY[(dt.weekday() + 1) % 7], MON[dt.month - 1], dt.day,
            dt.hour, dt.minute, dt.second, dt.year)
        out.append((t, s))
    return out


def emit_struct_array(name, rows):
    print("static const struct qd_vec_%s qd_vec_%s[] = {" % (name, name))
    for r in rows:
        print("\t%s," % r)
    print("};")
    print("static const int qd_vec_%s_n = %d;" % (name, len(rows)))


def main():
    print("/* timeabi_vectors.h - GENERATED by gen_vectors.py.  Do not edit.")
    print(" * Expected values from Python datetime and the host C library. */")
    print("#ifndef TIMEABI_VECTORS_H")
    print("#define TIMEABI_VECTORS_H")
    print()
    print("struct qd_tmf { int sec, min, hour, mday, mon, year, wday, yday; };")
    print("struct qd_vec_gm  { unsigned long t; struct qd_tmf f; };")
    print("struct qd_vec_mk  { int sec, min, hour, mday, mon, year;"
          " unsigned long expect; int has_norm; struct qd_tmf norm; };")
    print("struct qd_vec_sf  { unsigned long t; const char *fmt; const char *expect; };")
    print("struct qd_vec_as  { unsigned long t; const char *expect; };")
    print()

    rows = []
    for (t, f) in gmtime_vectors() + gregorian_leap_checks():
        rows.append("{ %dUL, { %d, %d, %d, %d, %d, %d, %d, %d } }" % ((t,) + f))
    emit_struct_array("gm", rows)
    print()

    rows = []
    for (inp, expect, norm) in mktime_vectors():
        if norm is None:
            nf = "0, { 0, 0, 0, 0, 0, 0, 0, 0 }"
            has = 0
        else:
            nf = "1, { %d, %d, %d, %d, %d, %d, %d, %d }" % norm
            has = 1
        exp_u = expect & 0xFFFFFFFF       # -1 is written as the 0xFFFFFFFF sentinel
        rows.append("{ %d, %d, %d, %d, %d, %d, %dUL, %s }" % (inp + (exp_u, nf)))
    emit_struct_array("mk", rows)
    print()

    rows = []
    for (t, f, e) in strftime_vectors():
        rows.append('{ %dUL, "%s", "%s" }' % (t, c_escape(f), c_escape(e)))
    emit_struct_array("sf", rows)
    print()

    rows = []
    for (t, s) in asctime_vectors():
        rows.append('{ %dUL, "%s" }' % (t, c_escape(s)))
    emit_struct_array("as", rows)
    print()
    print("#endif")


def c_escape(s):
    out = []
    for ch in s:
        if ch == '\\':
            out.append('\\\\')
        elif ch == '"':
            out.append('\\"')
        elif ch == '\n':
            out.append('\\n')
        elif ord(ch) < 32 or ord(ch) > 126:
            out.append('\\%03o' % ord(ch))
        else:
            out.append(ch)
    return ''.join(out)


if __name__ == "__main__":
    main()
