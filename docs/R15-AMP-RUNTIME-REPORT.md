# R15 AMP0 — runtime report

AMP kernel `build/qdos386-amp/pc-vga/qdos386-amp-vga.img` (headless `…/pc/qdos386-amp.img`); ordinary kernels
unchanged (KCTX-FIX2 `f8e5d5e1…`, `7c675978…`). QEMU 8.2.2 `-machine pc -cpu pentium -smp 2 -m 32` (SeaBIOS) is
primary, `-smp 1` the single-CPU regression, Tiny386 R12C one bounded smoke. At most two emulators ran at a time.
Status words: PASS, FAIL, NA, NOT TESTED. Hashes of every kernel and guest program in each run's `*.inputs.sha256`;
logs `logs/r15/`. Exact hashes of the delivered images: `SHA256SUMS` of the package.

## 1. Gates

| Gate | Result | Logs |
| --- | --- | --- |
| 0 audit | done: `docs/R15-AMP-AUDIT.md` (written from the source reading that preceded the code; there is no separate audit-only commit — the first checkpoint `48f4957` already contains the Phase 1–2 code) | — |
| 1 bring-up | **18/18 PASS** | `logs/r15/gate1/` |
| 2 mailbox, concurrent computation, lifecycle, worker fault / quiesce / restart | **25/25 PASS**; host oracle **39/39** results, 0 mismatches | `logs/r15/gate2/` |
| 3 (stretch) native application, kill without leak | **7/7 PASS** | `logs/r15/gate3/` |
| 4 regression and failure matrix | no regression attributable to AMP; two non-AMP results recorded (§5) | `logs/r15/regress-*`, `hack-smp2/`, `tiny/`, `identity/`, `static/` |
| two-worker `-smp 3` stretch | **NOT TESTED** (not attempted) | — |

## 2. Gate 1 — the second processor, and only that (`tests/r15/run-gate1.sh`)

| Run | Result |
| --- | --- |
| a `-smp 2 -m 32` | `AMP: ACPI MADT: 2 enabled processor(s), BSP LAPIC id 0`; `AMP: worker CPU LAPIC id 1 READY (boot 1, within 1–2 ms)`; INFO: worker reports LAPIC id 1 by MMIO and by CPUID; heartbeat 9.9 M → 22.1 M between two INFOs 3 s apart; DIR, ECHO, LEAK answered; 30 checked requests |
| b `-smp 1` | `only one enabled processor`, service UNAVAILABLE (reason 5), requests −80, shell normal |
| c `-cpu pentium,-apic` | `CPUID: no local APIC`, fail closed |
| d test kernel, start-up IPIs never sent | `worker did not report READY within 200 ms`, no hang |
| e test kernel, worker READY seconds late | unavailable at boot; INFO later shows `late 1` with the late worker's heartbeat running, still UNAVAILABLE (never adopted) |
| f ordinary kernel under `-smp 2` | boots and works as before (the second CPU stays in the firmware) |

Worker fault containment and restart (Gate 2 run r, `AMPT FAULT` / `AMPT QUIESCE`): a #UD on the worker is
recorded (vector 6, EIP `0x125041` = `_ap_invalid_op`, ESP, APIC ID 1), the worker halts, CPU 0 continues, the running
request reports AMP_ERR_FAULT, the restart (allowed only after that acknowledgement) gives boot 2 and 100 checked
requests pass; QUIESCE → acknowledged → restart → 20 checked requests.

## 3. Gate 2 — independent concurrent computation (`tests/r15/run-gate2.sh`)

Run **c** (concurrency), one boot: BOUNCE animating 3,000 frames in its own session; a 1,000,000,000-iteration NUMERIC
started in the background on the worker; meanwhile ECHO / DIR / PS typed and answered, SPINM measured CPU 0 at 432,840
loops / 300 ticks before and 404,974 while the worker computed; INFO showed the slot RUNNING; the background job
finished after 1,856 ticks with the result that CPU 0 recomputed (in 1,131 ticks) — and the host oracle recomputed;
BOUNCE.LOG `frames=3000 … verify=PASS`; LEAK equal before and after; no PGFAULT.

