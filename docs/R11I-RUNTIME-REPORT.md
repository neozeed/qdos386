# R11I runtime report — host-backed i386VM block device

Branch `r11i-block` from frozen R11H-FIX1 `4affc37`.  All runs are SCRIPTED (keys piped or sent by
the harness driver); no result below depends on a single delayed screenshot.  Logs: `logs/r11i/`.

## 1. Identities (kernel image and disk recorded separately)

| Artefact | SHA256 (16) | Note |
|---|---|---|
| i386VM R9 harness (stock + input-status + r9-ram16) | `a4724f79b26a716e` | unchanged from FIX1 |
| i386VM R9 **block** harness (+ `i386vm-block.patch`) | `5970e16223075cf5` | host-dependent build (see README) |
| `patches/i386vm-block.patch` | `92cca7889a4a3e86` | separate, optional |
| block kernel-only image `qdos386-r11i-block.bin` (3,670,016 B) | `3ee37c19ac52acbc` | rebuilt twice, byte-identical |
| block kernel `kernel-raw.bin` (52,960 B) | `22d914e534f28527` | |
| legacy RAM-disk image (12,582,912 B) | `c5462138ea630fa7` | **identical** to R11H-FIX1 `qdos386-r11h-rel.bin` |
| legacy kernel `kernel-raw.bin` (52,800 B) | `70902c40450c3786` | identical to FIX1 |
| PC `r11h-shell.img` / `kernel.aout` | `8f22b0b2613d0554` / `5fed2619e0a0e8d1` | **identical** to FIX1 (frozen PC recipe) |
| release volume `win1-r11h.img` (disk) | `672806ad2c2ca337` | unchanged after every matrix |
| negative volume `win1-r11h-neg.img` (disk) | `5af618370d810bb4` | unchanged after every matrix |
| Tiny386 / BIOS / VGA BIOS | `fb63b405…` / `7bd968419c81fcf8` / `f5aedb52cde5c88d` | unchanged |

The block image is not, and is not expected to be, byte-identical to the RAM-disk image.  Its kernel is
160 bytes larger (block backend), so the free-heap figures it prints are 160 bytes lower.

## 2. Host unit tests (strict C89, `-std=c89 -pedantic -Wall -Wextra -Werror`)

| Test | Result |
|---|---|
| `blockdev_test` | 88 checks, 0 failures — PASS |
| `layout_test` | 39 checks, 0 failures — PASS |

## 3. Three-target matrix, three rounds (`scripts/r11i-matrix.sh`, `logs/r11i/matrix/`)

| Target | MULTI ×3 | NEG ×3 |
|---|---|---|
| i386VM legacy RAM disk (one-argument harness) | PASS ×3 | 848/0 PASS ×3 |
| i386VM block (release / negative volume as host file) | PASS ×3 | 848/0 PASS ×3 |
| Tiny386 PC IDE (unchanged path) | PASS ×3 | 848/0 PASS ×3 |

Volumes hashed before and after: unchanged.  NEG covers job-ID reuse (780 stale ids refused over 40
churn cycles), owner protection, slot/profile collisions and heap integrity; MULTI covers two
concurrent fixed-origin profiles, canaries and shell recovery.  The MULTIB `acc` value is a
time-bounded LCG and legitimately differs between runs; it is not a content checksum.

## 4. Backend equivalence (`logs/r11i/equiv/`)

The same sequence (`DIR`, `TYPE` ×3, `RUN FTEST`, `RUN STREAM`, `RUN CATFIL README.TXT`) on all three
backends against the release volume:

* legacy vs block i386VM: identical transcripts except the final `HALT EIP` (different kernel code).
* block vs Tiny386: identical file output; differences are only how each harness echoes typed lines.
* `TYPE` output for README.TXT (4,011 B), LINES.TXT, ABCDEFGH.TXT is byte-identical to the file bytes
  read from the host image by an independent FAT16 reader.
