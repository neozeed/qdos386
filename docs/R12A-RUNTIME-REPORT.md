# R12A runtime report — dynamic process address spaces, private heaps/stacks, general process launch

Base: frozen R12 `0e92a742291a47067823d42783ee3eb366d02b83`. Branch `r12a-dynamic-processes`. The exact final commit and
the archive SHA-256 are in `PACKAGE.md` and the delivery note. Kernel under test (all Stage 1-4 evidence below):
QEMU `build/r12a/pc-vga/r12a-vga.img` SHA-256 `257460b9…5b99`, headless `build/r12a/pc/r12a.img` `0b23ab9d…2ff3`
(`logs/r12a/stage4/kernel-sha256.txt`).

**Private process memory is not security isolation.** Every process still runs at CPL 0 (R12A-KNOWN-LIMITATIONS.md §1).

## 1. Result

| Gate | Content | Result |
|---|---|---|
| Phase 0 | R12 baseline, audit, design | done; R12 smoke PASS (demo 27/27, NEG 848/0, MULTI PASS) |
| 1 | ≥3 copies of one unchanged executable at one origin; distinct frames; independent globals; exit codes; frames back to baseline | **PASS** (20/20) |
| 2 | private heaps and stacks under preemption; safe kernel entry; fault isolation; bad pointers; reclaim | **PASS** (24/24) |
| 3 | an ordinary application spawns two same-origin children, waits for both, repeats without leaks; the shell does the same | **PASS** (35/35; SPAWNT 95 checks over 3 cycles) |
| 4 | R12 applications in concurrent same-origin processes; shell and BASIC usable; memory to baseline; QEMU regressions explained | **PASS** (demo 23/23; R12 suites all pass; historical fixed-slot tests: differences explained, §5) |
| Tiny386 | one bounded full-machine smoke, guest 0x8900 shutdown | **PASS** (exit through SHUTDOWN.AOU, flush report, fsck + walker clean) |

**No fixed slots remain in the normal launch path**: the shell itself and every program it or any other process starts get
frames from one pool; the historical link origins are honoured per process, not reserved globally.

## 2. Evidence classes

* **Host test** (strict C89, `-Werror`, `sh tests/host/r12a/run.sh`): frames/vas 74 checks (accounting, exhaustion, double
  free, distinct frames for one VA in four spaces, template sharing, pool pages never visible, page 0 hidden, **rollback to
  the exact free count at every one of the 38 allocation points**, 200 create/destroy cycles, table limit); pheap 17 checks
  (errors, zero fill, coalescing, growth to the cap, refused growth, 20,000 random operations against a shadow model).
* **Guest executed**, QEMU full machine (VGA, PS/2 keyboard, IDE; scripted keyboard; text-buffer capture), unless stated.
* **Independent comparison**: mtools reads, `fsck.fat -n`, `tools/r11m-fatwalk.py --vfat --strict`, default-image hashes.

## 3. Gates 1-3 (QEMU)

**Gate 1** (`tests/r12a/run-s1.sh`, `logs/r12a/stage1/`): four MAPTEST copies (one unchanged a.out, origin 0x240000)
alive at once (every start before any end) in four directories; the data page and the user stack of each are four
different frames at identical virtual addresses; each kept its own global for 5,000 rounds; exit statuses 11/12/13/15;
two foreground copies 16/17. BIGBSS (960 KiB BSS) started until the pool ran out: the next START got the stable error
**-60** ("out of memory: no free page frames"), no hang; after `KILL /CON:2` the kernel heap, channels, spaces and frames
were back at the baseline and a new copy ran (18).

**Gate 2** (`logs/r12a/stage2/`): three HEAPT copies + a MAPTEST sibling + two hold-flag testers concurrently:

| | HEAPT A | HEAPT B | HEAPT C |
|---|---|---|---|
| peak private heap | 417,792 B | 512,000 B | 466,944 B |
| heap at end | 0 | 0 | 0 |
| timer preemptions (on the user stack) | 59 | 98 | 69 |
| kernel-stack high-water mark | 1,484 B of 16 KiB | 1,484 B | 1,484 B |