* **PROOF** (CPU 0 inside the QT_AMP trap, interrupts off, executing only a load/pause loop until the worker answers):
  `iters 20000000 seed 99 → r0 21431280 r1 f65e18a4, 587,691 spins, ticks 6161 → 6161`; `iters 100000000 seed 5 → r0
  23ba5d70 r1 f9954ad2, 2,615,248 spins, ticks 6560 → 6560`. No timer tick was handled on CPU 0 during the wait, yet
  the correct result arrived: it was computed by the other processor. Both results also checked by the host oracle.
* **Host evidence** (`tests/r15/cpu-sampler.py` on a second QEMU monitor, 478 samples over 120 s): the two vCPUs run on
  two host threads (multi-threaded TCG); each consumed **115.3 s of host CPU in 120.3 s** — simultaneous execution;
  from the worker start on, CPU 1's EIP was always inside the worker code (0x124b3c–0x125050) and inside `ap_run` in
  87 samples; CPU 0's EIP was never in the worker code.

Run **r** (repetition and lifecycle):

| Check | Result |
| --- | --- |
| a live job's request: owned (slot busy, gen 1); a foreign job's POLL / CANCEL → AMP_ERR_OWNER | PASS |
| owner exits after completion → reclaimed as orphan (no cancel needed) | PASS |
| owner exits during a long cooperative request → cancel sent at once, reclaimed after CANCELLED | PASS |
| owner gone, request that ignores cancellation → NOT reclaimed while running; reclaimed after DONE | PASS |
| `AMPT EDGE` (zero / 16,384 / 16,385 lengths, kernel pointers refused, invalid command → ERROR BADCMD, XFORM bounds, malformed raw request refused by the worker, BUSY, NOTREADY, stale POLL, stale worker answer, cooperative cancel, non-cooperative cancel, 40 requests across the 32-bit generation wrap) | 24/24 PASS |
| **10,000 requests** (CRC32 / XFORM / NUMERIC, lengths 0–16,384), each checked byte for byte; worker completed exactly 10,000; ticks monotonic | PASS, 4,767 ticks |
| worker #UD (`AMPT FAULT`): record (vector 6, EIP = `_ap_invalid_op`), halt, AMP_ERR_FAULT, restart, 100 checked requests | 6/6 PASS |
| `AMPT QUIESCE`: acknowledged, restart, 20 checked requests | 4/4 PASS |
| LEAK before = after | PASS |

## 4. Gate 3 — a native application (`tests/r15/run-gate3.sh`)

`WORKER FILE /ETC/STARTUP.CMD` → CRC-32 `b19bed34`, equal to the host's `zlib.crc32` of the source file;
`WORKER VECTOR 5` / `6`: 16 KiB transforms verified byte for byte while the program counted its own work;
`WORKER BUSY` KILLed → its request cancelled immediately and reclaimed after the worker's acknowledgement (slot 0,
orphans 1, cancels 1); LEAK unchanged (the pinned buffers are static: nothing to leak); `-smp 1`: "no worker CPU
service", exit 3. WORKER uses only QT_AMP (no LAPIC, no shared memory).

## 5. Gate 4 — regressions and failure matrix

Each suite with its own unchanged script, volume and judge (`tests/r15/run-regress.sh`, `-smp`/`-m` forced by
`tests/r15/qemu-wrap`); every main transcript shows the kernel under test's AMP boot line (15 of 19; the other 4 are
R13B Gate 4's own frozen epoch / no-RTC test kernels).

| Suite | AMP **burn** kernel, `-smp 2 -m 32` (worker computing CRCs continuously) | AMP kernel, `-smp 1 -m 32` |
| --- | --- | --- |
| R12A Stage 2 / 3 | 24/24, 35/35 | 24/24, 35/35 |
| R13B Gate 1 / 2 / 3 / 4 | 23/23, 33/33, 33/33, 24/24 | 23/23, 33/33, 33/33, 24/24 |
| R13A Gate 2 / 3 | 26/26, 22/22 | 26/26, 22/22 |
| R14 Gate 1 / 4 (UART, QLTerm + lrzsz) | 42/42, 37/37 | 42/42, 37/37 |
| R14A Gate 3 (Hack, deterministic replay) | **30/53 FAIL** — see below | 53/53 |
| R14A Gate 4 (coexistence, errors, disk full, low memory) | 48/48 | **46/48 FAIL** first run; **48/48** on rerun — see below |

