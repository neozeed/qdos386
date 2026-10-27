# QDOS/386 R8 — runtime report

This report records what R8 was run on, the commands, the results, and what was
not tested. Two labels are used throughout:

- **SCRIPTED** — bytes are piped to the i386VM harness (`session.py --emu i386vm`).
  The keyboard path is not used.
- **PS/2 via -term** — keys are driven by `tests/interactive/session.py` through
  the Tiny386 PS/2 keyboard path (`--emu pc`). This is scripted input. It is not
  human keyboard testing, and no human keyboard test was performed for R8.

## Build

The toolchain is the historical GCC 1.40 / GAS 1.38 / i386-ld tree, exported as
`I386VM`. Nothing in it was changed for R8.

    export I386VM=<i386vm tree>            # bin/i386-gcc, as, i386-ld
    make i386vm pc apps fat16 r8-fat16 sdk-examples i386vm-r8-shell pc-r8-shell i386vm-r8-shell-notimer

The `make` step builds the kernel, the R2–R7 images, the R8 applications
(`args.aout`, `large.aout`, the negative fixtures) and the SDK samples. It
prints compiler warnings from `kernel/jobs.c` (passing a `volatile` pointer to
`run_tasks`, lines 207 and 226). They were present in this build, do not affect
the result, and were not investigated further.

## Harnesses, firmware and images

Harness and firmware hashes come from `logs/r8/matrix/images.sha256`:

| Item | SHA-256 |
|---|---|
| `scratchpad/firmware-known-good/bios.bin` (SeaBIOS, known good) | `7bd968419c81fcf8262c5c1182cef43bb0d5eb70f375f94567ae71fd64a15e9e` |
| `scratchpad/firmware-known-good/vgabios.bin` | `f5aedb52cde5c88d85e5b304a4cc48459c231aef42b9950ccc82bd321d9c127e` |
| i386VM harness used by the matrix (`x/i386vm-r7/bin/i386vm`, patched input-status register 0x800004) | `2d2f8beba177592ed5485c60b3ca32e201e7ebec339e10e7756acf9498ad0bc1` |
| Tiny386 headless (`tiny-build/tiny386_headless`) | `fb63b4057e3a62f5fadd3d532e9c553a0d8ccecd5a9a32a7523b1f228706a9fc` |
| `build/fat16/win1.img` (R7 volume, unchanged) | `2dab8fd553117fdcf597d905cf385522ccc9185a75a953a2d543edd4220cbef0` |
| `build/fat16/win1-r8.img` (R8 volume) | `dfbcf562315f8c22868bf1174b008c52c05a85a63323a1681a52341f8a0ec732` |
| `build/i386vm-r8/qdos386-r8-shell.bin` | `b668798e78ac8085fa4bc1290fca5c1f537a99ad2e5c8887c47badd8a98ef7dd` |
| `build/pc-r8/r8shell.img` | `0260a8abc6aeec9fe419a7157fd4f1becd5af8705e7724b571f5102f1ecfea43` |
| `build/i386vm-r8-notimer/qdos386-r8-shell-notimer.bin` (timer-off negative) | `045f3652611c9f935ab6d67de9e55cba63f96412bb5e1f3cd0fde17bbc84b8ab` |
| `build/apps/args.aout` | `766ee2d8f5b20b38891bd4a413ef3feb10ae5608360f23f63ea294437f2c9a5b` |
| `build/apps/large.aout` | `b1a8c4a663ee5ca8fc60a9f20daf4940aa2344f2c4a5aa9d9465d16ca331b571` |
| `build/apps/shell.aout` | `6abde9e717daa68d86a0d9cd6253d07444f41169f4879ea98549b2af7fe2b4c6` |

Harness status, per `patches/HASHES.txt` and `patches/`:

- The i386VM used by the matrix is the stock i386VM with
  `patches/i386vm-input-status.patch` applied (an R7 addition that makes
  scripted input readable on register 0x800004). It is built in a copy and is
  not part of the OS source.
