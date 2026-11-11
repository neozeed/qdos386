# Apollo 13 (R17) — Phase 0 audit of the R16B limits

Baseline: tag `r16b-foreground-smp` (`479951d`) + verification commit `bf7d350`; branch `r17-apollo13-mem-multicore`.
Frozen outputs (must stay byte-identical): `make qdos386-pc` → `qdos386-vga.img` `f8e5d5e1…` / `qdos386.img`
`7c675978…` (KCTX-FIX2); `make qdos386-smp-pc` → `qdos386-smp-vga.img` `b9ecb69a…` (R16A); `make qdos386-r16b-pc` →
`qdos386-r16b-vga.img` `d073fbea…` (R16B).

## Baseline smoke (R16B kernel, QEMU `-smp 2 -accel tcg,thread=multi -m 32`, logs/a13/p0)

`R16T RACE` (CPL3 children on CPU 1 and CPU 0) done, both status 0; `R16T MIGTEST 20 1` MATCH (`0de23a2d`, 20 flips,
0 errors); `CPUSET POLICY` FG; protected BOUNCE started on a new session (focus switch) and back to the shell; `TOP`
`badrel=0`; no PGFAULT; LEAK `heap=664448 chans=15 spaces=2 held=266240` before (the second LEAK differs only because
BOUNCE was still running: `spaces=3`).

## Memory: measured limits

| Item | R16B value | Kind | Source |
| --- | --- | --- | --- |
| RAM discovery | CMOS 0x30/0x31 extended-memory KiB + 1 MiB; no E820, no firmware map | runtime (CMOS) | `kernel/proc.c` `proc_boot` |
| Physical ceiling | **16 MiB** (`FR_LIMIT 0x01000000`); RAM above is ignored, a CMOS value of 0 assumes 8 MiB | compile time | `kernel/frames.h`, `proc.c` |
| Frame database | two bitmaps of `FR_NFRAMES` = 4,096 bits (`member`, `used`) | compile time | `kernel/frames.h/c` |
| Kernel identity map | template directory + `TPL_DIRS` = 4 page tables (0–16 MiB, supervisor) in kernel BSS (`tpl_mem`) | compile time | `proc.c`, `vas.c` |
| Pool ranges | 0x200000–0x33FFFF, 0x380000–0x6FFFFF, 0x800000–RAM top (≤16 MiB), every frame presence-tested | compile time | `proc_boot` |
| Kernel heap | `bss_end` (R16B 0x1539B0) … `HEAP_END` = 0x200000 (≈ 690 KiB: kernel stacks, channels, surfaces) | link time | `appmap.h`, `memory.c` |
| Per-process spaces | directory + ≤ `VAS_MAX_TABLES` = 16 private tables; heap window 0x10000000 + `QDOS_PROC_HEAP_MAX` 8 MiB; stack 0x1FFF0000–0x20000000 | compile time | `vas.h`, `proc.h` |
| Pool-span handling | a 4 MiB span that is all pool is *not present* in a process directory; a partly-pool span gets a private table copy with the pool PTEs cleared (kernel touches pool frames only with the template active) | design rule | `vas.c` `vas_create` |
| DMA | none (ATA PIO, no ISA/bus-master DMA in `platform/pc/disk.c`) | — | — |
| Boot loader | `boot2.s` (QBOOT2): INT 13h reads only; no memory map is captured | — | `platform/pc/boot2.s` |

Architectural constraint (32-bit, no PAE): the kernel reaches a frame only through the identity mapping of the template
directory, and process windows begin at 0x10000000 (heap) and 0x1FFF0000 (stack). The identity map can therefore grow
to **256 MiB** (directory entries 0–63) without touching any user window; beyond that the identity spans would overlap
the heap window, which would need a kernel mapping window (not attempted). PAE is not used (the kernel and every page
table are 32-bit 2-level).

## CPUs: measured limits

