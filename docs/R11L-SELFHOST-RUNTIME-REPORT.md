# R11L runtime report: native GCC 1.40 self-hosting stretch

Base: frozen R11K `82405025f873dcf13dea8bf352399add195ebaca` (branch `r11l-selfhost`; the final source commit is in `PACKAGE.md`).
Every claim below is classified **host build**, **guest executed**, **independent comparison**, **not tested** or **blocked**.
All five stages are proven; the boundary of the word "self-hosting" is stated in section 6 and in `R11L-KNOWN-LIMITATIONS.md`.

## 1. What was achieved

Inside QDOS/386, on a writable FAT16 scratch volume, with nothing but guest programs:

```
ENTER.AOU HELLO.C  ->  CC.AOU  ->  CCCP.AOU   HELLO.C  -> HELLO.I      (GCC 1.40 preprocessor)
                                   CC1.AOU     HELLO.I  -> HELLO.S      (GCC 1.40 compiler, i386)
                                   AS.AOU      HELLO.S  -> HELLO.O      (GNU as 1.38)
                                   AS.AOU      CRT0.S   -> CRT0.O
                                   LD.AOU      CRT0.O HELLO.O libs -> HELLO.AOU   (binutils-1.94 i386 a.out ld)
                      RUN HELLO.AOU  ->  "Hello from guest-compiled C, argc=1", exit status 7
```

`CC.AOU` (`sdk/examples/r11l/qdcc.c`) is the driver: it starts each tool as a paged child (`qd_pg_startv`), waits for it,
reads its exit status, stops at the first failing stage, and deletes the stale outputs first, so a failed build never
leaves a runnable old program. The tools are the user's historical GCC 1.40 (cccp, cc1), GAS 1.38 and binutils-1.94 ld,
unmodified except for the two edits in `port/patches/gcc-1.40.patch` (`R11L-TOOLCHAIN-PORT.md`).

## 2. Evidence by stage

| Stage | Claim | Class | Platforms | Result | Evidence (under `logs/r11l/final/` unless noted) |
|---|---|---|---|---|---|
| 0 | audit, dependency table, blockers with measured numbers | host build + guest executed (MEMSTAT runs) | i386VM | done | `docs/R11L-AUDIT.md`, `R11L-DEPENDENCIES.csv/.md` |
| 1 | writable FAT16: create, write, append, truncate, rename, remove, seek, flush, disk full, root full, read-only device, I/O error, persistence across emulator restarts, 107 interrupted-operation cuts | guest executed + independent comparison (`fsck.fat`, `tools/r11l-fatcheck.py`) | i386VM, Tiny386, QEMU | WRTTEST 88 / 88 / 93 / 93 / 7 / 1 checks, 0 fail; Tiny386 88, QEMU 88; persistence 4 + 5 checks; sweep: 107 cuts, 0 broken, 0 checker disagreements; every image clean | `stage1/summary.txt` |
| 2 | guest GAS 1.38 and a.out ld produce the host's bytes | guest executed + independent comparison | i386VM, Tiny386, QEMU | CRT0.O, HELLO.O, HELLO.AOU byte-identical to the host oracle on all three; an assembler error and an unresolved symbol stop the pipeline with the driver's statuses 12 / 13 and leave no output (Stage 4 judge, `stage4/`); the direct-run statuses of `as`/`ld` for those cases were observed in the earlier Stage 2 runs on i386VM, whose raw logs were superseded by this final run | `gate23/` |
| 3 | guest cccp and cc1 produce the host's bytes | guest executed + independent comparison | i386VM, Tiny386, QEMU | HELLO, FUNCS, FPT: `.I .S .O .AOU` identical at every stage; MACROS: identical except cc1's string pool (`EQUIVALENT`); three malformed sources give the host's exit statuses (0/33, 33/0, 33/0) and diagnostics; 32 judge checks per platform | `gate23/` |
| 4 | orchestration: sequential launch/wait/exit status, error stop, no stale outputs, repeat and clean builds, two sources give two binaries, multi-source link, `-c` | guest executed + independent comparison | i386VM (paging kernel) | 108 / 108 | `stage4/vm.judge.txt` |
| 5 | end to end on a writable scratch FAT16: type HELLO.C in the guest, build, run; change the source, rebuild, run; invalid source, no stale output runs; new emulator process on the same disk: files persist, CRCs equal, run, rename + rebuild + `CMP` | guest executed + independent comparison | i386VM, QEMU (VGA, monitor capture), Tiny386 | suite A 40 / 40 and suite B 30 / 30 on **each** of the three; final HELLO.C/.I/.S/.O/.AOU and CRT0.O byte-identical to the host oracle; guest `SUM` (CRC-32) equals `zlib.crc32` of the host copy and equals what session A printed before the "reboot" | `stage5/` |
| regr. | earlier rounds unharmed | see section 4 | | see section 4 | `regress-r11l-kernels*`, `../regress/r11k*` |

"Reboot" means a new emulator process started on the final disk image of the previous process (cold boot of the kernel and
shell from the boot sector; the volume is the only state carried over).