- `patches/i386vm-synthetic-timer.patch` is the R1 bootstrap extension (IRQ0
  driven by the executed-step count, port 0x800003). Every phase from R1 onward
  uses it, including R8. It is not new in R8.
- Tiny386 (`tiny386_headless`) is unmodified.
- The firmware is the known-good SeaBIOS-compatible `bios.bin` and
  `vgabios.bin` used by `run.ini`. Local SeaBIOS source changes from an earlier
  phase are recorded in `external/seabios-local-changes.diff`; they were not
  rebuilt for R8.

**FAT16 image hashes.** The R8 and R7 volume hashes were recorded
before and after the matrix (`logs/r8/fat16-sha256-before-matrix.txt`,
`logs/r8/fat16-sha256-after-matrix.txt`). The "before" value was taken as the
matrix started, after its first build step, so it is not a pre-build
baseline. Both values are identical after the run, so the matrix did not
modify either volume.

## Commands

    ROUNDS=3 sh scripts/r8-matrix.sh > logs/r8-matrix.txt

Each round runs:

1. i386VM test suite on the R7 volume (R2–R8 kernel tests), scripted pipe.
2. Tiny386 test suite on the R7 volume.
3. i386VM R8 shell session on the R8 volume — **SCRIPTED**.
4. Tiny386 R8 shell session on the R8 volume — **PS/2 via -term**.
5. i386VM R8 EOF session — **SCRIPTED**.

After the rounds: the timer-off negative (the R8 shell image with the timer
never started, `DIR` piped in) and the image hashes.

## Results: three rounds on both targets (final clean PC object tree)

| Check | Round 1 | Round 2 | Round 3 |
|---|---|---|---|
| i386VM suite | PASS: 204 CHECK lines, 27 R8 loader checks, 0 FAIL | same | same |
| Tiny386 suite | PASS: 204 CHECK lines, 27 R8 loader checks, 0 FAIL | same | same |
| i386VM R8 session, SCRIPTED | PASS, 22/22 | PASS, 22/22 | PASS, 22/22 |
| Tiny386 R8 session, PS/2 via -term | PASS, 22/22 | PASS, 22/22 | PASS, 22/22 |
| i386VM R8 EOF session, SCRIPTED | PASS, 2/2 | PASS, 2/2 | PASS, 2/2 |

Timer-off negative: `ok (no input read, no exit; > instruction limit exceeded)`.
The R8 shell prints its banner and prompt, never runs the `DIR` command, and
the run is stopped by the harness's instruction limit.

The matrix exited 0 (`matrix-exit=0` in `logs/r8-matrix.txt`).

The R2–R7 kernel tests are part of each suite, so the R2–R7 regressions were
run in each round, not separately.

## Fresh-extraction rebuild

The committed tree (`git archive HEAD`) was extracted into an empty directory,
the R8 targets were rebuilt there, and one full matrix round was run there.
Result: the export built with exit 0 (the build log is
`logs/r8/fresh-export/build-fresh.log`; it has the two `kernel/jobs.c` warnings
noted above and no errors). One matrix round then passed on both targets:
i386VM and Tiny386 suites each PASS (204 CHECK lines, 27 R8 loader checks,
0 FAIL), the i386VM SCRIPTED R8 session PASS, the Tiny386 PS/2 via -term R8
session PASS, the i386VM EOF session PASS, and the timer-off negative `ok`.
The matrix's `images.sha256` from the export matches the working-tree hashes
for every R8 image, the R7 volume and the R8 volume (logs in
`logs/r8/fresh-export/matrix/`).

## What R8 verified at runtime

- The R7 shell remains alive in slot 0 across every R8 session.
- `RUN LARGE.AOU` loads a 106,837-byte a.out from `WIN1_` through channel reads
  and runs it from slot 1. Its sections are text 48,288 bytes, data 32,768 bytes
  and BSS 98,320 bytes, a total of 179,376 bytes. That is 78% of the 224 KiB
  limit (229,376 bytes). The program checks its own table and BSS contents,
  its argv, heap allocation and free with the free-byte count restored, and
  tick advance inside a CPU loop. It prints `QDOS/386 R8 LARGE A.OUT + ARGV PASS`
  only after those checks pass.
