# R11F runtime report

Branch `r11f`, created from the frozen R11E commit `7a62af1`. The source revision this report
describes is the commit recorded in `PACKAGE.md` of the delivered package (see the end of
this report). Every test run here used the same working tree as that commit, except this
report and the packaging metadata, which were written after the runs.

All runs were scripted. Keystrokes into Tiny386 are typed by `tests/interactive/run_pc_multi.py`;
input to i386VM is piped from a file; QEMU keystrokes are injected with `xdotool`. None of
the runs was operated by a person.

## 1. Artefacts tested

| Artefact | Built by | Role |
|---|---|---|
| `build/fat16/win1-r11f.img` | `scripts/r11f-volume.sh` | production data volume (46 files), sha256 `d319f3214efea391bc9c85d9048dac0bc5e77c42eeaf2d492fb9a1e820564375` |
| `build/fat16/win1-r11f-control.img` | same | test-only control volume: the production volume plus `TMABIW.AOU` (47 files), sha256 `7131170419b92543f0f53acb6378492b3ce9f7eaebe80cfc8583f8483da93892` |
| `build/i386vm-r11f-{none,pin,adv}/*.bin` | `scripts/r11f-images.sh` | i386VM kernels, production (no clock / pinned / advancing), embedding the production volume |
| `build/i386vm-r11f-control/qdos386-r11f-control.bin` | same | test-only i386VM control kernel, embedding the control volume |
| `build/pc-r11f-{none,pin,adv}/r11f-*.img` | same | Tiny386 kernel images for the three clock modes |
| `build/pc-r11f-vga/r9shell-vga.img` | `make pc` (VGA, pinned) | QEMU VGA kernel |
| `build/apps/PHOON.aout` | `scripts/r11f-phoon.sh` | Phoon, original source plus `third_party/phoon/r11f-portability.patch` |
| `build/r11f/PHDATE.aout`, `PHCHK.aout`, `TMABIN/P/A/II.aout` | `scripts/r11f-volume.sh` | probes and clock tests |
| `build/r11f/TMABIW.aout` | same | control only; in the control volume, never in production |

The SHA-256 of every artefact used is in `logs/r11f/artefact-hashes.txt`, and the final
values are in `PACKAGE.md`.

Clock modes: `none` (no clock; `time()` returns -1), `pin` (fixed at 1793102400 = 2026-10-27
12:00:00 UTC), `adv` (advances from 946684800 = 2000-01-01 00:00:00 UTC).

## 2. Integrated matrix (three rounds, final layout)

Command: `ROUNDS=3 sh scripts/r11f-run-all.sh`, log `logs/r11f/matrix-final.txt`, per-run logs
`logs/r11f/{tiny386,i386vm}-<key>-round<N>.txt`.

| Key | Platform | Checks per round | FAIL checks | Banners (PASS / FAIL) |
|---|---|---|---|---|
| regression (R11E sequence + R11F no-clock checks) | Tiny386 | 603 | 0 | 28 / 0 (x3 rounds) |
| regression | i386VM | 603 | 0 | 28 / 0 (x3) |
| pinned (battery, probe, loops) | Tiny386 | 100 | 0 | 16 / 0 (x3) |
| pinned | i386VM | 100 | 0 | 16 / 0 (x3) |
| advancing (TMABIA, PC rate) | Tiny386 | 76 | 0 | 5 / 0 (x3) |
| advancing (TMABII, 100/1) | i386VM | 76 | 0 | 5 / 0 (x3) |
| control: TMABII (must PASS) | i386VM | 35 | 0 | 1 / 0 |
| control: TMABIW (must FAIL) | i386VM | 33 ok, 2 FAIL | 2 | 0 / 1 |

Every regression, pinned and advancing run passed in all three rounds on both platforms.

The FAT16 production volume hash was `d319f3214efea391bc9c85d9048dac0bc5e77c42eeaf2d492fb9a1e820564375`
before and after the matrix, and the matrix log says it was unchanged.

### Pinned-clock detail (per platform, per round)

- 62 Phoon runs: 24 battery cases plus 5 post-2038 battery cases and 2 refusals, the default,
  the `-l` and `-x` options, and 12 repeated loops of `PHOON`, `PHOON -l 5 "11 Jan 2024 11:57:00"`
  and `PHCHK`.
