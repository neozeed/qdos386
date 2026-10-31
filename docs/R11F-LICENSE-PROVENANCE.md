# R11F licence and provenance

R11F adds the components below. Everything from R11E (`docs/R11E-LICENSE-PROVENANCE.md`) is
unchanged.

## Phoon (third_party/phoon/)

- **What:** Jef Poskanzer's moon-phase program, `phoon` version as shipped in
  `third_party/phoon/orig/` (`phoon.c`, `astro.c`, `astro.h`, `date_parse.c`,
  `date_parse.h`, `phoon.1`, `README`, `Makefile`). The files are unchanged copies.
- **Licence:** the BSD-style two-clause terms printed in each source header, copyright
  1986, 1987, 1988, 1995 by Jef Poskanzer (`date_parse.c` carries the 1995 notice). The
  README gives the author's contact address. The licence text is in the source headers
  and is kept with the source.
- **Port:** `third_party/phoon/r11f-portability.patch`, applied to a copy by
  `scripts/r11f-phoon.sh`. The patch is documented in `docs/R11F-PHOON-PORT.md`.
- **Built as:** `build/apps/PHOON.aout`, with `qdos386-gcc -DOS_SYSV`. The source compiles
  and links with no warnings under the historical GCC 1.40 wrapper.

## Calendar and clock code (new in R11F)

- `kernel/clock.c`, `kernel/clock.h`, `kernel/clock_boot.c`, `kernel/clock_boot.h`: the
  wall-clock service. Written for this project. Covered by the project's own licence.
- `sdk/libc/time.c` (rewritten): calendar conversions. The days-from-civil method is
  Howard Hinnant's ("chrono-Compatible Low-Level Date Algorithms"), written out for this
  library. Its licence terms are not copied here; the code is original to this library.
- `sdk/examples/r11f/timeabi/gen_vectors.py`: expected values come from Python `datetime`
  and the host C library's `strftime`. Its output, `timeabi_vectors.h`, is committed.

## Test references (not shipped)

- Host Phoon reference: built from the patched source with `gcc -std=gnu89 -DOS_SYSV` on
  x86-64, with a `time()` shim (`phoon-r11f/host/shim.c`). Not shipped.
- PyEphem (`ephem`), used only by `logs/r11f/moon-check.py` as an independent astronomy
  reference. Not shipped; its own licence applies to it.

## External inputs (see EXTERNAL-DEPENDENCIES.txt in the package)

The Tiny386 build, BIOS and VGA BIOS, the i386VM harness, and the GCC 1.40 toolchain are
external, as in R11E. Their hashes are in the package file.
