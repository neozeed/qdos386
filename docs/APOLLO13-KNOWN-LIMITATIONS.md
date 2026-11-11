# Apollo 13 (R17) — known limitations

These are deliberate scope limits or measured behaviour, not hidden failures.

## Memory

* **256 MiB ceiling.** The kernel reaches frames only through the identity map, and the user heap window begins at
  0x10000000. RAM above 256 MiB is detected, reported (`… KiB above it not used`) and ignored. Lifting the ceiling needs
  a kernel mapping window or PAE, which is a design change; no claim of 4 GiB support is made.
* **E820 only.** A kernel started by any boot loader other than QBOOT3 (e.g. a QBOOT2 sector written by an older
  image tool) finds *no map* and falls back to the R16B CMOS rule (16 MiB). EFI memory maps, E801 and INT 15h 88h are
  not used.
* **Records above 4 GiB** are ignored. They could not be mapped anyway.
* **Physical memory is not tested at boot** beyond the E820 map. RAMCHK at CPL3 is the test.
* **The R17 kernel area (0x680000–0x6FFFFF) is fixed at 512 KiB**: 60 high template tables plus 7 AP stacks fit with
  room to spare. It is no longer pool, so `-m 16` has 3,104 pool frames against R16B's 3,264. This figure also
  excludes reserved ACPI pages that R16B pooled.
* **Per-process limits**: 64 MiB heap, 32 private page tables. The job table holds 24 jobs (R17), against 16 before.
* **No DMA, no bounce buffers.** Disk I/O is ATA PIO, so the 16 MiB ISA DMA limit does not apply.

## CPUs and scheduling

* **The BKL is kept.** All kernel work is serialised. More CPUs add CPL3 throughput, not kernel throughput. BKL
  contention (`cont=` in TOP) grows with the CPU count.
* **At most 8 logical CPUs** (`KX_NCPU`). Additional MADT entries are counted and not started.
* **Device IRQs stay on CPU 0** (PIC). There is no IOAPIC programming and no interrupt redistribution, by design.
* **Topology is reported `unknown` (flat)** without CPUID leaf 0xB or leaf 1+4, e.g. on `-cpu pentium` (the test harness default). AMD leaf 0x8000001E is not decoded, so AMD SMT is treated as flat. HT modes AUTO and PHYSICAL
  then return -97.
* **The policy ranks by load, not by measured throughput.** Load is the number of placed protected jobs, so a job that
  sleeps counts as much as one that spins.
* **LEND** (R16B's two-CPU borrowing) is not available in R17. Policy is FG or OFF.
* **Ring-0 jobs always run on CPU 0** (unchanged).
* **The shared `#DF` task gate**: a double fault on two CPUs at once is not handled (unchanged from R16B).
* **A failed AP** is held with INIT and never retried at run time.
* **TCG timing**: measurements are from QEMU TCG, multi-threaded, on a 2-core host with 8 vCPUs. Percentages are
  tick-sampled. Absolute speeds and contention figures are not representative of hardware. No real hardware was tested
  in this session.

## Shutdown

* QEMU's `pc` machine does not decode the 0x8900 shutdown port. `SHUTDOWN` therefore reports `no response` (status 2),
  as documented for R11M. The soak first checks `SHUTDOWN /N` (no job left: safe to stop; the file system is
  write-through), then lets the harness stop QEMU and runs `fsck.fat -n` on the volume.

## Not in scope (as instructed)

Apollo 14 graphics, BKL splitting, DOS virtualisation, networking, DOOM, QDOS/040 and userland rewrites. EliteQL stays
a separate private application. It is not part of this package, and its source was not changed.
