# R13 memory budget — sessions and surfaces

Measured on the R12C kernel with the example system volume (Phase 0, `logs/r13/phase0/`, `PINFO` and `LEAK` at the
shell prompt):

| machine | RAM | frame pool | free frames | kernel heap free |
|---|---|---|---|---|
| QEMU (`-m 16`) | 16 MiB | 3,264 frames (12.75 MiB) | 3,201 | 777,536 bytes |
| Tiny386 (`mem_size = 8M`) | 8 MiB | 1,216 frames (4.75 MiB) | 1,153 | 774,096 bytes |

The kernel heap (below 2 MiB, identity-mapped in every address space) is the same size on both machines; the frame
pool scales with RAM. Process images, heaps and stacks use frames; kernel objects (channels, environments, console
sessions) use the heap.

## Where R13 memory comes from

* **Legacy consoles 0..3**: static, as in R12C (four `struct vcon`, ~8.3 KiB each, in kernel BSS). Unchanged.
* **Private text sessions** (TEXT80x25): one kernel-heap block per session, the same `struct vcon` (80x25 cells of
  2 bytes, input queue 128, cooked line 121, cursor, region, mode) — 8.4 KiB including the heap header. Created
  only on request (`START … /NEW`, `qd_surface_new`, or a graphics open on a shared session); freed when the last
  job of the session ends. Noninteractive background jobs keep the R12C rule (they share their creator's session)
  and cost nothing.
* **Graphics surfaces** (INDEX8): one kernel-heap block per surface, header + `stride × height` pixels + 768-byte
  palette. 320x200 (the only size the presenter shows full screen) = 64,000 + 768 + header ≈ 64.9 KiB. Smaller
  surfaces cost proportionally less. Allocated only by an explicit graphics open; freed at close, exit, kill or
  fault.

## Limits (fixed, enforced, tested)

| resource | limit | why |
|---|---|---|
| sessions in total | 20 (4 legacy + 16 private) | more than the job table can use: JOB_MAX 16 leaves 13 started jobs, so at most 13 private sessions exist (Gate 1 reaches it); FULL stays as a guard |
| graphics surfaces at once | 4 | 4 × 64.9 KiB = 260 KiB of the ~775 KiB heap |
| surface size | 1..320 × 1..200, stride = width rounded up to 4 | the presenter's mode 13h screen |
| one blit/fill/read call | at most one surface (64,000 bytes) | bounded copy; it runs inside the trap with interrupts off (as every R12 trap) |

Planning estimate (Phase 0) for the table limits (16 private text sessions + 4 full graphics surfaces): 16 × 8.4 +
4 × 64.9 ≈ 394 KiB of surfaces and sessions, plus the jobs' own kernel memory. A request beyond a limit, or a heap
allocation that fails, is refused with an error and changes nothing (Gate 1 / Gate 4 tests); every block is
returned when its owner ends (LEAK and PINFO back at the baseline).

## Measured (R13 kernels, `LEAK heap=` = free kernel heap in bytes)

| situation | free heap | in use vs idle |
|---|---|---|
| QEMU, shell idle (R13 VGA kernel) | 754,416 | — |
| QEMU, 4 INDEX8 surfaces (COLOUR, BOUNCE, 2 × SURFT gfx) + 1 text panel = 5 private sessions, 5 started jobs (Gate 4) | 389,632 | 364,784 |
| QEMU, 12 private text sessions + 12 started jobs (Gate 1; LEAK as the 13th) | 501,168 | 253,248 (≈ 21 KiB per job on its own session) |
| Tiny386, shell idle (R13 headless kernel) | 762,608 | — |

So a job on a private session costs its session (8.4 KiB) plus the job's own kernel memory (kernel stack, channels,
environment: about 12 KiB per job in the Gate 4 figure after the surfaces and sessions are taken off), and a 320×200
surface about 65 KiB. The real worst case is bounded by the job table (13 started jobs, so at most 13 private
sessions) and the 4-surface limit; with the Gate 4 figure plus 8 more jobs on private sessions (≈ 8 × 21 KiB) about
220 KiB of heap stays free. Both measured situations returned to 754,416 bytes exactly afterwards.

## Presenter

The presenter keeps no copy of any surface: it copies the foreground surface's dirty rows straight to `0xA0000`
(or the text cells to `0xB8000`). Its own state is the saved mode-3 register set (~70 bytes), the saved DAC palette
(768 bytes) and the saved text font (8 KiB, plane 2) — static kernel data, ~9 KiB. The Minerva font table is
2,304 bytes of kernel data (864 for QDOS5x9).