### Typed source and scripted input

The source is entered with `ENTER.AOU` through each emulator's console input path: a pipe on i386VM, PS/2 scancodes through
`-term` on Tiny386, `sendkey` through the QEMU monitor on QEMU. The keystrokes come from the test harness (standing in for a
person); the host never touches the content of `HELLO.C` after it is typed. The typed file is byte-identical to
`tests/r11l/typed/HELLO.C` (checked on the volume with `mtools`).

## 3. Excerpt (Tiny386, `stage5/tiny386-tiny-a.txt`)

```
> RUN ENTER.AOU HELLO.C
  ... 8 lines ...
ENTER: 8 lines, 147 bytes -> HELLO.C
exit status 0
> RUN CC.AOU -O -v HELLO.C
CC: CCCP.AOU -nostdinc -undef -Di386 -D__i386__ -I. HELLO.C HELLO.I
CC: CC1.AOU -quiet -O HELLO.I -o HELLO.S
CC: AS.AOU -o HELLO.O HELLO.S
CC: HELLO.C -> HELLO.O
CC: AS.AOU -o CRT0.O CRT0.S
CC: LD.AOU -N -T SLOT1.LD -e _start -o HELLO.AOU CRT0.O HELLO.O LIBM.A LIBC.A LIBGCC.A LIBQDOS.A
CC: HELLO.AOU built (27973 bytes)
exit status 0
> RUN HELLO.AOU
Hello from guest-compiled C, argc=1
exit status 7
```

An invalid source in the same suite (`int x = 5` without the semicolon, entered over `HELLO.C`):

