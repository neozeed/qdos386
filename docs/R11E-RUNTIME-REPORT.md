# R11E runtime report

Base: R11D commit `2d409fb` on branch `r11e`. Everything below was run in this
session; the logs are under `logs/r11e/`. All keystrokes were scripted (piped input on
i386VM, PS/2 key scripts on Tiny386, xdotool on QEMU). No human-operated session was
recorded.

## What was run

| Target | What | Rounds | Result |
|---|---|---|---|
| i386VM (`i386vm-r9` harness, R9 layout) | `scripts/r11e-run-all.sh` sequence: R11D regression list + R11E programs | 3 | 3 of 3 match the expected results (see below) |
| Tiny386 (`tiny386_headless`, FPU config per R10) | same sequence, `tests/interactive/run_pc_multi.py` | 3 | 3 of 3 match |
| QEMU VGA (`qemu-system-i386`, Xvfb, xdotool) | `scripts/r11e-qemu-smoke.sh` on the R11E VGA image | 1 | QDOSAPI, C89EDGE, STREAM PASS; CHILD2 exits with 9 and the shell recovers |
| Host C89 (host gcc, strict C89) | `tests/host/r11e/run.sh` | 1 | 62 of 62 checks |

Each matrix round prints 567 `CHECK ... ok` lines and 26 PASS banners. The only FAIL
lines are the three documented `floatundidf` discrepancies in LGTEST4 and the summary
line that reports them.

## Results per program (each round, both targets)

| Program | Checks | Result |
|---|---|---|
| C89COR | 83 | PASS |
| JMPTST | 19 | PASS |
| SCANTS | 33 | PASS |
| MATHX | 24 | PASS |
| HELPER / FPARSE / FMATH / FPRINT / FPCOMB / FPMULT | 30 / 8 / 12 / 9 / 13 / – | PASS |
| FPJOB | – | PASS |
| STREAM (R11E extended) | 50 | PASS |
| FORMAT | 32 | PASS |
| LIBQDO | 22 | PASS |
| REPEAT (R11E extended, run twice) | 7 each | PASS |
| CONSOL | 10 | PASS |
| CTYPE / STRINGS / FILETEST | 46 / 37 / 41 | PASS |
| LIBHELLO / FILECAT / MEMSTRESS / HEAPEX | – | PASS |
| EXITFL | – | exit status 0 |
| LGTEST4 | 3,576 | 3,573 pass; 3 known `floatundidf` discrepancies (unchanged, not corrected) |
| QDOSAPI (R11E) | 56 | PASS |
| PS (R11E) | – | lists 4 jobs (slots 0–3), marks its own job as `self` |
| C89EDGE (R11E) | 28 | PASS |
| CHILD1 `alpha` | – | argc 2, exit status 7 |
| CHILD2 | – | `CHILD2 unflushed`, exit status 9 (flushed by exit) |
| EXITX | – | exit status 5 |
| EXITR | – | exit status 6 |
| CONWAIT, after `Z` | – | `CONWAIT got 90`, exit status 90 |

The QEMU smoke run matched the i386VM and Tiny386 results for QDOSAPI, C89EDGE and STREAM.

## FAT16 volume

`build/fat16/win1-r11e.img` sha256 `b36a52aee33facf916136e78f70181db402a9046bd6672017f914c9480acb05c`
before and after every matrix round, and before and after the QEMU run. Unchanged.

## Defects found and fixed in this milestone

1. **`exit()` never ended the job.** `qd_exit` recorded the status and then suspended
   forever, so the shell waited indefinitely (instruction limit on i386VM). Fixed:
   crt0 records its entry frame, and `exit` unwinds to it (`qd_end_job`). Verified by
   CHILD2 on all runs.
2. **Kernel address refused only on the wrong test address.** The QDOSAPI checks used
   `0x00100000`, which is a heap address on i386VM (kernel at 0, heap to `0x200000`).
   The checks now use `0x1000`, below the arena base on both targets. Both checks and the
   run-argument check pass. The run-argument check now asserts `-14` exactly rather than
   any negative value.
3. **STREAM `fseek` past end asserted a success that C89 does not require.** The check
   now asserts the documented refusal and that the stream keeps its position.
4. **Lower bound of user pointers missing.** `chan_user_range_ok` accepted kernel
   addresses below the heap. Fixed with the heap arena base. Observed from guest code
   through item 2.

## Not verified

- **Kernel self-test owner and range checks** added to `tests/test_memory.c`
  (`memory_job_phase`) are **not observed**. The kernel self-test image stops at the
  harness instruction budget before that phase. The range refusal is observed through the
  guest tests instead. Owner checks across jobs are not exercised at all (see
  `docs/R11E-KNOWN-LIMITATIONS.md`, items 10–12).
- Concurrent console waiters and cross-job channel owner checks: not tested.
- Historical standalone matrices R2–R10 were not re-run separately. The integrated
  R11D regression list is run; it contains the R2–R11D programs that the R11D run list
  named.
- The host unit tests exercise the SDK's portable routines with `errno`, `exit` and
  `abort` renamed for the host build. A 64-bit host reads uninitialised data in
  `fpbig.c` (`qd_fp_exact`). The target is 32-bit and correct; see item 20 of the
  known-limitations document.

## Build hashes

From `logs/r11e/image-hashes.txt`:

- `build/i386vm-r11e/qdos386-r11e-shell.bin` `e45d0377…`
- `build/fat16/win1-r11e.img` `b36a52ae…`
- `build/pc-r11e-vga/r9shell-vga.img` `9eb7d776…`
- `sdk/lib/libqdos.a` `ca8155d6…`
- `sdk/lib/libc.a` `43fe06b7…`

Note: the QEMU smoke test first ran against the R10 VGA image. Its failures were the
old kernel (no `QT_JOB_SELF`, 10-character names), not R11E behaviour. The R11E VGA image
was built with `QDOS_CON_VGA` from the R11E kernel and the smoke test was rerun against it.
