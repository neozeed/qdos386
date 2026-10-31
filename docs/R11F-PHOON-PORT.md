# Phoon on QDOS/386: portability patch (R11F)

Phoon is the moon-phase program by Jef Poskanzer (see `third_party/phoon/orig/README`).
The original sources are kept unchanged in `third_party/phoon/orig/`. The R11F
port is the patch `third_party/phoon/r11f-portability.patch`, applied by
`scripts/r11f-phoon.sh` to a copy in `build/phoon-r11f/src/`. The result is built
as `build/apps/PHOON.aout` with `qdos386-gcc -DOS_SYSV`.

The astronomy (`astro.c`, `astro.h`) is not changed. The date parser's grammar
and tables are not changed. The patch changes only what QDOS/386 cannot provide
or where the original assumed a signed 32-bit time.

## What the patch changes

### date_parse.c

1. **Removed `#include <sys/types.h>`.** QDOS/386 has no such header. `time_t`
   comes from the SDK `<time.h>`.
2. **Removed `tzset()`.** The SDK has no `tzset`. The `OS_SYSV` branch already set
   `gmtoff = 0` (QL has no time zones), so the call did nothing useful.
3. **Raw Unix times read through a `long`.** `strtol` now stores into `lt`, and
   negative values are refused (`-1`) as before. The original stored into a
   signed `time_t`. Since `time_t` is unsigned here, the sign must be tested
   before the conversion.
4. **NULL-safe defaults.** If `localtime()` returns NULL (no clock), the
   defaults stay zero rather than dereferencing NULL. Every date format sets the
   year, so a date without a year is refused by `tm_to_time`. A time-only input
   such as `11:57` is therefore refused when there is no clock. This is the
   documented "no clock is configured; give a date" behaviour.
5. **Day 0 refused without a clock.** When there is no clock and the day is 0,
   the date is refused. Day 0 is never a real day, and without this it would
   silently map to the day before the 1st of the month.
6. **`tm_to_time` with signed days.** The day count is computed in `long` and
   refused when negative (before 1970) or greater than 49710 (the last day
   before 2106-02-07). The seconds-of-day are checked before they are added, so
   the unsigned result cannot wrap. The original returned a signed value that
   could represent dates before 1970.
7. **Year range 70..206.** Years before 1970 or after 2106 are refused.

### phoon.c

8. **Unsigned clock value.** `clocknow` is `unsigned long` because `time()` now
   returns an unsigned `time_t`, and storing it in a signed `long` would need a
   conversion. It is used only as a random seed (`clocknow % 13`, the "Hubert"
   easter egg). For every time after 1970 the value is the same as before, so
   the seed is unchanged. `time((time_t) 0)` replaces `time(&clocknow)`.
9. **No-clock refusal.** With no arguments and no clock, `time()` returns
   `(time_t) -1`. Phoon prints `no clock is configured; give a date` and exits
   with status 1. The original would have printed a phase for time 0xFFFFFFFF.
10. **Invalid parse result.** `date_parse` can return `(time_t) -1`, which is now
    refused with the usual `illegal date/time` message. The original tested
    `t <= 0` on a signed value. The new test is `t == (time_t) -1 || t <= 0`.

## Preserved behaviour

- The astronomy calculation, its constants, and its output format are unchanged.
- The date grammar (`DD mth YY HH:MM:SS ampm zone` and the `ctime` style) is
  unchanged. The sscanf format list is unchanged.
- The argument syntax (`-l <lines>`, `<date/time>`, and the no-argument case) is
  unchanged. The only new output is the no-clock message.
- `t <= 0` still refuses the epoch itself, as the original did.
- Output text is identical between the QDOS/386 build and the host build of the
  same patched source, for the 24-case battery (see the runtime report).

## Leap-day correction for 2101 and later

The original `tm_to_time` counts the leap days before a year as
`(tm_year - 69) / 4` and carries the comment "this will break in 2100!". That
formula is correct through 2100, but it overcounts from 2101: 2100 is divisible by
100 and not by 400, so it is not a leap year, and the formula still counts it. Every
date from 1 January 2101 on was therefore one calendar day late. Years 2101 to 2106 are
inside the range the R11F time ABI supports, so this had to be corrected.

The correction is a single expression in `date_parse.c`:

    days += ( tmP->tm_year - 69 ) / 4 - ( tmP->tm_year > 200 ? 1 : 0 );

Its increment against the first portability patch is kept as
`third_party/phoon/r11f-leapday-2101.diff`. The full patch,
`third_party/phoon/r11f-portability.patch`, now includes it. The original sources in
`third_party/phoon/orig/` are unchanged. Nothing else in `tm_to_time` changes, and the
date grammar, the astronomy and the 2100 results are unchanged. Attribution: the
defect is in Jef Poskanzer's original `date_parse.c`; the correction is R11F's.

### Verification

Expected values are Python's `calendar.timegm` (independent of Phoon), and GNU
`date -u` agrees with them for the boundary cases checked by hand:

- `2100-02-28 23:59:59` = 4107542399; `date -u -d` gives the same.
- `2101-01-01 00:00:00` = 4133980800; `date -u -d` gives the same.

`sdk/examples/r11f/phdate/gen_expected.py` produces 32 cases around 28 Feb/1 Mar 2100,
31 Dec 2100/1 Jan 2101, the 2104 and 2105 leap years, the post-2038 battery dates, the
2106 limit, the epoch, and ordinary dates. The probe `PHDATE.AOU`
(`sdk/examples/r11f/phdate/phdate.c`) calls the patched `date_parse` directly, on the
host (`gcc -std=gnu89 -DOS_SYSV`) and on the target.

- Host, corrected source: 32 of 32 match.
- Host, pre-fix source (the first patch): 13 of 32 differ. All of them are from 1 January
  2101 on, each one day (86400 s) late. The pre-fix build also refuses
  `7 Feb 2106 06:28:13` and `06:28:14`, which are inside the supported range.
- Target: see `docs/R11F-RUNTIME-REPORT.md`.

## Supported time range

`date_parse` and `tm_to_time` accept years 1970 to 2106. A time is accepted when it is
between `1970-01-01 00:00:00 UTC` (0) and `2106-02-07 06:28:14 UTC` (4294967294), the
last value of `time_t` other than the `0xFFFFFFFF` sentinel. Inside that range the
result is exact. Outside it, `date_parse` returns `(time_t) -1` and Phoon prints
`illegal date/time`, with exit status 1.

- `7 Feb 2106 06:28:14` is accepted. `7 Feb 2106 06:28:15` and `8 Feb 2106` are refused.
- `31 Dec 1969 23:59:59` and earlier are refused.
- Phase events after 2106 are not a problem. Phoon prints them as offsets from the
  requested time, so they need no `time_t` value.

