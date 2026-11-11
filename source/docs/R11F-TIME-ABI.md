# R11F time ABI and UTC policy

Status: R11F. Covers the trap, the kernel wall-clock service, and the SDK
`<time.h>` functions. Companion documents: `docs/R11F-FIXED-CLOCK.md`
(build and run instructions for fixed clocks), `docs/R11F-PHOON-PORT.md`
(the Phoon portability patch), `docs/R11F-C89-COVERAGE.md`.

## 1. Three different notions of time

QDOS/386 has three separate time sources. They are not interchangeable.

| Source | What it is | Available to apps as |
|---|---|---|
| Wall clock | Seconds since 1970-01-01 00:00:00 UTC, from a configured anchor plus elapsed time | `time()`, `gmtime()`, `localtime()`, `mktime()`, `ctime()`, `asctime()`, `strftime()` via trap `QT_WALL_TIME` (0x23) |
| Scheduler tick count | Number of timer interrupts since boot (`sched_ticks`) | Not exposed as a time API. Used only inside the kernel. |
| CPU time of a process | Time the process has spent running | Not implemented. `clock()` returns `(clock_t) -1`. |

`clock()` is deliberately unavailable. The wall clock and the tick count are
elapsed time, not process CPU time, and returning either one from `clock()`
would give wrong answers to programs that measure their own cost. `clock()`
becomes available only when correct per-process CPU accounting exists.

## 2. time_t

- `time_t` is `unsigned long`, 32 bits on the i386 target.
- Valid values: `0` through `0xFFFFFFFE`.
- `0` is 1970-01-01 00:00:00 UTC.
- `0xFFFFFFFE` is 2106-02-07 06:28:14 UTC, the last representable second.
- `0xFFFFFFFF` is the unavailable sentinel. `time()` returns it as
  `(time_t) -1`. It is never produced by `mktime()` for a valid date.
- There is no negative time. Dates before 1970-01-01 cannot be represented,
  and `mktime()` refuses them.

The 2038 signed-32-bit boundary is not a problem. Values from 2^31 to
0xFFFFFFFE are ordinary dates in 2038-2106. A test covers 2147483647 and
2147483648.

## 3. UTC policy

- All calendar conversions use UTC. There is no time zone database.
- `localtime()` is identical to `gmtime()`. It returns the UTC broken-down time.
- `tm_isdst` is always 0.
- `strftime()` with `%Z` writes `UTC`.
- The calendar is the proleptic Gregorian calendar. Leap years follow the
  usual rule: divisible by 4, except centuries, which must be divisible by 400.
  2000 is a leap year. 2100 is not.
- Week days use `tm_wday` 0 = Sunday. Days of the year use `tm_yday` 0..365.

## 4. Clock model and configuration

The wall clock is a kernel service (`kernel/clock.c`, `kernel/clock_boot.c`).

- An anchor (`clock_set`) gives the time at a known tick baseline.
- Each timer tick adds `den` to an accumulator. Whole seconds are then moved
  to `secs` at a rate of `num/den` ticks per second, and the remainder is kept.
  This keeps the arithmetic exact for any timer rate.
- The timer rate comes from `timer_rate()` in the platform layer. On the PC it
  is the PIT rate divided by the programmed divisor (1193182 / 11932, that is
  100 Hz). On i386VM it is the nominal 100/1 rate. The i386VM timer is
  step-based, not wall time, so its clock runs at the nominal rate and is not
  synchronised with real time.
- Overflow of the secs counter is checked. A clock whose elapsed seconds would
  pass 0xFFFFFFFE enters the OVERFLOW state and reports unavailable.
- `clock_now()` returns the anchor plus elapsed seconds, or unavailable.

Build variants (see `kernel/clock_boot.h`):

| Build | Macro | Behaviour |
|---|---|---|
| Default | none | Clock is NONE. `time()` returns -1 and calendar functions fail. |
| Advancing | `-DQDOS_CLOCK_EPOCH=<seconds>` | Anchor set. Time advances with ticks. |
| Pinned | `-DQDOS_CLOCK_EPOCH=<seconds> -DQDOS_CLOCK_PINNED` | Anchor set. Time does not advance. Used for reproducible tests. |

