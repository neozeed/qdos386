# R11E known limitations

Each item states what R11E does not do, or what it has not verified. Nothing here is
claimed as working unless the runtime report says it was run.

## Architecture limits (kept deliberately)

1. **One child slot.** `RUN` and `QT_IMG_LOAD` load into child slot 1 only. A program
   started with `qd_run` occupies that slot, so a program run from another program
   receives `APP_ERR_SLOTBUSY` (-28) while a child is active. The QDOSAPI test checks
   this explicitly. Consequence: the guest tests cannot start a second job from inside a
   running program; cross-job tests run from the shell instead.
2. **No dynamic address spaces, no paging, no relocation.** Every a.out is linked for its
   slot base. Pointer checks are a range check on the flat window, not a protection
   check. A job can still read and write any memory inside the user range, including
   another job's data.
3. **Single heap.** `qd_alloc` allocates from one kernel heap. There is no common/user
   split (C68 `mm_alloc` is native-only), and ownership is by job, not by region.
4. **Read-only FAT16.** `WIN1_` has no writes, no create, no delete, no rename, no
   directories below the root. Writable FAT16, FAT32 and VFAT long names are separate
   milestones.
5. **8.3 names only.** A `WIN1_` name carries at most 12 characters including the dot.
   The root directory is read through `qd_dir_open` / `qd_dir_next`; subdirectories are
   not supported.
6. **No date or calendar.** There is no RTC service in the trap ABI. `time`, `clock`,
   `gmtime`, `localtime`, `mktime`, `ctime` and `strftime` remain blocked or missing in
   the coverage table. `qd_ticks` gives the tick counter only.

## Kernel-level gaps and their effect on verification

7. **Exit status ambiguity.** `qd_job_exit_status` returns the status a job recorded with
   `QT_EXIT`, or a negative error. Statuses -2, -16 and -17 are the same numbers as
   `QD_ERR_NOJOB`, `QD_ERR_RUNNING` and `QD_ERR_NOSTATUS`. A program exiting with one of
   those values cannot be told apart from the error. No R11E test exits with them.
   A versioned status record would fix this; it was not added in R11E.
8. **`qd_job_chprio(id, 0)` deactivates a job.** An inactive job is never scheduled. A
   test that did this on itself hung on i386VM and was corrected. The header documents
   it; callers must restore a non-zero increment.
9. **`qd_alloc` returns NULL for results at or above 2 GiB.** The kernel returns a
   pointer in a signed register, so such a value is read as a negative error. Not
   reachable on the 16 MiB i386VM and 8 MiB Tiny386 configurations.
10. **Kernel self-test not observed for two R11E checks.** `tests/test_memory.c` gained
    owner-mismatch and range checks in `memory_job_phase()`. The kernel self-test image
    stops at the harness instruction budget before that phase, so those `r3_check`
    lines have **not** been observed running. The range refusal of the kernel's own
    lower bound is observed from guest code (QDOSAPI: `read into kernel memory` and
    `suspend with a kernel hold pointer`).
11. **Owner checks across jobs are not exercised in guest code.** The channel owner
    check (`CHAN_ERR_OWNER`) needs a second job holding a channel. With one child slot
    and no cross-job trap from a program, a guest test cannot create that situation.
    The check exists in the kernel; it is covered only by the kernel's own tests.
12. **Concurrent console waiters are not tested.** `qd_con_wait` returns
    `QD_ERR_CONBUSY` (-50) to a second waiter. That path needs two waiting jobs, which
    the one-slot architecture does not allow from a RUN program. Untested.

## Library behaviour that is deliberately partial

13. **`fseek` beyond end of file is refused** (returns non-zero) and leaves the position
    unchanged. C89 leaves seeking a text stream to an arbitrary offset undefined, so the
    test asserts the documented refusal, not a successful seek. `SEEK_END` is not
    supported (no file size in the kernel).
14. **`abort` does not flush stdio.** C89 requires termination, not flushing. It ends the
    job with status 3.
15. **`exit` flushes stdio and runs `atexit` functions in reverse order.** It unwinds the
    job's stack to its entry frame (see `docs/R11E-LIBQDOS-ABI.md`). The exit status is
    recorded before the unwind.
16. **Time functions:** see item 6.
17. **`fgetpos`** is partial and has no direct guest test.
18. **`tmpfile`** is blocked: it needs a writable file.

## Verification gaps

19. **Host tests run in a modified build.** `tests/host/r11e/run.sh` renames `errno`,
    `exit` and `abort` for the host build, because the host C library has symbols of the
    same names. The SDK sources are otherwise unchanged. The renaming is consistent across
    the test and the sources.
20. **`fpbig.c` assumes a 32-bit `unsigned long`.** `qd_fp_exact` fills a union with a
    `double` and reads two `unsigned long` words. On the 32-bit i386 target this is
    correct. On a 64-bit host (LP64), `w[1]` lies outside the `double`, so the host build
    reads uninitialised data (GCC reports this). The host strtod result is therefore not
    a valid check of the exact path; the guest and Tiny386 runs are the evidence for the
    target. Fixing this means changing `unsigned long w[2]` to `unsigned int w[2]`; it was
    not changed in R11E so that the verified target build stays byte-identical.
21. **Kernel warnings.** `kernel/jobs.c` has pre-existing non-volatile pointer warnings
    (`argument passing of non-volatile * pointer from volatile *`). They predate R11E and
    are not changed.
22. **QEMU run is scripted.** `scripts/r11e-qemu-smoke.sh` injects keystrokes with
    xdotool. It is not a human session. See the runtime report for what it did and did not
    show.
23. **Historical standalone matrices.** Only the integrated suite in `scripts/r11e-run-all.sh`
    (the R11D sequence plus the R11E programs) is run. Earlier standalone matrices from
    R2–R10 are not re-run in R11E; the runtime report lists which were run.