* FILETEST (seek/read), STREAM and FILECAT PASS on every backend.

## 5. Application regressions (`logs/r11i/regression/`)

* Tiny386 PC R11F regression sequence: matches R11F round 1 except the documented differences (six
  added R11H files, 47→53 files, heap −2,608 B, FPMULT/FPJOB timing counters).
* i386VM legacy and block, R11F regression (`none`) and pinned-clock (`pin`, Phoon ×14 PHCHK PASS,
  TIMEABI PASS): block equals legacy line for line except free heap (−160 B) and `HALT EIP`.
* Zork I: **not run**.  The private story volume is not in this worktree and is not embedded.
* The three known `floatundidf` LGTEST discrepancies are untouched by R11I (no compiler-runtime change).

## 6. Failure cases (block harness)

| Case | Result |
|---|---|
| no disk argument | boots; device NODISK, count 0 → `SHELL: WIN1_ volume not mounted`; clean halt |
| missing file / empty / size 511 | refused before boot: `refused (-2)` / `(-3)` / `(-4)`, exit 2 |
| 3 MiB of zeros / first 1 MiB of volume / volume minus one sector | boots; mount refused; clean halt |
| last valid sector, first invalid sector, 3 repeated reads (guest, test hook) | `BLKT: sectors 6144`, all checked, PASS |
| short read: copy truncated to 41,472 B under the running shell | `DIR` → 53 files; `RUN MULTI`, `TYPE`, `RUN HELLO` → `error -40` each; shell continues and exits normally |
| block kernel on the **unpatched** harness | stops at once: `read outside RAM: 00800017` (no hang) |
| legacy image on the **patched** harness | boots from its RAM disk, `MULTI: PASS` |

None hangs; none writes a file (all image hashes recorded in `neg-disks/after.sha`).

## 7. Read-only proof

* No write command exists in the device; `blk_write8` only stores CMD/LBA state.
* `scripts/r11i-fdcheck.sh`: during a live run the disk descriptor's `fdinfo` flags are `0100000`,
  access mode 0 = `O_RDONLY`; disk hash identical before and after → `FDCHECK: PASS`.
* Every matrix, regression and smoke run records volume hashes before and after: never changed.

## 8. Spare-span reclaim

Host: `layout_test` (§2).  Guest: test-only kernel `-DQDOS_TEST_SPARE` (`logs/r11i/spare/`):
`SPARE: base 9437184 end 16777216 pages 1792` / `SPARE: PASS` — the allocator hands out exactly
`0x900000…0x1000000`, a further page lands outside it, every page is written and read back, the
transfer area and `0x801000…0x8FFFFF` are unchanged, the block device still reads sector 0, and the
pool is empty after release.  The shell then loads and `MULTI: PASS`.  Details: `R11I-MEMORY-MAP.md`.

## 9. QEMU VGA smoke (PC path, `logs/r11i/qemu-smoke-*`)

`r9shell-vga.img` (unchanged PC VGA build) under QEMU, VGA text captured from `0xB8000` on every change:

* negative volume as `hdb`: `MULTI: PASS`, `NEG: checks 848, failures 0`, `NEG: PASS`; volume unchanged.
* release volume as `hdb`: MULTI PASS; NEG reports 10 failures, all `-44` (file not found) because the
  release volume does not contain NEG's malformed inputs — the same fixture-missing case identified in
  FIX1, not a regression.  Kept for the record.

## 10. Open items (not blocking)

* `BLK_IOERR` (host `fread` error) is code-reviewed but not reproducible portably in C89 tests.
* Throughput was not measured.  Each sector costs one host `fseek`+`fread` and 512 byte-wide guest
  reads with interrupts off; the full matrix ran in normal time.
* The block harness binary hash depends on the host C compiler; the patch and device sources are the
  reproducible inputs.
* An application-level allocator for the spare span, and paging, are deliberately out of scope.
* The R11F 38-byte application-size discrepancy remains documented in FIX1 and is not reopened.
