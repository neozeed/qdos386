# R13B wall clock

R13B keeps the R11F model (`docs/R11F-TIME-ABI.md`): wall time = anchor + elapsed timer ticks, exact integer
accumulation, states NONE / RUNNING / PINNED / OVERFLOW, trap `QT_WALL_TIME` (0x23), libc `time()`, `gmtime()`,
`ctime()`, `strftime()` unchanged, `0xFFFFFFFF` = unavailable, valid times 0 .. 0xFFFFFFFE (2106-02-07 06:28:14 UTC).
What changes is **where the anchor comes from** on R13B kernels.

## 1. Sources, in order

| Build | Anchor | Boot line |
|---|---|---|
| `-DQDOS_CLOCK_EPOCH=<s>` (test kernels, R11F) | the build-time value | `CLOCK: build-time epoch (test kernel)` |
| R13B kernel (`-DQDOS_R13B`) | the **CMOS RTC**, read once at boot | `CLOCK: CMOS RTC (UTC) read at boot [cc yy mm dd hh mi ss B = …], <s> s since 1970` |
| R13B kernel, RTC unusable | none: the clock stays NONE | `CLOCK: unavailable - <reason> [raw registers]` |
| `-DQDOS_NO_RTC`, or kernels before R13B | none | (R13A and older print no CLOCK line) |

The anchor is taken at boot, before the timer starts; from then on the clock advances with the PIT ticks
(1193182/11932 Hz). **Scheduler ticks** (`sched_ticks`, `qd_ticks()`) are a separate monotonic count since boot and are
never a time of day; the RTC is not read again (no drift correction: QEMU's PIT and RTC both follow the host clock).

## 2. Reading the RTC (`kernel/rtc.c`)

* Ports 0x70/0x71 (index with bit 7 clear: NMI stays enabled as elsewhere in the kernel).
* Wait while register A bit 7 (update in progress) is set (bounded), read seconds, minutes, hours, day of month,
  month, year, century (0x32), registers B and D; repeat until two consecutive reads agree (at most 8 reads), else
  `RTC never read the same twice`.
* Register D bit 7 clear, or all registers 0xFF (no device): `no RTC`.
* Register B bit 2: binary (1) or **BCD** (0, the PC default); a BCD nibble above 9 is refused.
* Register B bit 1: 24-hour (1) or 12-hour (0: hour bit 7 = PM, 12 AM = 0 h, 12 PM = 12 h; hour 0 or > 12 refused).
* Ranges: second 0–59, minute 0–59, hour 0–23, month 1–12, day 1–(days of that month; 29 February only in leap years
  — 2100 is not one).
* **Century**: register 0x32 must be 19, 20 or 21 (BCD or binary as the rest); otherwise `RTC has no usable century
  byte`: a two-digit year is never completed by guessing.
* The date must be representable: 1970-01-01 00:00:00 .. 2106-02-07 06:28:14 UTC, else `RTC date outside …`.
* The RTC is taken to keep **UTC** (QEMU's default and Tiny386's behaviour); there is no time zone (R11F UTC policy).

## 3. Shell

`DATE` → `DATE: Thu 2026-10-29 (UTC)`; `TIME` → `TIME: 12:00:05 UTC (1793275205 s since 1970)`. Display only (no
setting). Without a clock: `DATE: unavailable - this machine has no wall clock configured` (same for `TIME`). Phoon
without a date works when the clock is set and keeps its R11F message ("no clock is configured; give a date") when it is
not; Phoon itself is unchanged.

## 4. Emulators

* **QEMU** `-rtc base=…`: correct for dates up to 2038 and some beyond (2050 tested), but a base of 2100-02-28 or
  2106-02-07 comes out wrapped (the RTC showed 1964-01-23 and 1969-12-31: a 32-bit time somewhere in QEMU's RTC
  path). The kernel refuses those values as before 1970 — the clock stays unavailable rather than wrong. Dates after
  2038 are therefore tested with build-time epoch kernels (`make r13b-epoch-kernels`: 2100-02-28 23:59:57,
  2100-12-31 23:59:57, 2106-02-07 06:28:00), which exercise the same DATE/TIME/libc/overflow paths.
* **Tiny386**: CMOS time from the host's UTC clock in BCD with century 0x32, register B 0x02 — read the same way.

## 5. Tests

* Host: `tests/r13b/host-rtc.c` decodes 26 register sets from `tests/r13b/rtc_vectors.h`, written by
  `tests/r13b/gen-rtc-vectors.py` with Python `calendar.timegm` (BCD/binary, 12/24 h, leap days, 2038, 2100, 2101,
  2106 last second and one past it, 1969, every refusal). The R11F clock host tests run unchanged.
* QEMU Gate 4 (`tests/r13b/run-gate.sh 4`): RTC anchor vs the `-rtc base`, DATE/TIME vs libc (TIMEQ) vs the dynamic
  shell, ticks vs seconds, no-argument PHOON vs the host oracle (`tests/r13b/phoon-host.sh`: the R11F patched source
  with a `time()` shim) at the guest's instant, the 2100/2101/2106 epoch kernels, a refused RTC value, and the
  clockless R13A kernel.