A kernel built without the timer (`QDOS_NO_TIMER`) is always pinned, because
nothing would advance an unpinned clock.

The default build has no anchor because the machine has no trustworthy
real-time source in the trap ABI. A default-build program that calls `time(NULL)`
gets `(time_t) -1`, which is the documented "no clock" answer.

## 5. Trap ABI

- Trap number `QT_WALL_TIME` = `0x23` (`kernel/jobs.h`), `QD_TRAP_WALL` = `0x23u`
  (`sdk/include/qdos386.h`).
- The trap takes no arguments and returns an `unsigned long`.
- SDK wrapper: `unsigned long qd_wall_time(void)` (`sdk/lib/qdos.c`).
- The trap returns `0xFFFFFFFF` when the clock is unavailable.
- The trap is not a time-of-day query for arbitrary processes. It returns the
  same value to every caller at the same instant.

## 6. Function behaviour

| Function | Behaviour |
|---|---|
| `time(t)` | Returns the wall time, or `(time_t) -1` when unavailable. Stores it in `*t` if `t` is not NULL. |
| `clock()` | Always returns `(clock_t) -1`. Unavailable (see section 1). |
| `difftime(a, b)` | Returns `(double) a - (double) b`. Pure arithmetic, no clock. |
| `mktime(tm)` | Normalises `sec`, `min`, `hour`, `mday`, `mon`, `year` in that order with checked arithmetic. Fills `tm` with the normalised fields, including `tm_wday`, `tm_yday`, `tm_isdst = 0`. Returns `(time_t) -1` and leaves `tm` unchanged for results before the epoch, after `0xFFFFFFFE`, for a normalised year outside -1000000..1000000, or when the intermediate arithmetic would overflow. |
| `gmtime(t)` | Returns a pointer to a static `struct tm`, or NULL if `t` is NULL or `*t > 0xFFFFFFFE`. NULL is also returned for the sentinel 0xFFFFFFFF, so an invented date is never produced. |
| `localtime(t)` | Same as `gmtime`. |
| `asctime(tm)` | Returns a pointer to a static 26-byte buffer, format `"Www Mmm dd hh:mm:ss yyyy\n"`. Out-of-range week or month names print as `???`. |
| `ctime(t)` | `asctime(localtime(t))`, copied into a separate static buffer. NULL when the time is unavailable. |
| `strftime(s, max, fmt, tm)` | Supports `%a %A %b %B %c %d %H %I %j %m %M %p %S %U %w %W %x %X %y %Y %Z %%`. Any other conversion fails. If the result (including the terminating NUL) does not fit in `max`, it fails. On failure it returns 0 and sets `s[0] = 0` when `max > 0`. |

### Static buffers

`gmtime`, `localtime`, `asctime`, and `ctime` return pointers to static storage.
Each call overwrites the previous result. Copy the result before making another
call if it must be kept. `mktime` and `strftime` do not use static storage.

## 7. Validation

- `sdk/examples/r11f/timeabi/timeabi.c` checks the calendar functions against
  `timeabi_vectors.h`, generated by `gen_vectors.py`. The expected values come
  from Python `datetime` and the host C library `strftime`, not from the SDK.
- Tests in `timeabi` run in three modes: `none` (time unavailable), `pin`
  (pinned at 2026-10-27 12:00:00 UTC, 1793102400), and `adv` (advancing from
  2000-01-01 00:00:00 UTC, 946684800).
- Host tests of the clock arithmetic (`tests/host/r11f/clock_test.c`, 24
  checks) use expected literals computed independently in Python. They cover
  the tick rates, large tick counts, the 2^32 boundary, and the overflow
  edges.
- The 2^32 tick wrap cannot be reached on a target within a practical test
  time. It is covered only by the host tests.

## 8. Known limitations

- `clock()` is unavailable.
- Only UTC is supported. There is no time zone database and no DST.
- Dates before 1970 and after 2106-02-07 06:28:14 UTC cannot be represented.
- The i386VM clock is nominal and does not track real time.
- `mktime()` uses 32-bit intermediate limits and refuses rather than wraps.
