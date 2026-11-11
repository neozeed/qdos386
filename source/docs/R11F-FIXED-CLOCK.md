# R11F fixed-clock setup

The wall clock is off in the default kernel build. To run Phoon with a date
that is not the default, or to run the time tests with known answers, build
the kernel with one of the clock variants. The variants are described in
`docs/R11F-TIME-ABI.md`, section 4.

## Variants

| Name | Kernel flags | Clock |
|---|---|---|
| none | (none) | Unavailable. `time()` returns `(time_t) -1`. Regression image. |
| pin | `-DQDOS_CLOCK_EPOCH=1793102400 -DQDOS_CLOCK_PINNED` | Fixed at 2026-10-27 12:00:00 UTC. |
| adv | `-DQDOS_CLOCK_EPOCH=946684800` | Starts at 2000-01-01 00:00:00 UTC and advances with ticks. |

The pinned value is the same instant used by the Phoon battery and the
`TMABIP` test. The advancing value is used by the `TMABIA` and `TMABII`
tests.

## Building the images

The scripts assume the historical toolchain tree (`I386VM`) and the R9 shell
flags (`-DR7_SHELL_MODE -DQDOS_LAYOUT_R9`). They are the same flags the
R11E images use.

    QDOS386_TOOLS=<tree> I386VM=<same tree> sh scripts/r11f-images.sh

This builds three i386VM images and three PC images:

    build/i386vm-r11f-none/qdos386-r11f-none.bin
    build/i386vm-r11f-pin/qdos386-r11f-pin.bin
    build/i386vm-r11f-adv/qdos386-r11f-adv.bin
    build/pc-r11f-none/r11f-none.img
    build/pc-r11f-pin/r11f-pin.img
    build/pc-r11f-adv/r11f-adv.img

The i386VM images embed `build/fat16/win1-r11f.img`. If the volume changes,
rebuild the images. `scripts/r11f-volume.sh` builds the volume and must run
first.

## Running the fixed-clock tests

Tiny386 runs the PC images. The volume is the second hard disk.

    tests/interactive/run_pc_multi.py <tiny386-binary> <workdir> build/fat16/win1-r11f.img <log> <commands...>

The pinned sequence is `tests/r11f/seq-pinned.txt`. The advancing sequences
are `tests/r11f/seq-adv-pc.txt` (PC) and `tests/r11f/seq-adv-i386.txt`
(i386VM). `scripts/r11f-run-all.sh` runs all of them, with the environment
variables it documents, and writes logs to `logs/r11f/`.

## Launch examples on the pinned image

Run Phoon with no arguments, which uses the pinned clock:

    RUN PHOON.AOU

Print ten lines of phase times (`-l` sets the number of lines):

    RUN PHOON.AOU -l 10

Give an explicit date (quote it, since it contains spaces):

    RUN PHOON.AOU "11 Jan 2024 11:57:00"

Check the pinned clock directly:

    RUN TMABIP.AOU

## Notes

- A default kernel with no clock refuses Phoon's time-only arguments with an
  explicit message. Give a date to run Phoon on that image.
- The pinned clock never advances. Repeated runs print the same "now".
- Do not run `TMABIN` or `TMABIA` on the pinned image. They expect the
  matching clock mode. They fail there, and those failures are expected.
- A kernel built with `QDOS_NO_TIMER` is always pinned, whatever the flags.
