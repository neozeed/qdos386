# KCTX-FIX2 — runtime report

Kernels under test: the NORMAL build `make` (= `make qdos386-pc`): `build/qdos386/pc-vga/qdos386-vga.img`
`f8e5d5e1…` (QEMU) and `build/qdos386/pc/qdos386.img` `7c675978…` (Tiny386). Negative control: the original R14
kernels from the R14 delivery (`r14-vga.img` `8bfd4d58…`, `r14.img` `1ad61b13…`), used read-only. QEMU 8.2.2
(`-machine pc -cpu pentium -m 16`) is primary; Tiny386 R12C full machine is secondary; at most two emulators ran at
once. Status words: PASS, FAIL, NA, NOT TESTED. Logs: `logs/kctx2/`.

## 1. Gates

| Gate | Content | Result |
| --- | --- | --- |
| 0 | audit: paths, linkage, IF window, `context.s`/`paging.s` reachability (`docs/KCTX-FIX2-AUDIT.md`) | done |
| 1 | normal QEMU and Tiny386 builds preserve ESP residues 0–3 on timer, trap and yield (§2, §3) | **PASS** |
| 2 | probes + affected regressions with unchanged application binaries; negative control (§2–§5) | **PASS** |
| 3 | clean normal builds, historical rebuilds, manifests, fresh-extraction smoke (§6, package verification) | **PASS** |

## 2. Probes on the normal kernels

| Test | QEMU `qdos386-vga.img` | Tiny386 `qdos386.img` |
| --- | --- | --- |
| supplied `spin_aligned` / `spin_misaligned` | exit 0 / exit 0 | exit 0 / exit 0 |
| ALIGNT residues 0, 1, 2, 3 × timer spin (thousands of preemptions) / 200 000 traps / 300 yields — ESP, EBX, ESI, EDI, EBP and EFLAGS.AC checked every iteration | 12/12 OK | 12/12 OK (100 M loops / 50 000 traps / 200 yields) |
| two concurrent misaligned background jobs (residue 1 spin, residue 3 traps) + residue 2 foreground | OK, OK, OK | NA (one terminal) |
| no PGFAULT / #DF / halt; LEAK (heap, channels, address spaces) back to baseline | PASS | PASS; guest shutdown, image clean |
| judge | `logs/kctx2/stage2/probe/judge.txt`: **22/22 PASS** (see note) | `logs/kctx2/stage2/tiny/judge-kctx.txt`: **18/18 PASS** |

Note: a first QEMU probe run with the harness's 90 s per-command timeout judged 21/22 (`probe-90s-timeout/`): the
2·10⁹-loop foreground ALIGNT, slower since the EFLAGS check was added, printed `ticks=9465 result=OK` after the
timeout, so the judge could not attribute its output. Rerun with 180 s: 22/22. The check itself was not changed.

**Hand-off check** (test-only kernel `make qdos386-check-pc`, `logs/kctx2/stage1/check-kernel/`): every resume
hand-off goes through `kx_resume_check` — IF must be clear when the format is written, the frame at ESP must match
the format (format 1: CS = 0x08 and EFLAGS bit 1 above the popal block; format 0: a kernel return address above the
callee-saved block), and the scheduler's kernel-stack canary count must stay 0.

| Workload | Hand-offs checked | IF set | Format/frame mismatch | ESP mod 4 = 0/1/2/3 | Canary faults |
| --- | --- | --- | --- | --- | --- |
| misaligned probe set (as §2, plus spawn/kill) | ≥ 1 245 184 | 0 | 0 | 329 973 / 206 077 / 206 934 / 502 200 | 0 |
| frozen GWBASIC.AOU loop programs | ≥ 458 752 | 0 | 0 | 458 722 / 0 / 30 / 0 | 0 |

(`judge-check.txt`: **14/14 PASS**.) Counts are those of the last 65 536-resume summary (the remainder of each run is checked but not summarised).
GW-BASIC itself is interrupted with a misaligned ESP at times (residue 2 above), which is exactly what crashed the old kernel.

**Mismatch guard** (`logs/kctx2/stage1/mismatch/`): current `kctx.s` with the historical C side does not link
(`_kx_resume_fmt` undefined); the frozen switcher with the current C side builds but crashes during boot (PGFAULT
before the shell) — the halves cannot be mixed silently.

## 3. Frozen GWBASIC.AOU (unchanged, `4ae47943…`) on a copy of its launch volume (`ed03cd27…`)

