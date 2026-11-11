# QDOS/386 Apollo 13 (R17) — memory expansion and multicore topology: handover

**Package**: `qdos386-APOLLO13-R17-MEM-MULTICORE.tar.gz`. Public-safe: no SuperBASIC, no Infocom, no Elite.

**Source**: branch `r17-apollo13-mem-multicore`, on top of `r16b-foreground-smp` (479951d) plus the verification
commit bf7d350. Each gate is tagged:

| Tag | Content |
| --- | --- |
| `a13-gate0` | audit |
| `a13-gate1` | memory |
| `a13-gate2` | N CPUs |
| `a13-gate3` | topology/HT |
| `a13-gate4` | stress |
| `a13-r17` | release |

## What R17 is

An **opt-in** kernel target, `make qdos386-r17-pc`, which builds `qdos386-r17-vga.img` (SHA-256 prefix `40968de4`)
and `qdos386-r17.img` (`f53bd88a`).

The frozen targets are byte-identical to their previous releases (rechecked after every phase):

| Target | Image | SHA-256 prefix | Release |
| --- | --- | --- | --- |
| `qdos386-pc` | `qdos386-vga.img` | `f8e5d5e1` | KCTX-FIX2 |
| `qdos386-pc` | `qdos386.img` | `7c675978` | KCTX-FIX2 |
| `qdos386-smp-pc` | `qdos386-smp-vga.img` | `b9ecb69a` | R16A |
| `qdos386-r16b-pc` | `qdos386-r16b-vga.img` | `d073fbea` | R16B |

1. **Memory: up to 256 MiB from a validated E820 map**
   * The new QBOOT3 boot sector collects INT 15h E820. The kernel validates the map: reserved wins, page-aligned
     inwards, nothing below 1 MiB, nothing at or above 256 MiB.
   * It extends the supervisor-only identity map and the frame pool. Without a map it falls back to the R16B CMOS
     16 MiB rule.
   * Ring 3 protection is unchanged. User pages are U/S user, kernel mappings are supervisor-only, and pool spans are
     not present in process directories.
   * See `docs/APOLLO13-MEMORY-MAP.md`.
2. **Up to 8 logical CPUs, one BKL**
   * APs start sequentially with a barrier. Each AP has its own TSS, stack, halting idle job, LAPIC timer and
     accounting.
   * An AP that does not start is held with INIT and never scheduled.
   * CPU 0 keeps every device IRQ.
3. **Topology and the HT policy**
   * Each CPU probes itself with CPUID leaf 0xB, or leaf 1 + 4. The results are cross-checked against the LAPIC and
     MADT. Without proof the topology is `unknown`, the policy is flat, and no sibling is invented.
   * Foreground first: the foreground session's job runs on CPU 0. Background jobs go to the least-loaded allowed CPU.
   * HT modes `AUTO` (distinct cores first, CPU 0's sibling last), `PHYSICAL` (siblings parked) and `ALL`.
   * See `docs/APOLLO13-TOPOLOGY.md`.

## Build and run

```sh
export QDOS386_TOOLS=/path/to/i386vm-tree   # the GCC 1.40 / GAS 1.38 / i386-ld toolchain (see README.md)
make qdos386-r17-pc
sh scripts/a13-build-guest.sh                  # MEMINFO RAMCHK SMPT TOP17 CPUSET17 (+ R16T, BOUNCEP)
sh scripts/a13-sysvol.sh vol.img               # public-safe volume with the tools in /BIN
qemu-system-i386 -drive file=build/qdos386-r17/pc-vga/qdos386-r17-vga.img,format=raw,if=ide,index=0 \
                 -drive file=vol.img,format=raw,if=ide,index=1 -m 128 \
                 -cpu Nehalem -smp 8,sockets=1,cores=4,threads=2 -accel tcg,thread=multi
```

The packaged kernels are in `kernels/`. The example volumes `examples/system-r17-{superfloppy,mbr}.img` already
contain the tools.

Shell commands for R17:

| Command | What it does |
| --- | --- |
| `MEMINFO` | E820 records, pool and high-memory counters |
| `RAMCHK n [passes]` | CPL3 RAM pattern test of n MiB |
| `TOP [n] [s]` | per-CPU line with package/core/thread, cpl3/cpl0/idle %, load, PARKED |
| `CPUSET HT [AUTO\|PHYSICAL\|ALL]` | set or show the HT mode |
| `CPUSET POLICY FG\|OFF` | set the scheduling policy |
| `CPUSET <jobid> <0..7\|AUTO>` | pin a job to a CPU, or return it to the policy |
| `SMPT INFO\|WORK\|MIG\|FAULT\|KILL\|SPAWN\|HTCHK\|BUSY` | SMP tests |

## Evidence

The `evidence/` summaries are produced by the scripts in `tests/a13`. Development logs are not shipped. The detailed
results are in `docs/APOLLO13-RUNTIME-REPORT.md`. Package verification is `scripts/a13-verify-package.sh` (fresh
extraction: manifest, clean rebuild bit-for-bit, frozen rebuilds, guest tools, volume audit, QEMU smoke).

## Files

| Area | Files |
| --- | --- |
| Boot / memory | `platform/pc/boot3.s`, `platform/pc/make-image3.py`, `kernel/memmap.[ch]`, `kernel/proc.c` (R17 blocks), `kernel/frames.h`, `kernel/vas.h`, `kernel/proc.h` |
| SMP | `kernel/smp17.c` (replaces `smp.c` in R17), `kernel/kctx17.s`, `kernel/smp_s17.s`, `kernel/uart_s17.s`, `kernel/descriptors.c` (per-AP TSS), `kernel/jobs.[ch]`, `kernel/bkl.[ch]` (`KX_NCPU`), `kernel/taskmgr.c` (CPU 0…7) |
| Tests | `tests/a13/*.c`, `*.seq`, `run-soak.sh`, `joinlines.py`; `scripts/a13-*.sh` |
| Docs | `docs/APOLLO13-AUDIT.md`, `APOLLO13-MEMORY-MAP.md`, `APOLLO13-TOPOLOGY.md`, `APOLLO13-RUNTIME-REPORT.md`, `APOLLO13-KNOWN-LIMITATIONS.md` |

The patch against R16B is in `patches/r17-against-r16b.patch`.

## Constraints kept

* No source changes to GCC, QLTerm, Hack, Elite or GW-BASIC.
* No private SuperBASIC, Infocom or Elite assets in the package. The packaging script refuses them.
* The BKL is kept; no unrelated subsystem was fixed.
* CPU 0 owns external IRQs, with no IOAPIC redistribution.
* No MMIO or reserved frame is ever allocated, and no 4 GiB claim is made.
* Firmware settings are untouched.
* No job is double-run. Each job is dispatched only by the CPU named in its affinity, under the BKL, and the R16B
  `running_on` handshake is unchanged.

## Next steps (not started, as instructed)

* Splitting the BKL.
* A kernel mapping window or PAE for more than 256 MiB.
* IOAPIC routing.
* AMD topology (leaf 0x8000001E).
* Testing on real hardware.