```
> RUN CC.AOU -O HELLO.C
HELLO.C: In function main:
HELLO.C:4: parse error before `return'
CC: compiling HELLO.I failed (status 33)
exit status 11
> RUN HELLO.AOU
no such file
```

## 4. Regressions

| Group | Result |
|---|---|
| Default (non-paging, no write option) images: R11I block, R11K paging, R11K PC default, R11K PC paging, R11H PC, R11H release VM | all six **byte-identical** to the frozen hashes after every change of this round (`default-images.txt`; `scripts/r11l-default-check.sh`) |
| R11K matrix re-run unchanged on the frozen R11K kernels (`scripts/r11k-matrix.sh`, 1 round; paging, MULTI/NEG, PGRUN, SAME/SAMELNCH, large-transfer loaders, test kernels, R11F regression sequence incl. Phoon/time/FPU, default images, block and legacy kernels, host tests) judged by `scripts/r11k-assert.py` | PASS 358, FAIL 0, NA 6; NEG: 848 checks, 0 failures in each of 6 transcripts; the 6 NA are the documented ones (TWICET W stdin, short PGRUN programs, QEMU 80-column cut); all volumes unchanged (`../regress/r11k-assert.txt`) |
| The same R11K/R11H/R11F sequences on the **R11L kernels** (writable FAT16 option, new traps, 64 KiB stacks, `shell-l`; `scripts/r11l-regress.sh`) on i386VM block, Tiny386 and QEMU VGA, judged by the same assert script | PASS 235, real FAIL 0, NA 6; the 35 other failing checks belong to 18 groups that need R11K-only kernels or the host and are not part of this run (large-transfer loaders, paging test kernels, legacy RAM-disk kernels, the unpaged-task-fault test, host unit tests); they are covered by the row above (`regress-r11l-kernels.filter.txt`). NEG: 848 checks, 0 failures in each of 4 transcripts (i386VM paging and non-paging, Tiny386 paging and non-paging); the R11F sequence (Phoon, time, FPU) passes and its PASS/FAIL lines equal those of the non-paging R11L kernel |
| Read-only FAT16 | R11L kernels: `WRTTEST ro` on a read-only device (7 checks, image byte-identical before/after); R11K matrix above runs every volume read-only |
| Host unit tests (R11L: writable block device, FAT write layer on three volume shapes; R11I; R11K boot images) | pass (`host-tests.log`, `../regress/r11k/host-tests*.txt`) |
| Integrity | `fsck.fat -n` and `tools/r11l-fatcheck.py` agree on every final image: 33 images, 0 problems (`integrity.txt`) |

One regression was found and fixed by this re-run: with `-DQDOS_FS_WRITE` a `write`/`putc` on a channel not opened for
writing returned `-80` instead of `-15` NOTSUP, which the R11E `QDOSAPI` test expects. The driver was corrected
(`kernel/win1.c`); the test was not touched.

## 5. Defects found while running the real tools (and what they were)

| # | Symptom | Root cause | Where fixed |
|---|---|---|---|
| 1 | cccp triple-faulted at start | tool objects compiled against a 20-byte `struct stat`; libc's `fstat` wrote 28 | all objects rebuilt against `port/include/sys/stat.h` |
| 2 | cccp "I/O error on output"; GAS "Unknown error #2" | missing `freopen`/`write(fileno())` support; missing `sys_errlist` | `sdk/libc` |
| 3 | tool links that "succeeded" with undefined or duplicate symbols | stale stub objects; the historical `ld` exits 0 after "multiple definition" | `scripts/r11l-link-tools.sh` fails on any ld output |
| 4 | first guest `ld` corrupted an existing output | the 80-character shell line truncated the command | `shell-l` (160 characters) |
| 5 | paging kernel with the write option did not build | R11L traps collided with the R11K paging traps 0x25-0x29 | renumbered to 0x2A-0x2C |
| 6 | cc1 crashed on a struct passed by value | misplaced parenthesis in the legacy tree's `expr.c` | `port/patches/gcc-1.40.patch` |
| 7 | QEMU runs stalled 900 s per `ENTER` | the capture script waited for a prompt that only comes after the body is typed | the `ENTER` command line is a no-wait ("!") line |
| 8 | "ld hangs on Tiny386" | Tiny386's terminal input dropped the tail and the Enter of a 99-character typed line; the shell waited | paced typing (`PC_KEY_DELAY`); not a guest defect |
| 9 | `QDOSAPI` regression (section 4) | write-option driver returned `-80` | `kernel/win1.c` |
| 10 | stale oracle `HELLO.AOU` | the oracle holds a linked program, so it changes with every library rebuild | `scripts/r11l-oracle-all.sh` regenerated; the judges also rebuild a fresh host reference each time |

## 6. The exact boundary of "self-hosting"

| Done in the guest (guest executed) | Done on the host (host build) |
|---|---|
| typing `HELLO.C`; preprocessing, compiling, assembling (HELLO and CRT0) and linking it; running the result; rebuilding after a change; refusing to run a failed build | building the kernels, SDK libraries, `ENTER/CC/SUM/CMP/REN/DEL`, and the four tools `AS/CCCP/CC1/LD.AOU` (compiled by GCC 1.40 itself - a host-run copy - assembled with GAS 1.38, linked with the historical ld; the generated tables of cc1 come from the host bootstrap); populating the initial disk (base programs, tools, SDK headers, `CRT0.S`, the libraries `LIBC.A LIBM.A LIBGCC.A LIBQDOS.A`, `SLOT1.LD`) |

Not claimed: GCC compiling GCC (or ld, as, the libraries) on the guest; `ar`/`ranlib`/`make` in the guest; a source
tree with directories; a build without the paging kernel; more than one build at a time. See `R11L-KNOWN-LIMITATIONS.md`.

*Not tested*: rebuilding `CC1.AOU` or any tool inside the guest; Tiny386/QEMU runs of the Stage 4 error-code matrix (it
ran on i386VM; Tiny386 and QEMU ran the Stage 5 suites, which include the syntax-error path and Gate 2/3's error paths);
real hardware.

## 7. Resource numbers (MEMSTAT builds, i386VM)

Peak live heap / largest block / peak stack: cccp 23.6 KB / 9.5 KB / 5.9 KB; cc1 176.8 KB / 4.1 KB / 14.1 KB; as 203.7 KB / 64 KB / 2.4 KB;
ld 197.0 KB / 16 KB / 7.2 KB; at most 9 open channels (ld). Full table: `R11L-AUDIT.md`. The measurement builds are
separate binaries (`scripts/r11l-build-ms.sh`) and never used in an identity run.

## 8. Build identities (SHA-256, first 16 hex digits)

| Item | Hash |
|---|---|
| `AS.AOU` / `CCCP.AOU` / `CC1.AOU` / `LD.AOU` | 25a72c2f6c385083 / 0fceedd2e620ad22 / 7b9ada318840ae55 / a1cf5e392ce2dbbd |
| `CC.AOU` / `ENTER.AOU` | 9cf2132c0ba32552 / fff873c5b1adfa0b |
| R11L i386VM paging kernel / PC paging / PC paging VGA | 6e63e02798770471 / c871cd74ad03c390 / 131fe37703c7e4a1 |
| R11L i386VM non-paging / PC non-paging | 90e4b4b7ae2d6317 / de35f4248768922b |
| `libc.a` / `libqdos.a` | 4758275f4c436d68 / 51dde9b29b53d1c3 |
| host oracle `HELLO.AOU` (= the guest's) | fb3822e92f29e55e |

The SHA-256 of every original legacy source file is in `toolchain/ORIGINAL.SHA256` (820 files).

## 9. Reproduce

```
export I386VM=<historical tree> QDOS386_TOOLS=<same tree> TINY386=<tiny386_headless> HARNESS_DIR=<dir with i386vm-r9, -block, -block-fd>
sh scripts/r11l-build-all.sh                 # libraries, apps, tools, kernels
sh scripts/r11l-final.sh logs/r11l/rerun     # every test of this report, judged; summary.txt has one line per step
```

Licences and provenance: GCC, GAS, binutils, bison and libiberty sources are GPL and keep their notices; `R11B/C/D/E-LICENSE-PROVENANCE.md`
cover the SDK libraries. Nothing from the private Zork I story is included.
