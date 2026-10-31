# R11J runtime report — paging and same-origin multi-process proof of concept

Branch `r11j-paging` from frozen R11I `bc9a90f`.  Every run is SCRIPTED (keys piped to the harness or
sent by the run drivers); no human session.  Transcripts: `logs/r11j/matrix/`.  They are re-judged by
`scripts/r11j-assert.py`, which parses the addresses, directories, frames, counts, codes and fault
fields itself; where a transcript carries no independent evidence (some QEMU fields), it falls back to
the program's own verdict or reports NA.  Result: **375 PASS, 0 FAIL, 9 NA (all NA are QEMU fields cut at 80 columns)** (`logs/r11j/matrix/results.json`).

This report was revised after an independent review of the code and logs (see
`docs/R11J-KNOWN-LIMITATIONS.md`, "Review findings"): two kernel defects for unpaged slot-1 programs
were fixed, the proof program's detection was made independent of window memory, a negative control
was added, the checker was strengthened, and the whole matrix was rerun.  All figures below are from
that rerun.

## 1. Verdict per platform

| Claim | i386VM block | Tiny386 PC | QEMU |
|---|---|---|---|
| Identity paging (CR0.PG) with shell, console, timer, volume and ordinary programs | **live-proven** (MULTI ×3, NEG 848/0 ×3, R11F regression) | **live-proven** (MULTI ×3, NEG 848/0 ×3) | **live-proven** (MULTI ×3) |
| Two live copies of one a.out at 0x240000 in different CR3 spaces; distinct writable state across timer preemption; independent exits; frames released | **live-proven ×3** | **live-proven ×3** | **live-proven ×3** (per-copy preemption counts NA, §7) |
| Detection works: copies forced to share frames are caught | **live-proven** (negative control FAILs) | — | — |
| Another copy's frames are unmapped in a process (access faults) | **live-proven** (the core then ends the run) | **live-proven ×3** | **live-proven ×3** |
| Guest #PF handler reports and ends only the faulting job; others continue | **not provable** (core aborts before IDT delivery) | **live-proven ×3** | **live-proven ×3** |
| Unpaged job's hold flag written in its own space while paged copies run | **live-proven ×3** (8/8; pre-fix kernel 0/8) | **live-proven ×3** | on screen ×3 |
| Negative cases, repeated launch/exit, reclamation | **live-proven ×3** | **live-proven ×3** | **live-proven ×3** |
| Frame exhaustion refused (−60) | TEST cap image | **live-proven ×3** | **live-proven ×3** |
| Failed copy rolls back | TEST image | TEST image | not run |
| Interrupt immediately before and after every CR3 load | TEST image | TEST image | not run |
| Poll/schedule-list tasks run in the template | code-reviewed only | code-reviewed only | code-reviewed only |
| Default (non-paging) images unchanged and passing | **byte-identical; live ×3** | **byte-identical; live ×3** | not rerun (byte-identical to R11I) |

i386VM is deterministic: its three rounds replay identically and show reproducibility, not
independent samples.  Tiny386 and QEMU rounds vary in timing.

**Success criterion met** on i386VM block, Tiny386 PC and QEMU: two live instances of one
byte-identical fixed-origin a.out execute at the same virtual entry in different CR3 address spaces,
keep distinct writable state across real timer-driven preemption, use the same kernel, exit
independently and release their physical pages.

## 2. Identities

| Artefact | SHA256 (16) |
|---|---|
| Final source commit | recorded in PACKAGE.md |
| i386VM harnesses `i386vm-r9` / `-block` (R11I, unchanged) | `a4724f79b26a716e` / `5970e16223075cf5` |
| `i386vm-r9-block-fd` (optional faultdump patch; host-compiler dependent) | `2b5a15cb12cfceec` |
| Tiny386 emulator of record; PC firmware `bios.bin` / `vgabios.bin` | `fb63b405…`; `7bd968419c81fcf8` / `f5aedb52cde5c88d` |
| Historical toolchain `bin/i386-gcc` | `3567f3beab83e17d` |
| POC i386VM image `qdos386-r11j-poc.bin` (3,670,016 B, kernel only) | `6f839329d97968de` |
| POC PC disk `r11j-poc.img` / VGA `r11j-poc-vga.img` | `e2a38a992053ed55` / `f9b38e2064ff7f73` |
| SAME.AOU / SAMELNCH.AOU (unchanged SDK, `-slot 1 -O`) | `372b713099476f99` / `c45239fa92fb4ede` |
| R11J volume `win1-r11j.img` (separate disk) | `ee3dee11b931b4eb` |
| R11H release / negative volumes (unchanged) | `672806ad2c2ca337` / `5af618370d810bb4` |
| Default images (block, legacy, legacy-NEG, PC, PC kernel, PC VGA) | identical to R11I: `3ee37c19…`, `c5462138…`, `43ba3cc3…`, `8f22b0b2…`, `5fed2619…`, `0560ff32…` |

## 3. The same-origin proof (SAMELNCH main mode)

One file, `SAME.AOU`, started twice with `QT_PG_START` (arguments `1` and `2`).