* **R14A Gate 3 with a busy worker**: the gate replays one fixed game by running QEMU with `-icount shift=4` and a fixed
  RTC, so that Hack's clock seed is identical every run. Under `-icount` QEMU's virtual clock advances with the
  instructions of *all* vCPUs; the burn worker executes continuously, so the clock Hack reads at start-up differs and a
  different (valid) dungeon is played: the scripted moves stop matching (record line `… 50 SM root,quit` instead of
  `… 105 …`). Evidence that this is the replay mechanism and not a Hack or kernel fault: the same gate under `-smp 2`
  passes **53/53 with the ordinary kernel** and **53/53 with the AMP kernel whose worker is idle** (its polling
  `pause` yields in TCG), **53/53 under `-smp 1`**; the dungeon-independent Hack smoke passes **9/9 with the AMP kernel
  and 9/9 with the burn kernel under `-smp 2`** (start, inventory, version, save, restore, quit, record line), and
  R14A Gate 4 (Hack with save/restore beside four other programs) passes 48/48 with the burn worker. Recorded as FAIL of
  the replay, not hidden; `logs/r15/hack-smp2/`.
* **R14A Gate 4 under `-smp 1`, first run**: the two CPU-meter checks (SPINM while Hack waits ≥ 90 % of SPINM alone)
  read 677,617 and 738,617 vs 985,999 — host timing (the meter counts loops per tick on a shared host). Nothing of the
  AMP code runs under `-smp 1` beyond the boot-time check. Rerun of the same gate: 1,028,251 and 992,455 vs 1,056,419,
  **48/48** (`logs/r15/regress-smp1/rerun-r14a-g4/`). Both runs are kept.

Other checks:

| Check | Result |
| --- | --- |
| worker static check (`tests/r15/check-ap-objects.py`), VGA and headless AMP kernels: worker objects reference only their own symbols; no INT / far call / outside jump in 440 worker instructions; absolute operands only in worker data; CPU 0 objects use only the worker's start-up data and mailbox | 8/8 and 8/8 PASS (`logs/r15/static/`) |
| the worker never calls `sched_irq_tick`, `resume_dispatch`, `trap_dispatch`, the file system or the console | PASS (symbol- and disassembly-level, above) |
| Tiny386 (one CPU), AMP headless kernel: service unavailable (reason 1: no CPUID instruction), requests refused, WORKER exit 3, shell, SYNC, LEAK, shutdown, clean image | 8/8 PASS; R14A Hack smoke on the same kernel 15/15 (`logs/r15/tiny/`) |
| ordinary `make` kernels = KCTX-FIX2 bytes; `kfix1-pc` identical | PASS |
| frozen R12…R13B images (25), R14 files (15), R14A userland (10), R6…R11M legacy images (34) rebuilt identical | 0 differ in each (`logs/r15/identity/`) |
| timer monotonic during 10,000 requests and during all AMPT runs | PASS |
| disk integrity (suite fsck checks, Gate 2/3 volumes) and accounting (LEAK) | PASS |
| input latency | not measured as a number (commands answered within the per-command timeouts) |
| real hardware, `-smp 3`, SMP | NOT TESTED |

## 6. Reproduce

    make qdos386-amp-pc; sh scripts/r15-build-guest.sh
    make qdos386-amp-variant AMPV=nosipi AMP_EXTRA=-DQDOS_AMP_TEST_NOSIPI   (also late, burn: see the Makefile)
    sh tests/r15/run-gate1.sh OUT1; sh tests/r15/run-gate2.sh OUT2; sh tests/r15/run-gate3.sh OUT3
    sh tests/r15/run-regress.sh build/qdos386-amp-burn/pc-vga/qdos386-amp-vga.img 2 32 OUT4
    sh tests/r15/run-regress.sh build/qdos386-amp/pc-vga/qdos386-amp-vga.img 1 32 OUT5
    sh tests/r15/run-hack-smoke.sh KERNEL 2 OUT TAG; TINY386=… sh tests/r15/run-tiny.sh OUT
    python3 tests/r15/check-ap-objects.py build/qdos386-amp/pc-vga; python3 tests/r15/oracle.py TRANSCRIPT…
    sh tests/r15/check-identity.sh R14_PKG R14A_PKG R14A_CHECKOUT LOG
