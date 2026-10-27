#!/usr/bin/env python3
"""Expected results for the R11F date probe (PHDATE).

Expected values come from Python's calendar.timegm (UTC, proleptic Gregorian), not from the
code under test.  A date inside the documented range (1970-01-01 00:00:00 UTC to
2106-02-07 06:28:14 UTC) must give its timestamp; a date outside it must be refused.
Usage: LC_ALL=C python3 gen_expected.py > phdate-expected.txt   (TAB-separated: input, expected)
"""
import calendar

TMAX = 0xFFFFFFFE
CASES = [
    # around 28 Feb / 1 Mar 2100 (2100 is not a leap year)
    "28 Feb 2100 00:00:00", "28 Feb 2100 23:59:59", "1 Mar 2100 00:00:00", "1 Mar 2100 12:00:00",
    # around 31 Dec 2100 / 1 Jan 2101 (the leap-day defect was here)
    "31 Dec 2100 23:59:59", "1 Jan 2101 00:00:00", "1 Jan 2101 12:00:00", "2 Jan 2101 00:00:00",
    "15 Jun 2101 12:00:00", "1 Mar 2101 00:00:00",
    # leap years after the defect
    "29 Feb 2104 00:00:00", "1 Mar 2104 00:00:00", "31 Dec 2104 23:59:59", "1 Jan 2105 00:00:00",
    "31 Dec 2105 23:59:59",
    # post-2038 battery dates
    "1 Mar 2038 12:00:00", "1 Jan 2070 00:00:00", "15 Jun 2090 12:00:00", "1 Jan 2101 12:00:00",
    "7 Feb 2106 06:28:13",
    # the limit and just past it
    "7 Feb 2106 06:28:14", "7 Feb 2106 06:28:15", "8 Feb 2106 00:00:00", "1 Jan 2107 00:00:00",
    # the epoch and before it
    "1 Jan 1970 00:00:00", "1 Jan 1970 00:00:01", "31 Dec 1969 23:59:59", "1 Jan 1969 00:00:00",
    # ordinary dates
    "1 Jan 2000 00:00:00", "29 Feb 2000 12:00:00", "1 Mar 1972 00:00:00", "11 Jan 2024 11:57:00",
]

def expected(text):
    d, mon, y, hms = text.split()
    months = ["Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"]
    H, M, S = map(int, hms.split(":"))
    t = calendar.timegm((int(y), months.index(mon) + 1, int(d), H, M, S, 0, 0, 0))
    if t < 0 or t > TMAX:
        return "refused"
    return str(t)

if __name__ == "__main__":
    for c in CASES:
        print("%s\t%s" % (c, expected(c)))