- `RUN ARGS.AOU one "two words"` gives `argc = 3` and the expected argv strings.
  Fifteen user arguments give `ARGS argc = 16` and exit status 16. Sixteen user
  arguments are refused with `too many arguments (limit 15)`. A missing closing
  quote is refused with `unterminated quote`. These are checked by the session.
- Negative files fail with the documented error codes and leave the shell usable:
  `BIG.AOU` (not an a.out, code −21), `OVER.AOU` (BSS too large, −24),
  `TRUNC.AOU` (−20), `LONG.AOU` (−20), `ENTRY.AOU` (entry out of range, −25),
  `MAGIC.AOU` (−21).
- The kernel-side negative tests (`tests/test_r8.c`) cover read failure during
  validation and during the copy, length ±1, wrong slot (NOTSLOT1), layout
  one byte over and exactly at the limit, and argument-block faults (not
  NUL-terminated, past 432 bytes, seventeen strings). Each
  checks that the heap and slot ownership are restored and that a later good
  load still succeeds.
- The SDK samples (`SDKHEL`, `SDKARG`, `SDKSTM`, `SDKSTS`) were built with
  `qdos386-gcc` and run from the R8 volume in both sessions, with the
  success lines printed.

## Defects found and fixed during R8

- **Loader ordering.** The first loader copied the image before creating the
  job, so a heap-exhaustion case left a slot dirty. Now the job is created
  first, and it is cancelled if the copy fails.
- **Load time.** The original start-symbol scan read names at random offsets.
  A backward FAT16 seek restarts the cluster walk from the first cluster, so a
  106 KB file took about 12 s and about 10⁹ emulator steps, which exceeded the
  harness's per-run budget. The scan is now forward-only and chunked.
- **Stale PC objects.** The PC objects depended only on their `.c` files, so
  header struct changes left stale objects. That was judged to be the cause of
  random Tiny386 hangs in earlier runs. It is strongly suggested by the file
  timestamps but was **not reproduced deliberately**. The fix is a `PC_HDR`
  dependency on all PC objects. A clean rebuild passes all rounds.
- **Test fragility.** The R7 console-waiter test used a fixed wait. It is now
  bounded by ticks.

## Limitations and what was not tested

- **Preemption of LARGE was not separately verified.** The program checks tick
  advance in its CPU loop, and the shell remains schedulable in the sessions.
  No test isolates a shell input served while LARGE is spinning.
- **No human keyboard testing.** All keyboard input is scripted.
- **The 432-byte argument limit is not reachable from the shell.** The shell
  line is 80 columns, so the 432-byte string limit can only be tested in the
  kernel test; the session checks the 16-argument and 15-argument limits.
- **File names are at most 10 characters.** `CHAN_NAME_MAX` is 16 and includes
  the `WIN1_` prefix and the terminating NUL. The SDK examples were renamed to
  fit.
- **No SuperBASIC size probe.** The SuperBASIC archive was not part of this run.
- **No GCC 2.95.3 QDOS distribution.** The bundled libc was not available; the
  R9 audit in `docs/R8-SDK-INVENTORY.md` uses only the attached C68 archive.
- **`libqdos.a` is built with the host `ar`.** The historical tree has no `ar`.
  The archive links with the historical linker.
- **`-ansi` is not supported by GCC 1.40.** The wrapper refuses C99 and C11 modes
  and uses C89 by default.
- **The SDK is a foundation, not a libc.** There is no `stdio.h`, `malloc`,
  `printf` or `FILE *`. Heap calls are `qd_alloc` / `qd_free` traps and are not
  portable C `malloc` semantics.
- **Three-round results are for the scripted matrix, not for hardware.**
