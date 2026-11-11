# KCTX-FIX1 verification report

Corrected kernels: `build/kfix1/pc-vga/kfix1-vga.img` (QEMU) and `build/kfix1/pc/kfix1.img` (headless/Tiny386), built
by `make kfix1-pc` = the R14 kernel options + `-DQDOS_KCTX_FIX1` + `kernel/kctx1.s`. Audit and design:
`docs/KCTX-FIX1-AUDIT.md`. QEMU 8.2.2 (`-machine pc -cpu pentium -m 16`), R12C Tiny386. Logs: `logs/kctx/`.
Status words as in R14A: PASS / FAIL / NA / NOT TESTED; interpreter results are reported apart.

## 1. Controls (Stage 1 baseline and Stage 3)

| Test | R14 kernel (`8bfd4d58…`, unchanged) | KFIX1 QEMU | KFIX1 Tiny386 |
| --- | --- | --- | --- |
| `spin_aligned` (supplied) | exit 0 | exit 0 | exit 0 |
| `spin_misaligned` (supplied) | **#DF, kernel halt** | exit 0 | exit 0 |
| ALIGNT residue 0 — spin / trap / yield | OK / OK / OK | OK / OK / OK | OK / OK / OK |
| ALIGNT residue 1 — spin / trap / yield | **#DF** (spin) | OK (3249 ticks) / OK / OK | OK / OK / OK |
| ALIGNT residue 2 — spin / trap / yield | **#DF** (each) | OK (3207 ticks) / OK / OK | OK / OK / OK |
| ALIGNT residue 3 — spin / trap / yield | **#GP at iret** (spin) | OK (3367 ticks) / OK / OK | OK / OK / OK |
| two misaligned jobs at once (residue 1 spin 2·10⁹ loops, residue 3 trap 300 000) + residue 2 foreground | — | OK, OK, OK (7090 ticks) | — |
| no PGFAULT; LEAK before = after | — | PASS (22/22) | PASS (18/18) |

“Spin” = ESP held at the residue for thousands of timer preemptions with EBX/ESI/EDI/EBP patterns and ESP checked
every iteration; “trap” = a service trap per iteration (200 000); “yield” = a one-tick sleep per iteration (300), so
the scheduler runs other jobs and resumes this one. Logs: `logs/kctx/stage1/`, `logs/kctx/stage2/qemu-fix.txt`
(`tests/kctx/judge_probe.py`), `logs/kctx/stage3/tiny/`.

## 2. The frozen GWBASIC.AOU (unchanged) on its launch volume — `logs/kctx/stage3/`

| Program | R14 kernel | KFIX1 |
| --- | --- | --- |
| flat FOR/NEXT 16000 | never finishes (kernel halt) | DONE, Ok |
| flat 16400 / 20000 / 200000 | not reached | DONE, Ok (each) |
| nested 100 × 1000 | not reached | DONE, Ok |
| GOSUB 20000 | not reached | DONE, Ok |
| kernel fault / halt during the run | halt | none |
| GW-BASIC killed afterwards, LEAK before = after, SYNC, volume fsck | — | PASS |

Interpreter results, reported apart (`logs/kctx/stage3/saveload-ab/`, GW-BASIC not changed):

* `SAVE` → `Device I/O Error` for every name and directory tried, foreground and `/NEW` — **the same on the
  checkpoint's own R13B kernel, on R14 and on KFIX1**; `LOAD` → `File not found`. Not a kernel regression.
* String test: `B$=A$+" WORLD"` and `LEN` correct (`HELLO WORLD 11`); the `MID$/LEFT$/RIGHT$` line page-faults inside
  GW-BASIC (`eip 00241495 cr2 b9f80024`). The kernel ends only that process and continues (fault isolation). On R13B
  and R14 the run halts on the kernel defect before this point, so the fault was not visible there.
  (Correction: the message of commit `c137378` says the MID$ fault was reproduced on R13B/R14 too; it was not —
  only the SAVE/LOAD results were. This report is the correct statement.)

## 3. Existing OS suites on KFIX1 (each suite's own script, volume and judge) — `logs/kctx/stage3/suites/`

| Suite | Result |
| --- | --- |
| R12A Stage 2 (private heaps/stacks; NULL, wild, #UD faults and a stack overflow #DF end only that process) | 24/24 PASS |
| R12A Stage 3 (dynamic processes: spawn, wait, kill) | 35/35 PASS |
| R13B Gate 1 (dynamic shells) | 23/23 PASS |
| R13B Gate 2 (lookup, PATH, START, SuperBASIC EXEC) | 33/33 PASS |
| R13B Gate 3 (Task Manager) | 33/33 PASS |
| R13B Gate 4 (CMOS RTC, DATE/TIME, SYNC) | 24/24 PASS |
| R13A Gate 2 (rendered text consoles) | 26/26 PASS |
| R13A Gate 3 (graphical surfaces) | 22/22 PASS |
| R14 Gate 1 (serial driver) | 42/42 PASS |
| R14 Gate 4 (QLTerm, background X/YMODEM against lrzsz) | 37/37 PASS |
| R14A Gate 3 (Hack game, save/restore, QL comparison) | 53/53 PASS |
| R14A Gate 4 (Hack beside SH/BASIC/QLTerm/BOUNCE, errors, disk full, low memory) | 48/48 PASS |
| Tiny386: KFIX1 headless probes | 18/18 PASS |
| Tiny386: R14A Hack smoke on KFIX1 headless | 15/15 PASS |

## 4. Frozen releases

`tests/r14/check-frozen.sh`: the 25 frozen R12–R13B kernel and shell images rebuilt from this source — 0 differ.
`tests/r14a/check-r14-identical.sh`: R14 kernels, libraries, QLTerm and the R14 guest programs — 15/15 identical to
the R14 delivery; R14A's `HACK.AOU` rebuilds to the delivered `18c4ab6c…` (`logs/kctx/identity.txt`). The fix exists
only in the new `kfix1` target.

## 5. Not tested / limits

* Real hardware: NOT TESTED (QEMU `pentium` and Tiny386 only).
* `context.s` and `paging.s` keep the same `andl $-4` decode; they are linked only into frozen legacy images, which
  were deliberately not changed. A revived target using them needs the same change.
* GW-BASIC's own faults (§2) are for the GW-BASIC builder.
