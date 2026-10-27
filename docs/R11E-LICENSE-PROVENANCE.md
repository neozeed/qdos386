# R11E licence and provenance notes

This is private development and test material. Historical GNU, C68 libc and other
third-party copyright and licence notices in the existing tree are kept unchanged, and
R11D and earlier checkpoints are not modified. Redistribution terms for any third-party
file must be checked per file; none is asserted here. Public redistribution review is a
separate checkpoint.

## Third-party material and how R11E uses it

- **C68 libc 4.24.5 (`qdos/` reference).** Used for names, prototypes and documented
  behaviour only. No C68 object or source is linked into any R11E binary. The
  cross-reference generator reads its prototypes.
- **GCC 1.40, GAS 1.38, i386 binutils.** The historical toolchain that builds every SDK
  program, unchanged.
- **GNU host compiler (for the host unit tests only).** `tests/host/r11e/run.sh` uses the
  host `gcc` and host C library for the test driver. Those are not part of the target
  system.

## New in R11E (written for this project)

- Library: `sdk/lib/qdos.c` (additions: `qd_self` through `qd_dir_next`, `qd_exit`
  unwind), `sdk/lib/qtrap.s` (`_qd_entry_frame`, `_qd_end_job`), `sdk/lib/crt0.s` and
  `sdk/lib/crt0_libc.s` (one line each: the entry frame).
- Headers: `sdk/include/qdos386.h` (new trap numbers, error codes, wrapper declarations),
  `sdk/include/stdio.h` (`FILENAME_MAX`).
- Kernel: `kernel/jobs.h` and `kernel/jobs.c` (`QT_JOB_SELF`, `QT_JOB_SLOT`, hold-pointer
  checks), `kernel/channel.c` and `kernel/channel.h` (lower bound, `CHAN_NAME_MAX`),
  `kernel/memory.c` and `kernel/memory.h` (`mem_arena_base`), `kernel/imgload.c`
  (`IMG_NAME_MAX`), `tests/test_memory.c`
  (owner and range checks; see the known-limitations note on their execution).
- Shell: `apps/shell/shell.c` (name limits).
- Examples: `sdk/examples/r11e/` (QDOSAPI, PS, C89EDGE, STREAM, REPEAT, CHILD1, CHILD2,
  CONWAIT, EXITX, EXITR, `check.h`), and `sdk/examples/Makefile` rules for them.
- Host tests: `tests/host/r11e/host_test.c`, `tests/host/r11e/run.sh`.
- Tools: `scripts/r11e-volume.sh`, `scripts/r11e-run-all.sh`, `scripts/r11e-qemu-smoke.sh`,
  `scripts/r11e-coverage.py` (derived from `scripts/r11d-coverage.py`),
  `scripts/r11e-c68-xref.py`.
- Documentation: `docs/R11E-*.md` and `docs/R11E-*.csv`.

## Unchanged

The R11D and earlier checkpoints, LGTEST4 and its recorded hash, the FPJOB fixture and
its hash, and the three known GCC `floatundidf` discrepancies (see the runtime report).
