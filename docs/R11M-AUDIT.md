# R11M Phase 0: audit for the "usable OS" stretch (directories, virtual consoles, job control, VFAT)

Baseline: R11L `81d29ea900b60955837cbb394ea6176e9ffa9fa1` (frozen, branch `r11l-selfhost`). Work branch `r11m-os-usability`.
Classification used throughout: **host build** / **guest executed** / **independent comparison** / **not tested** / **blocked**.

R11M is an OS-facilities milestone, not a toolchain milestone. The native GCC pipeline is not rebuilt or re-run as a routine test;
the frozen R11L evidence stands, plus tiny host-cross-built guest programs.

## 1. Environment and hashes

| Item | Value |
|---|---|
| Source baseline | `81d29ea900b60955837cbb394ea6176e9ffa9fa1` (R11L) |
| Frozen earlier rounds | R11H-FIX1 `4affc37`, R11I `bc9a90f`, R11J `f7664d7`, R11K `82405025...`, R11L `81d29ea` |
| Historical compiler | GCC 1.40 + GAS + i386-ld tree (`i386-gcc` sha256 `3567f3be...`, `i386-ld` sha256 `dfdb3965...`) |
| i386VM harness (block, read-write) | `i386vm-r9-blockrw` sha256 `562f51ab...`; `i386vm-r9-block` sha256 `5970e162...` |
| Tiny386 | `tiny386_headless` sha256 `e201502f...` |
| QEMU | 8.2.2 (`qemu-system-i386` sha256 `28fa14f1...`), SeaBIOS, `-vga std`, `-cpu pentium`, `-m 16` |
| Independent FAT judges | `fsck.fat` 4.2, mtools 4.0.43, `tools/r11l-fatcheck.py` (root only), `tools/r11m-fatwalk.py` (recursive, new) |

Frozen default images that must stay byte-identical (checked by `scripts/r11l-default-check.sh`, and in every stage):

| Image | sha256 |
|---|---|
| r11i-block | `3ee37c19ac52acbc186bab89dbf06f20af67f4543d1d479d91c17f1bbbc17400` |
| r11k-vm-paging | `2e61c4aee1d05e5dcb18fb40e9aec8ea068afa216e44099d310b583e9c83730a` |
| r11k-pc-default | `2067b3e810cf295f6c7f6d5a8ca5849133b342ff385f65f2a63733a08a5a202c` |
| r11k-pc-paging | `3a09141da237c85c1f44b9a843a72d6bb88baaecf1dfdfdbacdf74154129617f` |
| r11h-pc | `8f22b0b2613d05543f8a174f341c772ffccc15a56e7aa53264ff0b8d6f512307` |
| r11h-rel-vm | `c5462138ea630fa73d5ca82c69d08f79e7a919eda2e7f11b4db5c2fd358af03a` |

Every R11M kernel feature is opt-in by a macro (`QDOS_FS_WRITE` for directories, later `QDOS_VCON`, `QDOS_JOBCTL`), so images built
without the macros contain none of the new code. The same technique `vga.c` already uses (a source file that compiles to nothing
unless its macro is defined) keeps the Makefile source lists unchanged.

## 2. What exists today (measured by reading the sources)

### 2.1 Filesystem (Stage 1 and 4)

| Fact | Source |
|---|---|
| FAT16 only, 512-byte sectors, two FAT copies, root directory in the fixed region. | `kernel/fat16.c` |
| R11L writes are write-through and ordered (data, FAT copy 1, FAT copy 2, directory entry); no crash-atomicity promise. | `docs/R11L-FAT-WRITE-ABI.md` |
| Before R11M every open, remove, rename and lookup is root-only. A name is a flat 8.3 name. | `fat16_open_ex`, `fat16_remove`, `fat16_rename`, `fat16_lookup` |
| One open-file registry keyed by the directory entry's (lba, offset): any number of readers xor one writer. | `kernel/win1.c` `openf[]` |
| Channel names are `WIN1_NAME` (a 5-character driver prefix plus an 8.3 name, at most 18 characters). | `kernel/channel.h`, `win1.c` |
| `CHAN_MAX` 16 with FS_WRITE. `CHAN_MAX_WRITE` 256 bytes per read or write call. | `kernel/channel.h` |
| Traps: 0x2A `CH_OPENX` (open with flags), 0x2B `CH_CTL` (flush, truncate, size, tell), 0x2C `FS_OP` (remove, rename, free space...). | `kernel/channel.c` |
| Long-name (VFAT) entries are skipped on read as volume labels; nothing creates them. | `fat16.c` |
| The image loader (`IMG_LOAD`, `PG_START`) opens a root file by flat name. | `kernel/imgload.c` |