Each did 1,500 rounds of malloc/realloc/free with per-block patterns, 40-level recursion with 512-byte frames on its
private stack, concurrent reads of LONG.TXT, heap validation, and an x87 accumulator replay — all PASS. Hold flags in
private BSS: 2 × 300 suspends, every flag set, sentinels intact (writes deferred to the owner's directory). STKT depth 40
passed; depth 90 ran into the guard: `#DF double fault … user stack overflow, process ended`. FAULTT: NULL store (CR2 0;
page 0 is hidden in process directories), wild call to 0x330000 (CR2 = EIP), `#UD`, `#DE`, unmapped window page — each
reported (vector, error, CR2, EIP, job, CR3, registers) and only that process ended; 12 bad pointers (NULL, kernel text,
kernel heap, unmapped page, crossing the stack top, past the heap, wrapping, transfer area, NULL/boot-stack names, hold
flag in kernel memory, info buffer in kernel memory) all refused. The MAPTEST sibling ran 9,000 rounds through all of it
(PASS, 21). Baseline restored; eight more create/exit cycles and another HEAPT left it unchanged.

**Gate 3** (`logs/r12a/stage3/`, `SPAWNT.LOG`): SPAWNT ×3 cycles, 95 checks: two CHILDT children (same image and origin,
own directories, parent recorded) waited out of order (22 then 21); poll of a running child; 20-tick timeout then wait;
zombie status kept for a child that ended first; not-a-child refusals (self, 0, unknown, the shell; kill of a non-child);
kill (status -66); held child cancelled before running, held child activated (27); missing file -44, malformed -21,
unknown flag -34, arguments in kernel memory -14 — none left anything behind; child limit (8, then -69); three concurrent
exits (41/42/43); frames and the parent's heap unchanged per cycle and overall. Orphan: the parent ended, its child ran to
completion. Interactive child on console 3 read its own line while the parent waited on console 1 (status 52). The shell
(a process itself) ran the same child with START (61/62) and RUN (63/64).

## 4. Gate 4: real applications

**R12A integrated demonstration** (`tests/r12a/run-demo.sh`, `logs/r12a/stage4/demo/`, 23/23): boot 1 — LEAK baseline;
three MAPTEST copies (console 1), EDIT on console 2, two SuperBASIC copies (consoles 3 and 1) all running at once (PS
lists the shell and these six processes); BASIC copy 1 runs a typed program (1 4 9), copy 2 runs times_table.bas and answers PRINT 6*7 = 42;
the editor saves a long-name file; meanwhile SPAWNT (spawn/wait from an ordinary application) and HEAPT pass on console 0;
statuses MAPTEST 11/12/13, EDIT 0, BASIC 0/0; LEAK identical before and after; EXIT clean. Boot 2 on the same volume: the
file is there, the editor reopens it, BASIC runs greeting.bas, SPAWNT passes again, two more MAPTEST copies (21/22), LEAK
identical to boot 1, EXIT clean; mtools reads exactly `line one\nline two`; `fsck.fat` and the VFAT walker clean.

**R12 suites, unchanged (R12 volumes, R12 shell binary, R12 programs byte for byte), on the R12A kernel**
(`tests/r12a/run-r12-suites.sh`, `logs/r12a/stage4/r12-suites/`):

| Suite | R12 on R12 kernel | on R12A kernel |
|---|---|---|
| Stage 1 shell (DIR /W /P, line editor, history, console API) | 59/59 | **59/59** |
| Stage 2 EDIT | 62/62 | **62/62** |
| Stage 3 sessions/jobs | 30/30 | **30/30** |
| Stage 4 BASIC interactive | 58/58 | **58/58** (see note) |
| Stage 4 SuperBASIC corpus (103 programs, 3 examples identical to the oracle) | 19/19 | **19/19** |
| R12 integrated demo (incl. Phoon, PATHT 68, STRTST 37, reboot) | 27/27 | **27/27** |

Note: in the four-runs-in-parallel pass on this two-CPU host, run b's 120-second wait for a background BASIC loop expired
at n=2000 of 4000 (the program then finished with status 0, visible in its PS); run b alone passed (58/58 with a, c, d of
the same pass). Timing of a busy loop under 4× CPU oversubscription, not a kernel fault; recorded, not hidden.

## 5. Historical fixed-slot tests (labelled)

`logs/r12a/stage4/historical/` (with the malformed images and BIG.BIN on the volume):

| Test | Result on R12A | Why / dynamic equivalent |
|---|---|---|
| NEG.AOU | 848 checks, **3 failures** | the 3 contention checks expect -28 "slot busy" for a second image at an occupied profile; R12A starts it in its own space. Malformed images (13), wrong origin, churn with stale-id rule, invalid ids: all pass. Invariant now proven by MAPTEST (Gate 1). |
| MULTI.AOU | FAIL | same assumption (`collide on profile 2` expected refused); its children at 0x380000/0x500000 ran and exited 3/7, stale ids refused |
| PGRUN 3 TWICE | **DONE** (3 copies, overlap yes, released yes) | R11K veneer works |
| TWICET M, F, W | **PASS** | pairs, faults beside a healthy copy, console waiter |
| TWICET H, E | FAIL | expect at most 3 paged spaces ("beyond the limit is refused"); R12A allows more. Their extra held copies stayed until `KILL /CON:0`, then baseline restored |
| TWICET N | FAIL | expects a paged process to be refused launching (R11K rule); Stage 3 lifts that rule |
| TWICET S | FAIL | expects the same frame run to be reused ("space reused"); frames are now allocated singly |
| TWICET Z | FAIL (desired) | expects the kernel heap to be shared between paged copies; R12A heaps are private — copy B cannot see copy A's block (it then stores through a NULL and is ended alone) |

## 6. Tiny386 (bounded secondary smoke)

`logs/r12a/stage4/tiny386/`: R12A headless kernel, 8 MiB: PINFO reports the pool **1,216 frames (4,864 KiB)**, caps
`0110001f`; DIR /W, MD, COPY to a long name; MAPTEST (7); SPAWNT 1 cycle PASS 33/0; two background MAPTEST copies;
BASIC times table; LEAK identical before and after; `SHUTDOWN.AOU` → `tiny386: guest shutdown request; disk images
flushed and closed`, exit 1 rc 0; FAT copies identical, `fsck.fat` clean, walker clean (622 files, 12 directories).
A first attempt (`logs/r12a/stage4/tiny386-first/`) had the same clean shutdown and image but SPAWNT counted the frames of
two unrelated MAPTEST copies that ended during its run as a difference (+58 frames, +32,992 heap bytes — more free, not
less); the sequence was reordered so SPAWNT runs alone. Harness defect, recorded.

## 7. Memory figures

| | R12 kernel | R12A kernel |
|---|---|---|
| free kernel heap, idle shell | 667,024 B | 806,064 B (QEMU) / 806,864 B (headless) |
| application memory | 3 × 1 MiB fixed windows + shared heap | pool 3,264 frames (12.75 MiB) QEMU, 1,216 (4.75 MiB) Tiny386 |
| a small process (MAPTEST) | 1 MiB window + 64 KiB heap stack | 28 frames (112 KiB) + 16 KiB kernel stack |
| jobs | 6 | 16 |

## 8. Regressions guarded

* Default images: 6 × IDENTICAL (`scripts/r11l-default-check.sh`); R12 kernels rebuild byte-identical
  (`dd11fe87…`, `1e00386b…`); R12 binaries on the R12 volumes unchanged (shell-m, EDIT, BASIC, LEAK, TICKER, PATHT, Phoon).
* No CPU core or emulator change.

## 9. Package verification

`tests/r12a/verify-package.sh` on a fresh extraction: manifest (`SHA256SUMS`) check, a QEMU smoke from the extracted
kernel and volume (PINFO, three MAPTEST copies, SPAWNT, LEAK), and a rebuild of both R12A kernels and the R12A shell from
the extracted source — byte-identical to the shipped ones (`logs/r12a/stage5/package-verify.txt`).

## 10. Not tested

See `R12A-KNOWN-LIMITATIONS.md` §5.
