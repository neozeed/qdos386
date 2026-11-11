# Configuration (canonical R18/R20 PC baseline)

A build is a set of `-D` macros plus a platform (PC headless or PC VGA). Options are cumulative: each release flag
includes the previous one through `Makefile` variables (`R13A_OPTS` → `R13B_OPTS` → `R14_OPTS` → … → `R18_OPTS` →
`R20_OPTS`). The macro set of each supported build is in `docs/R20C-CONDITIONAL-INVENTORY.md`.

## Supported builds

| target | added macros | output |
|---|---|---|
| `make qdos386-r18-pc` | `-DQDOS_CON_VGA` for the VGA output only, plus the R18 chain | `build/qdos386-r18/pc/`, `build/qdos386-r18/pc-vga/` |
| `make qdos386-r20-pc` | the R18 chain plus `-DQDOS_R20` | `build/qdos386-r20/pc/`, `build/qdos386-r20/pc-vga/` |

Both targets pass `-DQDOS_LAYOUT_R9 -DR7_SHELL_MODE`. `QDOS_CON_VGA` selects the VGA console and presentation; without it
the console writes only to the debug port 0x402.

## Flags in the supported builds

| macro | selected by | meaning | defined in / documented in |
|---|---|---|---|
| `QDOS_LAYOUT_R9` | both | memory layout of the R9 application slots | `kernel/appmap.h` |
| `R7_SHELL_MODE` | both | the built-in shell job | `kernel/shellboot.c` |
| `QDOS_CON_VGA` | VGA builds | VGA text console and presentation | `platform/pc/vga.c`, `docs/DISPLAY.md` |
| `QDOS_APP_STACK=65536u` | both | application stack size | build flag |
| `QDOS_FS_WRITE` | both | FAT16 write support (required by VCON, SYNC, VFAT) | `kernel/fat16.h`, `kernel/channel.h` |
| `QDOS_VFAT` | both | VFAT long file names | `kernel/fat16.h` |
| `QDOS_VCON` | both | virtual text consoles (needs `QDOS_FS_WRITE`) | `kernel/vcon.h` |
| `QDOS_JOBCTL` | both | job control records and kill | `kernel/jobctl.h`, `docs/R11M-JOB-CONTROL-ABI.md` |
| `QDOS_TITLES` | both | job titles | `kernel/jobctl.h` |
| `QDOS_TASKMGR` | both | the task manager job | `kernel/taskmgr.c` |
| `QDOS_SURFACES` | both | display surfaces, `QT_SURFACE` (needs `QDOS_VCON`, `QDOS_JOBCTL`, `QDOS_TASKMGR`) | `kernel/surface.h`, `docs/R13-DISPLAY-ABI.md` |
| `QDOS_SYNC` | both | storage sync (needs `QDOS_FS_WRITE`) | `kernel/sync.h`, `docs/R12C-SYNC-DURABILITY.md` |
| `QDOS_ENV` | both | per-process environment | `kernel/env.h`, `docs/R12B-ENV-PROCESS-ABI.md` |
| `QDOS_PAGING` | both | paging and per-process address spaces | `kernel/paging.h` |
| `QDOS_DYNPROC`, `QDOS_DYNPROC_SHELL` | both | dynamic process table (`JOB_MAX` 16, then 24 with R17) | `kernel/jobs.h`, `kernel/proc.h` |
| `QDOS_R13A`, `QDOS_R13B` | both | R13 graphics and clock ABI | `docs/R13A-GRAPHICS-ABI.md`, `docs/R13B-CLOCK-ABI.md` |
| `QDOS_R14` | both | serial port (UART) support and R14 channel behaviour | `kernel/uart.h`, `docs/R14-SERIAL-ABI.md` |
| `QDOS_R16`, `QDOS_R16B` | both | CPL3 jobs, the BKL, two-CPU scheduling; per-CPU accounting and migration | `docs/R16-ARCHITECTURE.md`, `docs/R16B-SCHEDULER-POLICY.md` |
| `QDOS_R17` | both | E820 memory map, up to 8 CPUs (`KX_NCPU` 8), `JOB_MAX` 24 | `docs/APOLLO13-MEMORY-MAP.md` |
| `QDOS_R18` | both | 640×480×16 planar presentation views (`GS_VIEW_VGA12`, `GS_VIEW_VGA12W`) | `kernel/gsurf.h`, `docs/APOLLO14-DISPLAY-ABI.md` |
| `QDOS_R20` | R20 only | the Ring 0 storage worker and the bounded sector cache | `docs/A16-CACHE-DESIGN.md` |
| `QDOS_KCTX_PTRBITS` | not set | historical context-switch encoding of the older `kctx.o` targets only | `Makefile` (`PCKCTX`) |

R20 test-only switches (not set in any build): `R20C_INJECT_EVERY` (device failure injection), `R20C_TEST_ENT` (cache entry
count for contention tests), `R20_RSV_MAX` (reservations per reader; the default 4 is used), `R20X_N`, `R20X_GIVEUP_IDX`,
`R20D_WAIT`, `R20K_WAIT` (test program parameters, set only by `scripts/a16-p2b-build.sh` and its variants).

## Rules

- A new option is a new macro with its own `#if` and a default that keeps the supported builds unchanged.
- A flag that another flag needs must be checked in the code that uses it (for example `QDOS_SYNC` needs `QDOS_FS_WRITE`).
- Changing the flag list of a supported build is a functional change: it needs its own branch and its own identity checks.
