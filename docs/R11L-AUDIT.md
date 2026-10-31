# R11L Phase 0: audit of what native GCC 1.40 self-hosting needs

Baseline: R11K `82405025f873dcf13dea8bf352399add195ebaca` (frozen). Branch `r11l-selfhost`.
Classification used throughout: **host build** / **guest executed** / **independent comparison** / **not tested** / **blocked**.
Per-symbol dependencies: `docs/R11L-DEPENDENCIES.csv` (`.md` for the narrative).

## 1. Pipeline and per-stage sizes

`hello.c -> cccp -> hello.i -> cc1 -> hello.s -> as -> hello.o -> ld (CRT0.O + libs) -> hello.aou`

| Item | Measured |
|---|---|
| AS.AOU (GAS 1.38) | text 77 K, data 16.8 K, bss 40 K, file 109,702 B |
| CCCP.AOU | 59 K / 1.9 K / 18 K, file 71,317 B |
| CC1.AOU | 463 K / 7.5 K / 23 K, file 534,494 B |
| LD.AOU (binutils-1.94) | 107 K / 3.4 K / 17.7 K, file 134,747 B |
| HELLO.C / .I / .S / .O / .AOU | 152 B / 2,237 B / 220 B / 180 B / 27,973 B (HELLO.AOU is mostly libc + libqdos) |
| link inputs (volume) | CRT0.S 1,676 B, LIBC.A 43,798 B, LIBM.A 8,882 B, LIBGCC.A 3,500 B, LIBQDOS.A 4,170 B, SLOT1.LD 370 B, 16 SDK headers (qdos386.h 11,977 B) |
| FAT16 scratch volume | 8,143 clusters x 4 KiB (about 32 MiB); the full tool set + SDK + test files uses about 2,400 clusters |

## 2. Runtime resources of each tool (MEMSTAT builds, i386VM; maximum over the corpus HELLO MACROS FUNCS FPT LSTDIO)

MEMSTAT builds (`scripts/r11l-build-ms.sh`) add counters to `malloc`, the stack and channels; they are measurement-only and never
used in an identity run. Heap = bytes of live `malloc` blocks at the peak; stack = deepest use of the 64 KiB job stack.

| Tool | heap peak | largest block | `malloc` calls | open channels | stack peak |
|---|---|---|---|---|---|
| cccp | 23.6 KB | 9.5 KB | 156 | 4 | 5.9 KB |
| cc1 | 176.8 KB | 4.1 KB | 124 | 2 | 14.1 KB |
| as | 203.7 KB | 64.0 KB (obstack chunk) | 445 | 1 | 2.4 KB |
| ld | 197.0 KB | 16.0 KB | 1,219 | 9 | 7.2 KB |

All fit the shared 1.9 MiB kernel heap with a wide margin. The measured cc1 stack peak (14.1 KB) is close to the stock 16 KiB job
stack, so R11L kernels use 64 KiB (`-DQDOS_APP_STACK=65536u`; R11K kernels are unchanged).
Open channels peak at 9 (`CHAN_MAX` is 16 with `-DQDOS_FS_WRITE`; 8 without, which `ld` would not have fitted).

## 3. Blockers found and how each was resolved

| # | Blocker (measured) | Resolution | Status |
|---|---|---|---|
| B1 | FAT16 was read-only: the pipeline writes `.i .s .o .aou` | opt-in writable FAT16 (`-DQDOS_FS_WRITE`), new traps 0x2A-0x2C, write-through ordered writes, fault-injection (107 cuts) | solved, Stage 1 |
| B2 | tools use `open/read/write/lseek/fstat/unlink/...`, `freopen`, `fdopen`, `fileno` (22 `posixio` and 66 `stdio` symbols, 191 rows in the CSV) | `sdk/libc/posixio.c`, `stdio.c` extensions, 12 descriptors | solved |
| B3 | the shell line is 79 characters; the `ld` command line is 150+ characters | `shell-l` (159 characters) for hand-run sequences; `CC.AOU` builds the line itself | solved |
| B4 | arguments: 432 bytes / 15 arguments; `ld` needs 11 | fits; the driver stays below these limits (checked: error status 2 otherwise) | solved |
| B5 | one slot-1 window of 1 MiB; CC1 needs 463 K + 23 K bss | fits; a driver must itself occupy slot 1 | solved |
| B6 | a program cannot launch another slot-1 program while it occupies slot 1 | R11K paging launcher: the driver starts each tool as a paged child in its own 1 MiB window and reads its exit status (`qd_job_exit_status`) | solved (needs a paging kernel) |
| B7 | job stack 16 KiB | 64 KiB in R11L kernels (measured need: 14.1 KB for cc1) | solved |
| B8 | historical LP64-only code in the legacy tree; `struct stat` layout mismatch; libc gaps (`sys_errlist`) | see `R11L-TOOLCHAIN-PORT.md` | solved |
| B9 | ld reports "multiple definition" yet exits 0; `as` exits 0 on a missing input | link script and driver check outputs and ld stderr | handled |
| B10 | interpretive Tiny386 speed was suspected after a linker run seemed to hang | the "hang" was the Tiny386 terminal dropping part of a long typed line (see `R11L-KNOWN-LIMITATIONS.md`); with paced input the whole four-tool pipeline, including `ld`, runs on Tiny386 in seconds to a few minutes | solved (harness issue) |
| B11 | no directories; the flat root holds 94+ files during a full build | SDK headers copied to the root, `-I.`; a build uses about 2,400 of 8,143 clusters | accepted limitation |
| B12 | string-pool difference between guest cc1 and the oracle host cc1 | documented, compared as "equivalent modulo pool"; HELLO identical | accepted |

## 4. Architecture decisions

* The driver is an **ordinary slot-1 program**; the tools run as **paged children**. A paged child cannot spawn, which is fine
  because the tools do not. Tools run strictly one after another; heaps and stacks are shared, so the build makes no isolation claim.
* Paging stays opt-in. Default (non-paging) images are byte-identical to R11K and R11I (`scripts/r11l-default-check.sh`).
* No CPU core change, no new privilege levels.