### 2.2 Console and input (Stage 2)

| Fact | Source |
|---|---|
| The CON channel driver calls `con_putc` for every output byte, whoever the writer is. There is exactly one console. | `kernel/channel.c` |
| PC `con_putc`: `outb(0x402)` (always), the serial port 0x3F8 (optional), the VGA text buffer (only with `-DQDOS_CON_VGA`). i386VM `con_putc`: MMIO 0x800001. | `platform/pc/platform.c`, `platform/pc/vga.c`, `platform/i386vm/platform.c` |
| VGA: 80x25 text at 0xB8000, hardware cursor via CRTC ports 0x3D4/0x3D5, scroll by moving memory; no attribute control beyond one fixed attribute; no backing store. | `platform/pc/vga.c` |
| Input is one global 128-byte ring fed by the i8042 IRQ 1 handler (PC) or polling (i386VM); `kbd.c` decodes scancode set 1 with Shift only (no Alt, no function keys, no Ctrl). | `kernel/cin.c`, `kernel/kbd.c`, `platform/pc/i8042.c` |
| `QT_CON_WAIT` (0x1C) sleeps the caller until input exists. There is a single global `con_waiter`; a second waiter gets `CON_ERR_BUSY` (-50). | `kernel/cin.c` |
| A paged caller is refused QT_CREATE, POLL_ADD, SCHED_ADD, IMG_LOAD, IMG_START, PG_START and PG_CANCEL (-62); console waits are allowed. | `kernel/paging.c` |

### 2.3 Jobs and paging (Stage 3)

| Fact | Source |
|---|---|
| `JOB_MAX` is 6 (SYSTEM, IDLE, the shell and three more). Each job has a kernel stack (4 KiB) and, for application jobs, a user stack (64 KiB on R11L kernels). | `kernel/jobs.h` |
| Slot 0 is the shell at 0x200000 (256 KiB). Slot 1 is 0x240000, 1 MiB; it is also the paging window. | `docs/R11K-MEMORY-MAP.md` |
| Paging (R11K): `PG_START` 0x25 launches a standard slot-1 a.out in a private 1 MiB image window; `PG_INFO` 0x26, `PG_CAPS` 0x27, `PG_TEST` 0x28, `PG_CANCEL` 0x29. `PG_PROC_MAX` is 3. Frame windows: 7 on the i386VM block map, 2 on the PC/QEMU map. | `kernel/paging.h`, `docs/R11K-PAGING-APP-ABI.md` |
| Only the image window is private. Kernel, heap, stacks and devices are shared and writable (not memory protection). A paged child cannot spawn. | same |
| `job_kill_current` exists (used for paging exceptions). `reclaim_dead` frees dead jobs in trap or yield context, never in an ISR. | `kernel/jobs.c` |
| Job ids are tagged (a reuse counter in the upper bits), so stale ids are detectable. | `kernel/jobs.c` |
| The shell has `RUN` (foreground, synchronous) only. No job listing exists to a program except `qd_job_slot`/`qd_job_info`. | `apps/shell/shell.c`, `sdk/lib/qdos.c` |

## 3. Dependency map

An arrow means "cannot be started before".