| Item | R16B value | Kind | Source |
| --- | --- | --- | --- |
| CPUs used | 2 (BSP + the **first** non-BSP APIC id in the MADT/MP list) | compile time | `smp.c` `smp_boot`, `ap_id` |
| Enumeration | MADT (enabled LAPIC entries) or MP table; `cpu_ids[16]`; no topology | runtime | `smp.c` `detect_cpus` |
| Per-CPU arrays `[2]` | `g_jobcur`, `g_jobidle`, `kx_pc[2][4]`, `g_curcr3`, `g_excdepth`, `bkl_depth/acq/rel/spins/wait_kc/contended`, `st_ticks/cpl3/cpl0/idle/ctxsw/migr` | compile time | `jobs.c`, `proc.c`, `bkl.c`, `smp.c` |
| CPU identity | `str`: TR 0x18/0x20 → CPU 0, TR 0x38 → CPU 1; EBP = 16·cpu indexes `kx_pc` in 10 assembly entry stubs | compile time | `kctx3.s`, `smp_s.s`, `uart_s3.s` |
| GDT | 64 bytes: …, user code 0x28, user data 0x30, **one** AP TSS 0x38 | compile time | `descriptors.c` |
| AP TSS / stack | one `kx_tss_ap`, one 16 KiB `ap_stack`, one shared `smp_ap_esp` | compile time | `descriptors.c`, `smp.c` |
| AP start-up | INIT, SIPI (+1 retry), poll `smp_ap_entered` 200 ms; trampoline at 0x9000 | runtime | `smp.c`, `smp_s.s` |
| AP timer | LAPIC periodic, vector 0x40, count calibrated once against PIT ch. 2 | runtime | `smp.c` |
| AP idle job | `IDL1`: `for (;;) ;` (busy loop) | — | `smp.c` |
| Device IRQs | PIC on CPU 0 only; no IOAPIC programming | design | — |
| `#DF` | one task gate / TSS (0x20) shared by all CPUs | compile time | `descriptors.c` |
| Policy | FG: foreground session → CPU 0, every other protected job → CPU **1**; LEND: CPU 0 borrows from CPU **1** | compile time | `smp.c` |
| Stats ABI | `QT_SMP STATS` v1: 2 per-CPU blocks of 16 words at word 10 + 16·c | ABI | `smp.c`, `tests/r16b/top.c` |
| BKL | owner word = cpu + 1, `LOCK CMPXCHG` acquire, recursive, owner-checked release (`badrel`); `job_exit` takes it (R16B fix) | — | `bkl.c`, `bkl_s.s`, `jobs.c` |

## Change inventory (all opt-in: `make qdos386-r17-pc`, `-DQDOS_R17`; frozen targets unchanged)

1. **Boot loader `boot3.s` (QBOOT3)** = QBOOT2 + an INT 15h E820 loop into 0x504 (≤ 32 entries, count/end at 0x500),
   tagged so the kernel can tell it from QBOOT2. Only the R17 image uses it.
2. **Memory map** (`kernel/memmap.c`): capture, validate (size, overflow, overlap, reserved-wins), print, fall back to
   the CMOS/16 MiB rule when the map is missing or invalid. Usable RAM up to the 256 MiB ceiling joins the pool.
3. **Frame database** sized for 256 MiB (`FR_LIMIT` per build), template tables for the spans above 16 MiB allocated
   from a reserved kernel area (0x680000–0x6FFFFF, no longer pool in R17), larger per-process heap and table limits.
4. **N CPUs** (`KX_NCPU` = 8 in R17, 2 otherwise so R16A/R16B code is unchanged): every `[2]` array; GDT with one TSS
   per AP (0x38 + 8·(n−1)); TR → CPU index in the assembly stubs (R17 copies of `kctx3.s`, `smp_s.s`, `uart_s3.s`); per-
   AP stack, TSS, idle job (halting), LAPIC timer; sequential INIT/SIPI with a start-up barrier; an AP that does not
   arrive is never scheduled.
5. **Topology** (`kernel/topo.c`): per-CPU CPUID (leaf 0xB, else leaf 1 + 4) run on each CPU itself, cross-checked with
   the MADT list; `unknown` → flat policy.
6. **Policy**: FG keeps the foreground on CPU 0; background protected jobs go to the least-loaded allowed CPU, ordered
   by the HT mode (`PHYSICAL` parks siblings, `AUTO` fills distinct cores first, `ALL` uses every logical CPU).
   `QT_SMP` ops for HT mode and topology; STATS v2 for up to 8 CPUs; TOP/CPUSET updated (v1 still understood).

## Risks

* Extending the identity map makes every kernel access to a high frame depend on the template being active (already
  the R12A rule); a violation would #PF in the kernel. Mitigation: the high spans are *not present* in process
  directories by construction, and the Ring-3 RAM checker exercises allocation, use and teardown of high frames.
* 8 vCPUs on a 2-core host under TCG: slow, timing-sensitive tests; a busy-looping idle job would starve the host.
  The R17 AP idle job halts.
* Shared `#DF` TSS across CPUs (existing); a double fault on two CPUs at once is not handled (documented).
* The BKL serialises all kernel work: more CPUs add contention, not kernel throughput.