- 100 exit-status-0 runs and 10 exit-status-1 runs. All ten exit-1 runs are intended refusals:
  `-l`, `-x`, `"0"`, `"-5"`, `"abc"`, `"1970-01-01"` (ISO dates are not accepted by the original
  parser), `"31 Dec 1969 23:59:59"`, `"7 Feb 2106 06:28:15"`, and `"8 Feb 2106 00:00:00"`.
- PHCHK passed 14 times on each platform. Its heap-invariant check was 1 every time.
- Free heap was constant across all repeats on each platform: 906064 bytes on Tiny386 and
  1955872 bytes on i386VM. The two differ because the platforms have different memory
  configurations, not because of a leak.

## 3. Wrong-rate control

`TMABII` (the i386VM rate, 100 ticks per second) passes all 35 checks on the advancing i386VM
image. `TMABIW` is the same binary with `TIMEABI_RATE_NUM=110`, so it assumes 10% more ticks per
second. It fails on the same image:

- `advancing clock: starts at the configured epoch, bracketed by ticks: FAIL`
- `advancing clock: wall minus epoch matches the tick count (bracketed): FAIL`

The other two advance checks pass: "about 5 seconds elapse" (its tolerance is wide) and
`time(NULL)` agreeing with `qd_wall_time`. The control binary and the normal binary differ only
in the rate constant, so the failure is attributable to the assumed rate and the measured
elapsed time, not to a timeout, an emulator crash, or an unrelated error. The control is in the
control volume and the control i386VM image only, never in a production image.

The natural 0.015% difference between the i386VM and PC rates is below what this check resolves,
so the control is not based on it.

## 4. Phoon

### 4.1 Leap-day correction (2101)

The original `tm_to_time` counted 2100 as a leap year, so dates from 1 January 2101 were one day
late. The correction is in `third_party/phoon/r11f-leapday-2101.diff` and in the combined patch
`third_party/phoon/r11f-portability.patch`. The original sources are unchanged in
`third_party/phoon/orig/`. See `docs/R11F-PHOON-PORT.md`.

### 4.2 Independent date checks (PHDATE)

`sdk/examples/r11f/phdate/phdate.c` calls Phoon's `date_parse` directly. Expected values come
from Python `calendar.timegm`, and GNU `date -u` agrees with them for the 2100 and 2101 boundary
values checked. The 32 cases cover 28 Feb / 1 Mar 2100, 31 Dec 2100 / 1 Jan 2101, the 2104 and
2105 leap years, the post-2038 battery dates, the limit 7 Feb 2106 06:28:14, the epoch, and dates
just outside the range.

| Build | Result |
|---|---|
| Host, corrected source | 32 of 32 match |
| Host, pre-fix source | 13 of 32 differ, all from 1 Jan 2101 on, each one day late; also refuses 7 Feb 2106 06:28:13 and :14 |
| Tiny386, pinned, 3 rounds | 31 unique inputs, 0 mismatches, every round |
| i386VM, pinned, 3 rounds | 31 unique inputs, 0 mismatches, every round |

The 32nd line of the expected file repeats `1 Jan 2101 12:00:00`, which is in both the
2101 group and the battery group. The regression logs run only two probe cases, and those
two match.

### 4.3 Astronomy battery (31 cases)

`tests/r11f/phoon-battery.txt`. Host reference: the same patched source built with
`gcc -std=gnu89 -DOS_SYSV` and a `time()` shim fixed at 1793102400.

| Comparison | Result |
|---|---|
| Tiny386 pinned, target vs host, 3 rounds | 33 target runs per round compared; 0 differ |
| i386VM pinned, target vs host, 3 rounds | 33 target runs per round compared; 0 differ |

Output text and exit status are identical between the two targets and the host for every case.

PyEphem comparison (`logs/r11f/moon-check.py`), 38 events, identical on target and host:

- maximum error +305 s, median 146 s, all positive (Phoon's model runs late)
- 2101 cases: +95 s and +247 s, inside the same range as the other dates
- the largest residuals are at the far end of the range (2100 and 2106), where the original
  model is least accurate

These are model differences. The expected values were not adjusted to match PyEphem.

### 4.4 Supported range

Accepted: 1970-01-01 00:00:00 UTC (0) to 2106-02-07 06:28:14 UTC (4294967294), exact.
Refused with `illegal date/time` and exit status 1: `7 Feb 2106 06:28:15`, `8 Feb 2106 00:00:00`,
`31 Dec 1969 23:59:59`, and earlier dates. The refusals were checked on the target and the host.

## 5. QEMU VGA smoke test

Scripted by `scripts/r11f-qemu-smoke.sh` on Xvfb `:99`, with the pinned VGA kernel
(`build/pc-r11f-vga/r9shell-vga.img`) and the production volume. Keystrokes were injected with
`xdotool` (scripted, not human-operated). Commands: `dir`, `run tmabip.aou`, `run phoon.aou`,
`run phoon.aou "11 Jan 2024 11:57:00"`, `run phchk.aou`.

Final run: `logs/r11f/qemu-smoke-final.txt` (summary), `logs/r11f/qemu-smoke-final/`
(`debug-0x402.txt` machine-readable console log, `qemu-r11f-scripted.png` screen capture,
`qemu-stderr.txt`).

- The graphical shell booted from the VGA kernel. The production volume hash was
  `d319f3214efea391bc9c85d9048dac0bc5e77c42eeaf2d492fb9a1e820564375` before and after, and
  the run log says it was unchanged.
- `run tmabip.aou`: `QDOS/386 R11F TIMEABI PASS`.
- `run phoon.aou` (default, pinned clock): the moon artwork is drawn, with the phase lines
  `Full Moon +  1 7:46:52` and `Last Quarter -  5 8:30:02`, exit status 0. The artwork is in
  `debug-0x402.txt`. In the screen capture it has scrolled above the top of the display, so the
  capture shows the text panel of the later run.
- `run phoon.aou "11 Jan 2024 11:57:00"`: `New Moon +  0 0:-1:-5`, `First Quarter -  6 15:56:56`,
  exit status 0.
- `run phchk.aou`: `PHCHK: 2 checks passed, 0 failed`, `QDOS/386 R11F PHCHK PASS`, exit status 0.
- The shell returned to its `>` prompt after every command.

Scripted keystrokes were injected with `xdotool` into the QEMU window. This is a scripted
run, not a human-operated session. The earlier QEMU runs (`logs/r11f/qemu-smoke/`, and
`logs/r11f/qemu-smoke-rc/`, which used an intermediate volume) are superseded by this one.

## 6. Clock(), calendar coverage and the C89 coverage table

- `clock()` returns `(clock_t) -1`. It is not implemented from the wall clock or the tick
  count (see `docs/R11F-TIME-ABI.md`).
- `docs/R11F-C89-COVERAGE.{csv,md}` is generated by `scripts/r11f-coverage.py` from the built
  libraries. `time`, `gmtime`, `localtime`, `mktime`, `ctime` and `strftime` are "partial" (UTC
  only, range 1970 to 2106, static return buffers). `clock` is "blocked" by design. `difftime`,
  `asctime` are "implemented". Calendar coverage is therefore partial, not complete.

## 7. Reproducing the results

    export QDOS386_TOOLS=<historical GCC 1.40 tree> I386VM=<same tree> TINY386=<tiny386_headless> \
           FW=<dir with bios.bin, vgabios.bin> I386VM_HARNESS=<i386vm-r9 binary>
    ROUNDS=3 sh scripts/r11f-run-all.sh            # volume, images, full matrix
    python3 logs/r11f/battery-compare.py logs/r11f/tiny386-pinned-round1.txt \
            logs/r11f/phoon-battery-host-31.txt tests/r11f/phoon-battery.txt
    python3 logs/r11f/phdate-compare.py sdk/examples/r11f/phdate/phdate-expected.txt \
            logs/r11f/tiny386-pinned-round1.txt
    DISPLAY=:99 FW=<dir> sh scripts/r11f-qemu-smoke.sh <outdir>
    sh scripts/make-r11f-package.sh <outdir>

The host-side artefacts (`logs/r11f/phoon-battery-host-31.txt`, `moon-check` output) need the
host Phoon build described above, and PyEphem for `moon-check.py`. Neither ships with the
package.

## 8. Limitations

See `docs/R11F-KNOWN-LIMITATIONS.md`. In summary:

- UTC only; time_t range 1970 to 2106; `clock()` unsupported.
- The i386VM clock is nominal (its timer is step-based), not wall time.
- The host reference is 64-bit (no 32-bit libraries on the build host).
- Phoon's astronomy differs from PyEphem by up to +305 s, median 146 s. This is the original model.
- Three `floatundidf` discrepancies in LGTEST4 are retained from R11E as documented baseline qualifications.
- The 2^32 tick wrap is tested on the host only.

## 9. Source revision

The commit is recorded in `PACKAGE.md` in the delivered package and in the delivery message.