```
Stage 1  paths ----------------------------------------------+
  fat16.c  (resolve, mkdir, rmdir, remove/rename by path)    |
    -> win1.c (cwd per job, WIN1: prefix, open flags)         |
    -> channel.c (second driver prefix, FS_OP ops)            |
    -> imgload.c (RUN/START resolve through cwd and /BIN)     |
    -> libqdos / libc (qd_* path calls, fopen/remove/rename/  |
       mkdir/chdir/getcwd/stat)                               |
    -> shell (CD PWD MD RD COPY DEL DIR TYPE)                 |
                                                              v
Stage 2  consoles (independent of Stage 1 code, shares the shell prompt and START /CON)
  vcon.c  (cell buffers, cursor, scroll)
    -> platform hooks (PC VGA renderer, i386VM MMIO/serial fallback)
    -> kbd.c (Alt+Fn decoding), cin.c (per-console queues and waiters)
    -> channel.c (CON channel bound to the owning session's console)
    -> libqdos wrappers, shell CON n, START /CON:n

Stage 3  job control (needs Stage 1 for path resolution of START, Stage 2 for /CON:n)
  jobs.c  (name, console, start state in the table; bounded job query trap)
    -> paging.c (PG_START reuse for background launch, kill of held and running paged jobs)
    -> shell START PS WAIT KILL, COUNTER.AOU, TICKER.AOU

Stage 4  VFAT (needs Stage 1: directory machinery; independent of Stages 2 and 3)
  fat16.c  (LFN chains, 8.3 alias generation, checksum)
    -> directory listing returns the long name; open resolves long name or alias

Stage 5  integration (all stages) -> demo, regressions, documents, package
```

Order of work: 1, 2, 3, then 4 (stretch, own commit and gate), then 5. A blocked stage is recorded with its evidence and the
highest genuinely verified milestone is packaged.

## 4. Decisions taken in the audit

1. **One path convention.** The native form is `WIN1:/DIR/NAME` (absolute) or `WIN1:DIR/NAME` (relative to the calling job's current
   directory). Inside programs and the shell a bare `DIR/NAME` or `/DIR/NAME` means the same, mapped by the library (`qd_fsname`) or the
   shell. Parsing lives in `fat16_resolve`, not in a shell alias table. Separator is `/`. Case-insensitive lookup; created names are
   upper-case 8.3 (until Stage 4).
2. **Legacy names are a compatibility interface.** `WIN1_NAME` is the root file `NAME` (strict upper-case 8.3 at the kernel, folded
   to upper case by the C library). Underscores are never reinterpreted: `WIN1_ZORK1_Z3` is the root file `ZORK1_Z3`, `WIN1_SRC_X.TXT`
   is the root file `SRC_X.TXT`, never `/SRC/X.TXT`. `WIN1_` followed by a name that contains `/` is a root-anchored native path.
3. **Current directory is per job**, inherited at creation (copy of the creator's directory), and changed only by that job. A directory that
   is some job's current directory cannot be removed (`BUSY`).
4. **The volume root is the ceiling.** `..` at the root stays at the root; there is no way to name anything outside the volume.
5. **Consoles are an opt-in kernel layer** (`QDOS_VCON`); the legacy single-console behaviour is the default and is what the
   frozen images keep.
6. **Job control uses the existing tagged job ids and the R11K paging launch**; it adds a bounded, versioned query trap rather than
   exposing kernel structures.
7. **VFAT is limited and labelled** (ASCII subset) if it is attempted; unsupported names are rejected explicitly.

## 5. Risks recorded before implementation

| Risk | Mitigation |
|---|---|
| The kernel is compiled by GCC 1.40 (K&R-friendly C89). | New code is written in strict C89 and host unit tests compile it with `-std=c89 -pedantic -Wall -Wextra -Werror`. |
| Large new code in `fat16.c` could break R11L behaviour. | The R11L host tests (fat16w 290 x3, block 88, layout 39) are re-run in `tests/host/r11m/run.sh`; the legacy flat-name tests are kept; default images are hash-checked. |
| Tiny386 drops input bursts and cannot send Alt+Fn. | Tiny386 uses paced keys (`PC_KEY_DELAY`) and the shell `CON n` fallback; Alt+Fn is tested on QEMU only. |
| Prompt detection by the old harnesses expected a bare `> `. | `tests/interactive/session.py` and the QEMU capture accept `/DIR> `. |
| Console input waits and wakeups could be lost under preemption. | Stage 2 gate includes a stress test and a documented wake protocol. |