| Observation | i386VM block (all 3 rounds identical) | Tiny386 PC (rounds 1–3) |
|---|---|---|
| `main` / `gvals` / `gdata` of both copies | 0x2404fc / 0x243edd / 0x243b30 (same) | same |
| Launcher (template): CR3, PTE of 0x240000 | 0x1B000, 0x00240063 (itself) | 0x11B000, 0x00240063 |
| Copy 1: CR3, frames, PTE | 0x20000, 0x900000, 0x00900027 | 0x120000, 0x380000, 0x00380027 |
| Copy 2: CR3, frames, PTE | 0x25000, 0xA00000, 0x00A00027 | 0x125000, 0x500000, 0x00500027 |
| Pointer checks (each copy): own, heap, 0x01000000, pool base, shell | 1, 1, −14, −14 (0x900000), −14 | 1, 1, −14, −14 (0x380000), −14 |
| Third copy while both live | −29 (no job slot; frames rolled back) | −60 (no frames) |
| Launcher's own slot-1 hold flag, 8 sleeps beside the copies | set 8 of 8 | set 8 of 8 |
| Timer preemptions, copy 1 / copy 2 | 21 / 24 | 18–19 / 19–20 |
| State checks / bad | 38 / 0 and 37 / 0 | 375–396 per copy / 0 |
| Hold-flag writes deferred (copies' + launcher's) | 32 | 31–32 |
| CR3 loads per run | 1,334 | 1,315 |
| Exits; after reclaim | 0x51, 0x52; held 0, regions 2/0x140000 (baseline) | same |

**How sharing would be detected.**  Each copy computes its expected values from `qd_self()` (its tagged
job id, from the kernel) kept on its job stack, which is shared kernel heap, outside the window.  It
fills the same virtual globals (BSS 256 words, a data word) and a stack canary with that id's pattern,
keeps an x87 accumulator, and checks everything repeatedly through three phases: CPU-bound (only the
timer switches), 12 sleeps with a hold flag in its own window, and 400 immediate yields (a CR3 switch
each).  If two copies shared their window, each would find the other's id pattern in `gvals`.

**Negative control** (`-DQDOS_TEST_PG_SHARE_FRAMES`: every copy's window mapped to the first copy's
frames): the launcher's PTE check fails, copy 1 reports `bad 40`, both copies exit 0x5F, and
`SAMELNCH M: FAIL`.  So the proof can fail, and does when frames are shared.

The launcher observed both copies dispatched and still running at the same instant, the copies'
tick intervals overlap, and the checker found their output interleaved.

`gvals` lies at an odd address and one element crosses a page boundary; on i386VM that access goes
through the harness path that uses the first physical address only, and it is correct because each
copy's frames are one contiguous run.

## 4. Faults

| | Tiny386 PC / QEMU (guest handler) | i386VM (faultdump harness) |
|---|---|---|
| Copy 2 reads copy 1's frame address | `PGFAULT: vec 0e err 0 cr2 00380000 eip 00240679 … cr3 00125000`; copy 2 ends (−17) | `guest exception 14`; `FAULTDUMP: cr0 80000011 cr2 00900000 cr3 00025000 eip 00240679`; run ends |
| Copy 3 reads 0x01000000 | `cr2 01000000`; copy 3 ends (−17) | not reached |
| Copy 1, running beside both | exits 0x51; frames released | — |

err 0 = not-present read at CPL 0.  On i386VM the dump's CR3 equals copy 2's reported directory in all
three rounds.

## 5. Negative cases, reuse, rollback

On i386VM and Tiny386 ×3, and QEMU ×3: bad header −21, profile-2 image −61, missing file −44,
unterminated arguments −34, bad name pointer −46, bad info buffer −2, never-created id −2, stale id −2
after reclaim; held bytes, regions and records unchanged after each.  Eight launch/exit cycles per run
reuse the same frames and directories with full release each cycle.  TEST images: injected read failure
in the copy → −40, everything released, then a normal pair runs (i386VM, PC); 2 MiB pool → third launch
−60 (i386VM).

## 6. Interrupts around CR3 switches

TEST image `-DQDOS_TEST_PG_INT_AROUND_CR3`: a software `int 0x27` through the IDT immediately before and
after every CR3 load, inside the scheduler.  i386VM: 1,336 loads, 2,672 IRQ7 entries; PC: 1,315 loads,
2,630 entries; both SAMELNCH PASS.  This is synchronous: it shows the IDT path, stubs and stacks work
under both directories at exactly those points.  Asynchronous timer interrupts around switches are
covered statistically (about 1,300 switches per run with 18–24 timer preemptions per copy).

## 7. QEMU evidence and its limits

Scripted monitor keys; the 80×25 VGA text buffer is sampled from boot to the end; a command is taken as
finished when the bare shell prompt is the last row again.  Snapshot reads are not atomic and rows over
80 columns are cut, so the checker uses distinct lines, not counts, and marks NA: per-copy preemption
counts and deferred counts (end lines) and "held 0" in the after line.  On screen and checked: the
launcher verdicts, A/B directories and frames, pointer-check values, unpaged hold flag, overlap,
exits, the third-launch code, fault CR2/EIP, the negative codes, the repeat cycles and MULTI.

## 8. Unchanged default builds and the POC kernel's compatibility

* Default images: MULTI ×3 and NEG (848/0) ×3 on i386VM legacy, i386VM block and Tiny386 (18/18);
  the R11H regression script (R11F regression and pinned sequences, both emulators) with its volume
  unchanged; host unit tests BLOCK 88/0 and LAYOUT 39/0.
* POC kernels running ordinary (unpaged) programs: MULTI ×3 and NEG 848/0 ×3 on i386VM block and on
  Tiny386, MULTI ×3 on QEMU; the R11F regression sequence on the i386VM POC kernel gives exactly the
  default kernel's PASS/FAIL lines (no FAIL).
* Volume hashes (R11J, R11H release, R11H negative) identical before and after the matrix.

## 9. Not tested

* Failed-copy and interrupt-around-CR3 test images, and NEG, under QEMU.
* A poll/schedule-list task registered by an unpaged program while paged copies run (no removal
  service; code-reviewed only).
* A multi-byte access straddling the window end on i386VM (see the known limitations).
* Zork I (story unavailable).
