# R11H-FIX1 Phoon 2106 boundary (Phase 3)

## Result

No defect in the current QDOS build. The 2106 behaviour matches the designed range, and the earlier
"discrepancy" came from stale reference logs. No code change to Phoon was made.

Supported range: `time_t` is unsigned 32-bit (`sdk/include/time.h`). `QD_TMAX = 0xFFFFFFFE`
(`sdk/libc/time.c`) is the largest valid value. Independently computed:

| second | epoch value | UTC | outcome on current QDOS |
|---|---|---|---|
| 7 Feb 2106 06:28:13 | 4294967293 | 2106-02-07T06:28:13Z | accepted |
| 7 Feb 2106 06:28:14 | 0xFFFFFFFE | 2106-02-07T06:28:14Z | accepted (the maximum) |
| 7 Feb 2106 06:28:15 | 0xFFFFFFFF | 2106-02-07T06:28:15Z | refused (-1 sentinel) |
| 1 Mar 2100 12:00:00 | | | accepted |

## Same binary, same outcome

- PHOON.AOU is identical in the R11F, R11G and R11H volumes: sha256 `df2a4f1eb7b4bdd642c00b4ad1df29b7ac6e478f8f1c6cb3d439df969bb74fdf`.
- The PHOON.AOU in each volume was extracted with a FAT16 reader and hashed.

## What the earlier logs were

- `logs/r11f/phoon-battery-target.txt` has the banner `QDOS/386 R7`. It refuses 06:28:14 and 06:28:15. It was
  produced by the R7 build, before the R11F unsigned `time_t` correction, so it is not a record of the R11F or R11H
  behaviour.
- `logs/r11f/phoon-battery-host*.txt` refuse 06:28:14 as well. The original, unpatched Phoon compiled on this
  64-bit host accepts all four inputs (rc 0). The refusing host run therefore used a different configuration that
  was not recorded (it behaves like a signed 32-bit `time_t`, where 2106 values are negative).
- `docs/R11F-PHOON-PORT.md` (lines 95 to 102) says `7 Feb 2106 06:28:13` and `06:28:14` are inside the supported
  range. That matches the current behaviour and the designed range.

## Current evidence

Fixture run (`tests/fix1/phoon-2106-seq-used.txt`), scripted Tiny386 with the shipped emulator, on the R11F PC images
(`pc-r11f-none`, `pc-r11f-pin`) with the R11F volume:

- `docs/evidence/fix1-phoon-2106-r11f-none-tiny386.txt` and `...-r11f-pin-tiny386.txt`:
  06:28:13 exit 0, 06:28:14 exit 0, 06:28:15 `illegal date/time` exit 1, 2100 exit 0.

The R11H pinned battery (`scripts/`, transcripts under scratchpad r11h/pin) shows the same 06:28:14 accept and 06:28:15
refuse on both emulators. Those transcripts do not contain a 06:28:13 case.

`scripts/r11h-fix1-phoon-check.py` checks a transcript against `tests/fix1/phoon-2106-expect.txt`. The fixture run passes.

## Regression fixture

- `tests/fix1/phoon-2106-input.txt`: the four inputs, in order.
- `tests/fix1/phoon-2106-expect.txt`: expected exit status per input, with the range documented.

## Not done

- The 2038 signed boundary (03:14:07 and 03:14:08) is covered by the R11F battery, not re-run in FIX1.
- No 2101 leap-day or astronomy change; those are out of scope.
