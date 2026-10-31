# R11F known limitations

Everything from `docs/R11E-KNOWN-LIMITATIONS.md` still applies unless listed as changed
here. Items are grouped by what they affect.

## Calendar time

- **UTC only.** `localtime()` equals `gmtime()`. There is no time zone database and no DST.
  `tm_isdst` is always 0. `%Z` is `UTC`.
- **Range 1970-01-01 00:00:00 UTC to 2106-02-07 06:28:14 UTC.** Earlier times are
  refused. `time_t` is 32-bit unsigned.
- **`clock()` is unavailable.** It returns `(clock_t) -1`. The wall clock and the
  tick count are not process CPU time, and `clock()` is not implemented from them.
  `CLOCKS_PER_SEC` is defined only for source compatibility.
- **The default kernel has no clock.** `time()` returns `(time_t) -1`, and calendar
  functions fail. Images with a clock must be built with `QDOS_CLOCK_EPOCH`.
- **Tick-wrap at 2^32 is tested only on the host** (`tests/host/r11f`). The target
  cannot wait long enough to reach it.
- **The i386VM clock is nominal.** Its timer is step-based, so its clock runs at the
  nominal 100/1 rate and is not synchronised with real time.
- **Static return buffers** in `gmtime`, `localtime`, `asctime` and `ctime`.

## Phoon

- **Leap-day defect from 1 January 2101: fixed in R11F** (see `docs/R11F-PHOON-PORT.md`).
  Verified against independent UTC values in `sdk/examples/r11f/phdate`.
- **Moon-phase accuracy.** Phoon's Walker-style model differs from PyEphem (ELP2000)
  by +27 s to +277 s across the battery dates from 2000 to 2100 (median about 80 s). The positive bias grows away
  from 1980. This is the original algorithm's accuracy, not a porting change. Host
  and target print identical text.
- **ISO dates are not accepted** (original behaviour).
- **A time-only argument is refused without a clock** (new, explicit message).
- **Negative sub-fields are printed as-is** in the original output, for example
  `0  0:-1:-5`. This is the original display, not corrected.

## Host reference

- **The host build is 64-bit.** No 32-bit libraries are available on the build host.
  `long` is 64 bits and doubles use SSE. The host reference therefore demonstrates
  the same source and the same printed text, not the same arithmetic widths. The
  host build overrides `time()` to return 1793102400.

## Matrix and control

- **Rate sensitivity is tested with a deliberately wrong build.** `TMABIW` assumes
  110 ticks per second, 10% more than the i386VM timer (100/1). It must FAIL on the
  advancing i386VM image, and `TMABII` must PASS there. The natural difference between the
  i386VM and PC rates (0.015%) is too small for the five-second advance check, so it is
  not used as a control. Host arithmetic is tested in `tests/host/r11f`.

## Inherited baseline qualifications

- **Three `floatundidf` discrepancies** in `LGTEST4.aout` (the 3,576-check FPU suite)
  are retained from R11E (`docs/R11E-RUNTIME-REPORT.md`) as documented baseline qualifications; LGTEST4 reports 3,573 of 3,576 passing. They are not
  corrected here.

## Not in R11F

- No Infocom, writable FAT, multiple application slots, or broad libc completion.
- No CPU-time accounting.
- No time zone or locale support.