| Program | Original R14 kernel (negative control) | Current kernel |
| --- | --- | --- |
| flat FOR/NEXT 16000 | never prints DONE; system stops answering | DONE, Ok |
| flat 16400 / 20000 / 200000 | not reached | DONE, Ok each |
| nested 100 × 1000 | not reached | DONE, Ok |
| GOSUB 20000 | not reached | DONE, Ok |
| kernel fault / halt | yes (no LEAK or SYNC reply afterwards) | none |
| KILL, PS, LEAK back to baseline, SYNC, volume fsck clean | — | PASS |
| judge | `negative/judge-gw-negative.txt`: **3/3 as expected** | `gwbasic/gw-current.judge.txt`: **12/12 PASS** (kernel checks) |

Interpreter results, reported apart, **not** counted as kernel results and **not** fixed: `SAVE "LONGPROG"` →
`Device I/O Error`; `LOAD` → `File not found` (so RUN-after-LOAD is NA); the corrected A$ test prints
`HELLO WORLD 11` and then GW-BASIC page-faults on the `MID$/LEFT$/RIGHT$` line (`#PF eip 00241495 cr2 b9f80024`) —
the kernel ends only GW-BASIC ("process ended"), the shell answers. GW-BASIC is not claimed correct.

## 4. Negative control (original R14 kernel, read-only) — `logs/kctx2/stage2/negative/`

`spin_aligned` exits 0; `spin_misaligned` → `#DF double fault`, halt; ALIGNT residue 2 with service traps → #DF,
halt; ALIGNT residue 3 timer spin → `#GP` (vector 0x0d) at the `iret` of `resume_dispatch`, halt: **4/4 as expected**.
GW-BASIC flat 16000: **3/3 as expected** (§3). The same probes pass on the current kernel, so they detect the defect.

## 5. Existing OS suites on the current QEMU kernel (each suite's own unchanged script, volume and judge)

| Suite | Result |
| --- | --- |
| R12A Stage 2 (private heaps/stacks; NULL, wild, #UD faults and a user-stack overflow #DF end only that process) | 24/24 PASS |
| R12A Stage 3 (dynamic processes: spawn, wait, kill) | 35/35 PASS |
| R13B Gate 1 (dynamic shells) / Gate 2 (lookup, PATH, START, SuperBASIC EXEC) | 23/23, 33/33 PASS |
| R13B Gate 3 (Task Manager) / Gate 4 (CMOS RTC, DATE/TIME, SYNC) | 33/33, 24/24 PASS |
| R13A Gate 2 (rendered text consoles) / Gate 3 (graphical surfaces) | 26/26, 22/22 PASS |
| R14 Gate 1 (UART driver) / Gate 4 (QLTerm, X/YMODEM against lrzsz) | 42/42, 37/37 PASS |
| R14A Gate 3 (Hack game, save/restore, 31 checks against the QL binary's screens) | 53/53 PASS |
| R14A Gate 4 (Hack beside SH/BASIC/QLTerm/BOUNCE, errors, disk full, low memory; LEAK, fsck) | 48/48 PASS |
| Tiny386: R14A Hack smoke on `qdos386.img` (start, save, restore, quit, SYNC, LEAK, shutdown, clean image) | 15/15 PASS |

`logs/kctx2/stage2/suites/`, `logs/kctx2/stage2/summary.txt`. FAT and accounting: every suite ends with its own
fsck/LEAK checks (included above).

## 6. Build identity

* Current kernels from `make` = the KCTX-FIX1 kernels byte-for-byte (`logs/kctx2/stage1/current-hashes.txt`); both
  decode as EXACT, every frozen R12A…R14 kernel as PTRBITS (`resume-convention.txt`).
* Frozen: 25 R12…R13B images (0 differ), 15 R14 files (0 differ), 34 R6…R11M images built in an R14A checkout and here
  (0 differ) — `logs/kctx2/stage1/identity-r14.txt`, `legacy-targets.txt`.
* R14A userland rebuilt and compared with the R14A delivery: HACK.AOU, hack.map, libc68.a, libc.a, libqdos.a, ANSIT,
  SPINM, SH, the boot shell, QLTerm — 10/10 identical (`logs/kctx2/stage2/userland-identity-r14a.txt`). Application
  binaries used in the tests are unchanged.

## 7. Not tested

Real hardware (NOT TESTED); SMP (out of scope, single CPU); Tiny386 multi-session coexistence (NA, QEMU only); the
retired i386VM harness (images rebuilt identically, not run).
